#define _FILE_OFFSET_BITS 64

#include "wikipedia_xml_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>
#include <errno.h>


typedef struct {
  char* cdata;
  int cdata_len;
  int in_title;
  char* title;
  int in_text;
  char* text;
  void(* fn_ptr)(char* title,char* text, void* traveler);
  void* traveler;
} xml_progress_t;

/* Prototypes */
static void XMLCALL load_start(void *user_data, const char *name, const char **attr);
static void XMLCALL load_end(void *user_data, const char *name);
static void XMLCALL character_data_handler(void* user_data, const XML_Char *s, int len);

/* Public Implementation */
void wikipedia_xml_parse(const char* filename,void(* fn_ptr)(char* title,char* text,void* traveler), void* traveler)
{
  char buf[BUFSIZ];
  FILE* xml_file = NULL;
  xml_progress_t progress = {0};
  progress.fn_ptr = fn_ptr;
  progress.traveler = traveler;
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, &progress);
  XML_SetElementHandler(parser, load_start, load_end);
  XML_SetCharacterDataHandler(parser,character_data_handler);
  int done = 0;

  xml_file = fopen(filename,"r");
  if (xml_file) {
    do {
      size_t len = fread(buf, 1, sizeof(buf),xml_file);
      done = len < sizeof(buf);
      if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
        fprintf(stderr, "%s at line %lu\n", XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
        break;
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
    prog->fn_ptr(prog->title,prog->text,prog->traveler);
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
