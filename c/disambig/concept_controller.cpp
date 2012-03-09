#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <chibi.h>
#include "concept_resolver.h"
#include <pillowtalk.h>

static ConceptResolver* __cr = NULL;

typedef vector<char*> SnippetList;

static int load_file_into_memory(const char* filename, char** result);
static void conceptualize(chibi handle, const chb_request req, chb_response res, void* vstar);
static void split_into_vector(SnippetList& vec, char* input);

extern "C" void chibi_init(chibi handle)
{
  printf("Init\n");
  chb_router_register(handle,"/conceptualize",conceptualize);

  char* aliases = NULL;
  char* neighbors = NULL;

  if (chb_prop_get(handle,"aliases")) {
    aliases = strdup(chb_prop_get(handle,"aliases"));
  }

  if (chb_prop_get(handle,"neighbors")) {
    neighbors = strdup(chb_prop_get(handle,"neighbors"));
  }

  const char* config_file = chb_prop_get(handle, "config");
  if (config_file) {
    char* config_contents = NULL;
    load_file_into_memory(config_file,&config_contents);
    if (!config_contents)
      chb_abort("Invalid configuration file");

    pt_node_t* config_json = pt_from_json(config_contents);
    if (config_json) {
      const char* log_facility = pt_string_get(pt_map_get(config_json, "log_facility"));
      if (log_facility) {
        /*
        __use_syslog = true;
        int int_facility = string_to_log_facility(log_facility);
        if (int_facility >= 0) 
          openlog("everywhere",0,int_facility);
        else
          chb_abort("Invalid Log Facilty Specified");
          */
      }

      if (!aliases) {
        const char* conf_aliases = pt_string_get(pt_map_get(config_json,"aliases"));
        if (!conf_aliases)
          chb_abort("Please specify an aliases in the config");
        aliases = strdup(conf_aliases);
      }

      if (!neighbors) {
        const char* conf_neighbors = pt_string_get(pt_map_get(config_json,"neighbors"));
        if (!conf_neighbors)
          chb_abort("Please specify a neighbors in the config");
        neighbors = strdup(conf_neighbors);
      }

      pt_free_node(config_json);
      free(config_contents);
    }
  }

  if (!aliases || !neighbors) {
    chb_abort("You must specify both a neighbors and aliases config");
  }

  __cr = new ConceptResolver(aliases, neighbors);

}

static void conceptualize(chibi handle, const chb_request req, chb_response res, void* vstar)
{
  printf("Conceptualize\n");
  const char* cand = chb_rq_param_get(req,"candidates");

  char* cand_copy = NULL;

  if (!cand) 
    cand_copy = strdup("");
  else
    cand_copy = strdup(cand);

  SnippetList candidates;
  split_into_vector(candidates,cand_copy);

  ResolvedConceptList concepts;
  __cr->resolve(candidates,concepts);

  pt_node_t* result = pt_array_new();
  for(ResolvedConceptList::const_iterator ii = concepts.begin(); ii != concepts.end(); ii++) {
    resolved_concept_t conc = *ii;
    pt_node_t* entry = pt_map_new();
    pt_map_set(entry,"text_rep", pt_string_new(conc.text_rep));
    pt_map_set(entry,"concept", pt_string_new(conc.canonical));
    pt_map_set(entry,"score", pt_integer_new(conc.score));
    cout << "Concept:" << conc.text_rep << ":" << conc.canonical << ":" << conc.score <<  endl;
    pt_array_push_back(result,entry);
  }

  pt_node_t* payload = pt_map_new();
  pt_map_set(payload,"payload",result);

  char* json_str = pt_to_json(payload,0);
  pt_free_node(payload);

  chb_rs_header_set(res,"Content-Type","application/json");
  chb_rs_set(res,json_str);
}

/*
 * Split pipe delineated input into a set
 */   
static void split_into_vector(SnippetList& vec, char* input)
{
  if (input) {
    int len = strlen(input);
    char* snippet = (char*) input;
    for(int i=0; i < len + 1; i++) {
      if (input[i] == '|' || input[i] == '\0') {
        input[i] = '\0';
        vec.push_back(snippet);
        snippet = (char*) input + i + 1;
      }
    }
  }
}

/*
 * This function will load a file into memory using stdio.
 */
static int load_file_into_memory(const char* filename, char** result)
{
  uint64_t size = 0;
  FILE* f = fopen(filename,"rb");
  if (f == NULL) {
    *result = NULL;
    return -1;
  }

  fseek(f,0,SEEK_END);
  size = ftell(f);
  fseek(f,0,SEEK_SET);

  *result = (char*) malloc(size + 1);
  if (size != fread(*result,sizeof(char),size,f)) {
    free(*result);
    return -2;
  }
  fclose(f);
  (*result)[size] = '\0';
  return size;
}
