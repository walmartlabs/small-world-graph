/* 
 * File: tc_parsed_importer.cpp
 * Author: Curtis Spencer
 * ---
 * This executable is used to take a wikipedia xml dump file, parse all the
 * pages and dump them to a Tokyo Cabinet Hash DB.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <tcutil.h>
#include <tchdb.h>
#include "wikipedia_xml_parser.h"
#include "wiki_parser.h"
#include "wiki_title_validator.h"

/* Prototypes */
static void initialize_tc(const char* filename);

/* Globals */
static size_t __total_pages = 0;
static TCHDB* __hdb;


static void initialize_tc(const char* filename) 
{
  int ecode;
  __hdb = tchdbnew();
  if(!tchdbtune(__hdb,10000000,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
  }

  if (!tchdbsetxmsiz(__hdb,12710886400LL)) {
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "setxmsiz error: %s\n", tchdberrmsg(ecode));
  }

  if(!tchdbopen(__hdb, filename, HDBOWRITER | HDBOCREAT)){
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
  }


}

/* Close the database and free the handle */
static void finalize_tc() 
{
  int ecode;
  if(!tchdbclose(__hdb)){
    ecode = tchdbecode(__hdb);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
  }
  tchdbdel(__hdb);
}

/* 
 * This method checks to see if the title is valid, then transforms the wiki
 * text to human readable, then stores it into Tokyo Cabinet with the title as
 * the key
 */
static void parse_and_store_page(char* title, char* text, void* traveler) {
  int ecode;
  if (title && text && useful_title(title,title + strlen(title))) {
    __total_pages++;
    char* transformed = wikitext_to_plaintext(text);
    if (transformed) {
      // Put it into Tokyo Cabinet
      if (!tchdbput2(__hdb,title,transformed)) {
        ecode = tchdbecode(__hdb);
        fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
      }
    
      free(transformed);
    }
    if (__total_pages % 1000 == 0) {
      fprintf(stdout,"\rPages Stored: %9d",(int) __total_pages);
      fflush(stdout);
    }
  }  
  free(title);
  free(text);
}

int main(int argc,char** argv)
{
  int arg;
  char* wikipedia_xml = NULL;
  char* tokyo_cabinet_file = NULL;

  const char* help = "Use the following options to specify behavior\n\
  -i : input wikipedia xml file\n\
  -t : tokyo cabinet output db file\n\
  -h : this message :-)";

  while ((arg = getopt(argc, argv, "i:t:h")) != -1) {
    switch (arg) {
      case 'i':
        wikipedia_xml = optarg;
        break;
      case 't':
        tokyo_cabinet_file = optarg;
        break;
      case 'h':
        printf("%s\n",help);
        break;
    }
  }

  if (!wikipedia_xml || !tokyo_cabinet_file) {
    printf("%s\n",help);
    exit(-1);
  }

  initialize_tc(tokyo_cabinet_file);

  wikipedia_xml_parse(wikipedia_xml,parse_and_store_page,NULL);

  fprintf(stdout,"\rPages Stored: %9d\n",(int) __total_pages);
  fflush(stdout);

  finalize_tc();

}
