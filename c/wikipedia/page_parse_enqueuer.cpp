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
static char* __host = NULL;
static int __port = NULL;
static string __bucket = "";
static size_t __total_pages = 0;
static volatile sig_atomic_t gracefully_quit = 0;

static void handle_sigint(int signal) {
  if (!gracefully_quit) {
    cout << "Sent Quit Signal" << endl;
    gracefully_quit = 1;
  } else {
    exit(0);
  }
}

static void enqueue(vector<RawPage>* pages)
{
  shared_ptr<TTransport> socket(new TSocket(__host,__port));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  PageParseQueueClient client(protocol);
retry:
  try {
    transport->open();
    try {
      client.enqueue(*pages);
    } catch (QueueFullException &qfe) {
      transport->close();
      cout << "Queue Full. Retrying in 10s" << endl;
      sleep(10);
      goto retry;
    }
    transport->close();
  } catch (TException &tx) {
    cout << "Sleeping for 10s before retry" << endl;
    sleep(10);
  }
}

static void append_page(char* title, char* text, void* traveler) {
  if (title && text && useful_title(title,title + strlen(title))) {
    __total_pages++;
    vector<RawPage>* pages = (vector<RawPage>*) traveler;
    RawPage page;
    page.title = title;
    page.content = text;
    pages->push_back(page);
    if (__total_pages% 1000 == 0) {
      fprintf(stdout,"\rPages Counted: %9d",(int) __total_pages);
      fflush(stdout);
    }
    if (pages->size() == 1000) {
      enqueue(pages);
      pages->clear();
    }
  }  
  free(title);
  free(text);
}

int main(int argc,char** argv)
{
  struct sigaction sig_pipe_action;
  memset(&sig_pipe_action,'\0',sizeof(sig_pipe_action));
  sig_pipe_action.sa_handler = SIG_IGN;
  sigaction(SIGPIPE,&sig_pipe_action,NULL);
  signal(SIGINT,handle_sigint);

  __host = (char*) "localhost";
  __port = 10010;
  int c;

  while ((c = getopt(argc, argv, "h:p")) != -1) {
    switch (c) {
      case 'h':
        __host = optarg;
        break;
      case 'p':
        __port = atoi(optarg);
        if (__port == 0) {
          cout << "Invalid port" << endl;
          exit(-1);
        }
        break;
    }
  }

  cout << "Parsing: " << argv[1] << endl;
  vector<RawPage> pages;
  wikipedia_xml_parse(argv[1],append_page, &pages);
  enqueue(&pages);
  fprintf(stdout,"\rPages Counted: %9d",(int)__total_pages);
  fflush(stdout);

  return 0;
}
