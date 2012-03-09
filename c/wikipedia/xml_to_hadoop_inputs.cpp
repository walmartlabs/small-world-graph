/* 
 * File: xml_to_hadoop_inputs.cpp
 * Author: Curtis Spencer
 * ---
 * This executable is used to take a wikipedia xml dump file
 * and output links and tranformed text output
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "wikipedia_xml_parser.h"
#include "wiki_parser.h"
#include "wiki_title_validator.h"

/* Prototypes */

/* Globals */
static size_t __total_pages = 0;
static FILE* __link_fd;
static FILE* __text_fd;

/* 
 * This method checks to see if the title is valid, then parses the links out,
 * then transforms the wiki text to human readable, then stores it into the
 * respective outfiles
 */
static void parse_and_output(char* title, char* text, void* traveler) {
  if (title && text && useful_title(title,title + strlen(title))) {
    __total_pages++;
    wiki_parse_tree_t* tree = wiki_parse(text);
    if (tree) {
      for(size_t i=0; i < tree->outlinks.len; i++) {
        link_t* link = tree->outlinks.links[i];
        fprintf(__link_fd,"%s\n",link->name);
      }
      free_parse_tree(tree);
    }
    /*
    char* transformed = wikitext_to_plaintext(text);
    if (transformed) {
      for(size_t j=0; j < strlen(transformed); j++) {
        if (transformed[j] == '\n')
          transformed[j] = ' ';
      }
      fprintf(__text_fd,"%s\n",transformed);
      free(transformed);
    }
    */
    if (__total_pages% 1000 == 0) {
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
  char* link_file= NULL;
  char* text_file= NULL;

  const char* help = "Use the following options to specify behavior\n\
  -i : input wikipedia xml file\n\
  -l : link flat file\n\
  -t : text flat file\n\
  -h : this message :-)";

  while ((arg = getopt(argc, argv, "i:l:t:h")) != -1) {
    switch (arg) {
      case 'i':
        wikipedia_xml = optarg;
        break;
      case 'l':
        link_file = optarg;
        break;
      case 't':
        text_file= optarg;
        break;
      case 'h':
        printf("%s\n",help);
        break;
    }
  }

  if (!wikipedia_xml || !link_file || !text_file) {
    printf("%s\n",help);
    exit(-1);
  }

  __link_fd = fopen(link_file,"w");
  if (!__link_fd) {
    printf("Error opening link file\n");
    exit(-1);
  }
  __text_fd = fopen(text_file,"w");
  if (!__text_fd) {
    printf("Error opening text file\n");
    exit(-1);
  }

  wikipedia_xml_parse(wikipedia_xml,parse_and_output,NULL);

  fclose(__link_fd);
  fclose(__text_fd);

  fprintf(stdout,"\rPages Parsed: %9d\n",(int) __total_pages);
  fflush(stdout);
}
