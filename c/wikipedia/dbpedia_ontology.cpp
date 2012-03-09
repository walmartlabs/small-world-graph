#define _FILE_OFFSET_BITS 64

#include "dbpedia_ontology.h"
#include <stdlib.h>
#include <iostream>
#include <raptor.h>
#include <string.h>
#include <map>  
#include <list>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

/* Prototypes */
void triples_handler(void* user_data, const raptor_statement* triple);
char* pull_title_from_resource(const char* resource);
char* pull_title_from_ontology(const char* ontology);


DbpediaOntology::DbpediaOntology(const char* filename)
{
  raptor_init();

  raptor_parser* rdf_parser=raptor_new_parser("ntriples");

  raptor_set_statement_handler(rdf_parser,&entities_, triples_handler);

  unsigned char * uri_string=raptor_uri_filename_to_uri_string(filename);
  raptor_uri* uri=raptor_new_uri(uri_string);
  raptor_uri* base_uri=raptor_uri_copy(uri);

  raptor_parse_file(rdf_parser, uri, base_uri);

  raptor_free_parser(rdf_parser);
  raptor_free_uri(base_uri);
  raptor_free_uri(uri);
  raptor_free_memory(uri_string);
  
  raptor_finish();
}

bool
DbpediaOntology::is_entity(const char* title)
{
  EntityTitleSet::const_iterator ii = entities_.find((char*)title);
  return (ii != entities_.end());
}

/* Static Implementation */

/*
 * <http://dbpedia.org/resource/Hello_World> -> Hello World
 */
char* pull_title_from_resource(const char* resource)
{
  int resource_len = strlen(resource);
  // len of resource prefix
  int title_len = resource_len - 29 - 1; 
  char* title = (char*) malloc(sizeof(char) * (title_len + 1));
  strncpy(title,resource + 29,title_len);
  title[title_len] = '\0';
  return title;
}

/*
 * <http://dbpedia.org/ontology/SkiArea> -> SkiArea
 */
char* pull_title_from_ontology(const char* resource)
{
  int resource_len = strlen(resource);
  // len of resource prefix
  int title_len = resource_len - 29 - 1; 
  char* title = (char*) malloc(sizeof(char) * (title_len + 1));
  strncpy(title,resource + 29,title_len);
  title[title_len] = '\0';
  return title;
}


void
triples_handler(void* user_data, const raptor_statement* triple) 
{
  EntityTitleSet* entities = (EntityTitleSet*) user_data;
  unsigned char* subj = raptor_statement_part_as_string(triple->subject,triple->subject_type,NULL,NULL);
  if (subj) {
    char* core_subject = pull_title_from_resource((char*) subj);
    if (core_subject)
      entities->insert(core_subject);
  }
}
