#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <tcutil.h>
#include <tchdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

#include "wikipedia_xml_parser.h"
#include "wikipedia_sequential_parser.h"
#include "sequential_tree_string_outputter.h"
#include "sequential_tree_json_outputter.h"

using namespace std;

//#define TC_MEMORY_SIZE 12710886400LL
#define TC_NUM_BUCKETS 10000000L
#define TC_MEMORY_SIZE 167108864L

typedef struct {
  bool print;
  bool verbose;
  bool json;
  bool xml;
  char* error_output_filename;
  FILE* error_output_file;
  char* output_db;
} parser_config_t;

/* Prototypes */
static void initialize_tc(const char* filename);
static void echo_parse_tree(char* content, bool verbose);
static void parse_to_seq_tree(char* title, char* text, void* traveler);
static void finalize_tc();
static int read_file_into_memory(const char* filename, char** result);

/* Globals */
static size_t __total_pages = 0;
static size_t __parse_error_count = 0;
static TCHDB* __hdb;

int main(int argc,char** argv)
{
  int c = 0;
  parser_config_t config = {0};


  string help = "Use the following options to specify behavior\n\
  -t : tokyo cabinet output db\n\
  -x : input is xml\n\
  -j : use JSON format in verbose output\n\
  -p : print titles of the pages as parsed\n\
  -e : parse error output file\n\
  -v : verbose output (a lot of output!)\n\
  -h : this message :-)";

  while ((c = getopt(argc, argv, "jpt:vhe:x")) != -1) {
    switch(c) {
      case 'j':
        config.json = true;
        break;
      case 'p':
        config.print = true;
        break;
      case 'x':
        config.xml = true;
        break;
      case 't':
        config.output_db = optarg;
        break;
      case 'v':
        config.verbose = true;
        break;
      case 'e':
        config.error_output_filename = optarg;
        break;
      case 'h':
        cout << help << endl;
        exit(-1);
        break;
      default:
        break;
    }
  }

  stringstream content;
  string line;
  string content_str = content.str();
  bool has_stdin = false;
  if (content_str != "") 
    has_stdin = true;

  if (!has_stdin) {
    if (argc <= optind) {
      printf("Please specify a filename to parse");
      exit(-2);
    }
  } 

  if (config.error_output_filename) {
    config.error_output_file = fopen(config.error_output_filename,"w");
    if (!config.error_output_filename) {
      fprintf(stderr,"Unable to open the error output file\n");
      exit(-1);
    }
  }

  // Use this to allow for remote debugging.
  //printf("Press ENTER to continue\n");
  //char res[1024];
  //char* result = fgets(res,1024,stdin);

  if (config.output_db) {
    initialize_tc(config.output_db);
  }

  if (!has_stdin && config.xml)
    wikipedia_xml_parse(argv[optind],parse_to_seq_tree,&config);
  else if (!has_stdin) {
    char* content_cstr = NULL;
    int size = 0;
    size = read_file_into_memory(argv[optind],&content_cstr);
    echo_parse_tree((char*) content_cstr, config.verbose);
  } else {
    echo_parse_tree((char*)content_str.c_str(), config.verbose);
  }

  if (config.output_db) {
    finalize_tc();
  }

  if (config.error_output_file)
    fclose(config.error_output_file);

  return 0;
}

/*****************************
 * Private Implementation
 *****************************/


/*
 * Skip any of the pages that start with Template: for now
 */
static void parse_to_seq_tree(char* title, char* text, void* traveler)
{
  parser_config_t* cfg = (parser_config_t*) traveler;
  if (title && text) {
    if (strstr(title,"Template:") != title) {
      wiki_seq_tree_t* tree = wiki_seq_parse(text);
      if (cfg->print) {
        if (tree->error) {
          printf("Failure: %s:%s\n", title, tree->error_str);
        } else {
          printf("Success: %s\n", title);
        }
      }

      if (cfg->verbose) {
        char* string_rep = NULL;
        if (cfg->json) {
          string_rep = wiki_seq_tree_to_json(tree,1);
        } else {
          string_rep = wiki_seq_tree_to_string(tree);
        }
        printf("%s\n",string_rep);
        free(string_rep);
      }

      if (cfg->output_db) {
        if (tree->error) {
          __parse_error_count++;
          if (cfg->error_output_file) {
            fprintf(cfg->error_output_file,"%s\n",title);
            fflush(cfg->error_output_file);
          }
        } else {
          char* string_rep = wiki_seq_tree_to_json(tree,0);
          if (!tchdbput2(__hdb,title,string_rep)) {
            int ecode = tchdbecode(__hdb);
            fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
          }
          free(string_rep);
        } 
      }

      free_wiki_seq_tree(tree);
    }
    __total_pages++;
    if (__total_pages % 1000 == 0) {
      fprintf(stdout,"\rPages Stored: %9d Parse Errors: %9d",(int) __total_pages, (int) __parse_error_count);
      fflush(stdout);
    }
  }
  free(title);
  free(text);
}

static void echo_parse_tree(char* content, bool verbose) {
  wiki_seq_tree_t* tree = wiki_seq_parse(content,verbose);
  char* string_rep = wiki_seq_tree_to_json(tree,1);
  printf(string_rep);
  free(string_rep);
  free_wiki_seq_tree(tree);
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

static int read_file_into_memory(const char* filename, char** result)
{
  unsigned int size = 0;
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    *result = NULL;
    return -1; // -1 means file opening fail
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  *result = (char *)malloc(size+1);
  if (size != fread(*result, sizeof(char), size, f)) {
    free(*result);
    return -2; // -2 means file reading fail
  }
  fclose(f);
  (*result)[size] = 0;
  return size;
}
