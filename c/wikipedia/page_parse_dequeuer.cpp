#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vector>
#include <list>

#include "wikipedia_xml_parser.h"
#include "wiki_parser.h"
#include "wiki_title_validator.h"
#include "s3_helper.h"
#include <signal.h>
#include "string_utils.h"

#include "PageParseQueue.h"
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

/* Globals */
shared_ptr<S3Helper> __s3;
static size_t __total_pages = 0;
static size_t __pages_imported = 0;
static volatile sig_atomic_t gracefully_quit = 0;

static void handle_sigint(int signal) {
  if (!gracefully_quit) {
    cout << "Sent Quit Signal" << endl;
    gracefully_quit = 1;
  } else {
    exit(0);
  }
}

static void handle_raw_page(const char* title, const char* text, const char* s3_bucket, const char* directory) {
  if (title && text) {
    stringstream output;
    string filename = string(title) + string(".html");
    output << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
    output << "<html xmlns='http://www.w3.org/1999/xhtml'>\n\t<head>\n\t\t<meta http-equiv='content-type' content='text/html; charset=utf-8' />\n\t\t<title>" << title << "</title>\n\t</head>\n";
    output << "\t<body onload=\"document.domain='cruxlux.com';if(parent&&parent!=self){parent.show_from_frame(document.title);}var s=document.createElement('script');s.src='http://assets.cruxlux.com/javascripts/dec.js';document.body.appendChild(s)\">";

    char* transformed = wikitext_to_plaintext(text);
    if (transformed) {
      string result = replace_all(transformed,"\n","\n<p></p>");

      output << result;
      output << "\n\t</body>\n</html>";

      string str_output = output.str();

      if (directory) {
        chdir(directory);
        ofstream file(filename.c_str());
        if (file.is_open()) {
          file << result;
          file.close();
        }
      }

      if (s3_bucket) {
        cout << "[P] " << filename << " -> " << s3_bucket << endl;
        __s3->put(s3_bucket,filename.c_str(),str_output.c_str(), str_output.size(), "text/html",true);
      }
      free(transformed);
    }
  } 

}

int main(int argc,char** argv)
{
  struct sigaction sig_pipe_action;
  memset(&sig_pipe_action,'\0',sizeof(sig_pipe_action));
  sig_pipe_action.sa_handler = SIG_IGN;
  sigaction(SIGPIPE,&sig_pipe_action,NULL);
  signal(SIGINT,handle_sigint);

  char* host = (char*) "localhost";
  int port = 10010;
  int c;
  char* s3_bucket = NULL;
  char* directory = NULL;

  while ((c = getopt(argc, argv, "h:ps:d:")) != -1) {
    switch (c) {
      case 'h':
        host = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        if (port == 0) {
          cout << "Invalid port" << endl;
          exit(-1);
        }
        break;
      case 's':
        s3_bucket = optarg;
        break;
      case 'd':
        directory = optarg;
        break;
    }
  }

  shared_ptr<TSocket> socket(new TSocket(host,port));
  socket->setConnTimeout(1000);
  socket->setRecvTimeout(1000);
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  PageParseQueueClient client(protocol);

  __s3 = S3Helper::instance();

  for(;;) {
    try {
      if (gracefully_quit == 1) {
        cout << "Gracefully Quitting" << endl;
        exit(0);
      }
      transport->open();
      vector<RawPage> work_units;
      client.dequeue(work_units);
      transport->close();
      if(work_units.size() > 0) {
        int dequeued_amount = work_units.size();
        for(vector<RawPage>::const_iterator ii = work_units.begin(); ii != work_units.end(); ii++) {
          RawPage page = *ii;
          handle_raw_page(page.title.c_str(),page.content.c_str(),s3_bucket,directory);
          __pages_imported++;
          fprintf(stdout,"\rDequeued %d | Pages Imported: %9d ",(int) dequeued_amount, (int) __pages_imported);
          fflush(stdout);
        }
      } else {
        fprintf(stdout,"\rQueue is Empty");
        fflush(stdout);
        sleep(10);
      }
    } catch (TException &tx) {
      transport->close();
      printf("ERROR: %s\n", tx.what());
      printf("Sleeping for 10s before retry\n");
      sleep(10);
    }
  }


  fprintf(stdout,"\rPages Imported: %9d / %d",(int) __pages_imported, (int) __total_pages);
  fflush(stdout);
  cout << endl;
  return 0;
}
