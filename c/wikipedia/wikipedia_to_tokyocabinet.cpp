#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <tcutil.h>
#include <tchdb.h>
#include <iostream>
#include <string>

#include "wikipedia_xml_parser.h"

#define TC_NUM_BUCKETS 10000000L
#define TC_MEMORY_SIZE 167108864L

// Prototypes
static void write_to_tokyocabinet(char* title, char* text, void* traveler);
static void initialize_tc(const char* filename);
static void finalize_tc();

// Globals
static TCHDB* __hdb;

int main(int argc,char** argv)
{
  if (argc < 3) {
    printf("Please specify a wikipedia xml file and a tokyo cabinet output file\n");
    exit(-1);
  }

  if (argv[2]) {
    initialize_tc(argv[2]);
  }

  wikipedia_xml_parse(argv[1],write_to_tokyocabinet,NULL);

  if (argv[2]) {
    finalize_tc();
  }

  return 0;
}

//------------------------------------------------
//-- P r i v a t e  I m p l e m e n t a t i o n --
//------------------------------------------------

/*
 * Take in the xml text and write it out to the Tokyo Cabinet database
 */
static void write_to_tokyocabinet(char* title, char* text, void* traveler)
{
  if (title) {
    if (text)
      tchdbput2(__hdb,title,text);
    else
      tchdbput2(__hdb,title,"");
  }
}

static void initialize_tc(const char* filename) 
{
  int ecode;
  __hdb = tchdbnew();
  if(!tchdbtune(__hdb,TC_NUM_BUCKETS,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if (!tchdbsetxmsiz(__hdb,TC_MEMORY_SIZE)) {
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "setxmsiz error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if(!tchdbopen(__hdb, filename, HDBOWRITER | HDBOCREAT)){
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }
}

/* Close the database and free the handle */
static void finalize_tc() 
{
  int ecode;
  if(!tchdbclose(__hdb)){
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }
  tchdbdel(__hdb);
}
