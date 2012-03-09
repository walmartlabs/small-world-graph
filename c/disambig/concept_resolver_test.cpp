#include "concept_resolver.h"
#include <iostream>
#include <pillowtalk.h>
#include <vector>
#include "benchmark.h"

using namespace std;

static int load_file_into_memory(const char* filename, char** result);
static void tag_input_file(ConceptResolver* cr,const char* path);

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("Specify a aliases_tch, a neighbors_tch, (optionally) a number of times to repeat, and (optionally) files containing a json array of tag candidates\n");
    exit(-1);
  }

  char* aliases_tch = argv[1];
  char* neighbors_tch = argv[2];
  int n_repeats = 1;
  if (argc > 3)
    n_repeats = atoi(argv[3]);

  ConceptResolver* cr = new ConceptResolver(aliases_tch, neighbors_tch);
  
  if (argc > 4) {
    bench_start("resolves");
    for (int j = 0; j < n_repeats; j++) {
      for (int i = 3; i < argc; i++)
        tag_input_file(cr,argv[i]);
    }
    bench_finish("resolves");
  } else {
    while(1) {
      string input_line;
      cout << "Enter a json file: ";
      getline(cin,input_line);
      if (input_line == "") 
        break;
      tag_input_file(cr,input_line.c_str());
    }
  }

  delete cr;
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

static void tag_input_file(ConceptResolver* cr,const char* path)
{
  char* input_contents;
  int res;

  res = load_file_into_memory(path,&input_contents);
  if (res < 0) {
    printf("Please specify a valid input file\n");
    return;
  }

  vector<char*> input;
  pt_node_t* tag_candidates = pt_from_json(input_contents);
  if (tag_candidates && pt_array_len(tag_candidates) > 0) {
    pt_iterator_t* it = pt_iterator(tag_candidates);
    pt_node_t* cur = NULL;
    while((cur = pt_iterator_next(it,NULL)) != NULL) {
      const char* tc = pt_string_get(cur);
      input.push_back(strdup(tc));
    }
    free(it);
    pt_free_node(tag_candidates);
  } else {
    printf("Please specify an array of json tag candidates inside the tag content file\n");
    free(input_contents);
    return;
  }

  vector<resolved_concept_t> resolved_concepts;
  cr->resolve(input,resolved_concepts);

  for(vector<resolved_concept_t>::const_iterator ii = resolved_concepts.begin(); ii != resolved_concepts.end(); ii++) {
    resolved_concept_t conc = *ii;
    cout << "Concept:" << conc.text_rep << ":" << conc.canonical << ":" << conc.score <<  endl;
  }

  for(vector<char*>::const_iterator ii = input.begin(); ii != input.end(); ii++) {
    free(*ii);
  }

  free(input_contents);
}
