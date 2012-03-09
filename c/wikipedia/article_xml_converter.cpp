#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <expat.h>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include "wiki_scanner.h"

using namespace std;
using namespace boost;


/* Typedefs */

typedef struct {
  char* cdata;
  size_t cdata_len;
  int in_title;
  char* title;
  int in_text;
  char* text;
} xml_progress_t;

static void XMLCALL load_start(void *user_data, const char *name, const char **attr);
static void XMLCALL load_end(void *user_data, const char *name);
static void XMLCALL character_data_handler(void* user_data, const XML_Char *s, int len);
static void parse_outlinks(xml_progress_t*);

/* Globlals */
static const regex redirect_rx("^#REDIRECT\\s+\\[\\[(.*?)\\]\\]",boost::regex::perl);
static ofstream output("/tmp/titles");

int main(int argc,char** argv)
{
  if (argc < 2) {
    cerr << "Please specify an articles.xml" << endl;
    exit(-1);
  }

  char buf[BUFSIZ];
  FILE* xml_file = NULL;
  xml_progress_t buffer = {0};
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, &buffer);
  XML_SetElementHandler(parser, load_start, load_end);
  XML_SetCharacterDataHandler(parser,character_data_handler);
  int done = 0;

  xml_file = fopen(argv[1],"r");
  if (xml_file) {
    do {
      size_t len = fread(buf, 1, sizeof(buf),xml_file);
      done = len < sizeof(buf);
      if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
        fprintf(stderr, "%s at line %lu\n", XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
      }
    } while (!done);
  } else {
    printf("Error opening file: %s\n",strerror(errno));
    fprintf(stderr,"Cannot find file\n");
  }
  XML_ParserFree(parser);
}

/* Static Private Implementation */

static void XMLCALL
load_start(void *user_data, const char *name, const char **attr)
{
  xml_progress_t* prog = (xml_progress_t*) user_data;
  if (!strcmp(name,"title")) {
    prog->in_title = 1;
  } else if (!strcmp(name,"text")) {
    prog->in_text = 1;
  }
}

static void XMLCALL
load_end(void *user_data, const char *name)
{
  xml_progress_t* prog = (xml_progress_t*) user_data;
  if (!strcmp(name,"title")) {
    prog->title = prog->cdata;
    prog->in_title = 0;
  } else if (!strcmp(name,"text")) {
    prog->text = prog->cdata;
    prog->in_text = 0;
  } else if (!strcmp(name,"page")) {
    parse_outlinks(prog);
    free(prog->title);
    free(prog->text);
  }
  prog->cdata = NULL;
}

static void XMLCALL
character_data_handler(void* user_data, const XML_Char *s, int len)
{
  xml_progress_t* prog = (xml_progress_t*) user_data;
  if (len > 0) {
    if (prog->in_title || prog->in_text) {
      if (prog->cdata) {
        size_t new_cdata_len = prog->cdata_len + len;
        prog->cdata = (XML_Char*) realloc(prog->cdata,sizeof(XML_Char) * (new_cdata_len + 1));
        memcpy(prog->cdata + prog->cdata_len,s,len);
        prog->cdata[new_cdata_len] = '\0';
        prog->cdata_len = new_cdata_len;
      } else {
        prog->cdata_len = len;
        prog->cdata = (XML_Char*) malloc(sizeof(XML_Char) * (len + 1));
        memcpy(prog->cdata,s,len);
        prog->cdata[len] = '\0';
      }
    }
  }
}

/* Here we search for outlinks and output them */
static void parse_outlinks(xml_progress_t* prog)
{
  char* p = prog->text;
  char* pe = p + prog->cdata_len;

  wiki_token_t token;
  scan(&token,NULL,p,pe);
  while(token.type != END_OF_FILE) {
    cout << "Token Scanned: " << token.type << endl;
    scan(&token,&token,NULL,pe);
  }
}
