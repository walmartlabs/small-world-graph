/* Support Large File */
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

using namespace std;

shared_ptr<S3Helper> __s3;
static string __bucket = "";
static size_t __total_pages = 0;
static size_t __pages_imported = 0;

static void handle_node(char* title, char* text, void* traveler) {
  if (title && text) {
    stringstream output;
    string filename = string(title) + string(".html");
    output << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
    output << "<html xmlns='http://www.w3.org/1999/xhtml'>\n\t<head>\n\t\t<meta http-equiv='content-type' content='text/html; charset=utf-8' />\n\t\t<title>" << title << "</title>\n\t\t<script id='dec' type='text/javascript' src='http://assets.cruxlux.com/javascripts/dec.js'></script>\n\t</head>\n\t<body>";

    char* transformed = wikitext_to_plaintext(text);
    if (transformed) {
      output << transformed;
      free(transformed);
      output << "\n\t</body>\n</html>";

      string str_output = output.str();
      //__s3->put(bucket.c_str(),filename.c_str(),str_output.c_str(), str_output.size(), "text/html",true);
    }
    if (__pages_imported % 100 == 0) {
      fprintf(stdout,"\rPages Imported: %9d / %d",(int) __pages_imported, (int) __total_pages);
      fflush(stdout);
    }
  } 

  __pages_imported++;

  free(title);
  free(text);
}

static void count_page(char* title, char* text, void* traveler) {
  if (useful_title(title,title + strlen(title))) {
    __total_pages++;
    if (__total_pages% 1000 == 0) {
      fprintf(stdout,"\rPages Counted: %9d",(int) __total_pages);
      fflush(stdout);
    }
  }  
  free(title);
  free(text);
}

int main(int argc,char** argv)
{
  if (argc < 3) {
    cout << "Please specify a filename and a destination bucket" << endl;
    exit(-1);
  }

  struct sigaction sig_pipe_action;
  memset(&sig_pipe_action,'\0',sizeof(sig_pipe_action));
  sig_pipe_action.sa_handler = SIG_IGN;
  sigaction(SIGPIPE,&sig_pipe_action,NULL);

  __bucket = argv[2];
  __s3 = S3Helper::instance();
  wikipedia_xml_parse(argv[1],count_page, NULL);
  wikipedia_xml_parse(argv[1],handle_node, NULL);

  fprintf(stdout,"\rPages Imported: %9d / %d",(int)__pages_imported, (int)__total_pages);
  fflush(stdout);
  cout << endl;
  return 0;
}
