/* Support Large File */
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <expat.h>
#include <string>

#include "wikipedia_page.h"

#include <google/sparse_hash_map>
#include "paul_hsieh_hash.h"
#include "equality.h"
#include <getopt.h>

using namespace std;

/*
 * Look at the mmap size string and turn it into a uint64_t
 */
static uint64_t parse_mem_size(char* mem_size)
{
  uint64_t final_size = 0;
  unsigned int size;
  char unit;
  sscanf(mem_size,"%d%c", &size, &unit);
  if (unit == 'M') {
    final_size = size * 1024 * 1024;
  } else if (unit == 'G') {
    final_size = ((uint64_t)size) * 1024LL * 1024LL * 1024LL;
  } else
    final_size = size;
  
  return final_size;
}

int main(int argc,char** argv)
{
  char* type = NULL;
  char* input = NULL;
  char* output = NULL;
  char* output_type = NULL;
  int num_neighbors = 0;
  int num_edges = 0;
  bool output_sparse_graph = false;
  bool output_distances = false;
  bool output_bidi = false;
  bool output_topic_priority_list = false;
  bool output_link_counts = false;
  uint64_t mmap_size = 67000000;
  int c;
  
  string help = "Use the following options to specify behavior\n\
  -t : input type (xml|index|tokyo)\n\
  -y : output type (index|tokyo)\n\
  -i : input file or input index_root \n\
  -o : output index root\n\
  -n : number of neighbors to output per page\n\
  -e : number of edges/page to use in sparse graph\n\
  -s : output sparse graph\n\
  -d : output distances\n\
  -b : output bidi graph\n\
  -p : output topic priority list\n\
  -c : output link counts \n\
  -m : memory to give to the outputter\n\
  -h : this message :-)";

  while ((c = getopt(argc, argv, "t:hn:i:o:sde:bpcy:m:")) != -1) {
    switch (c) {
      case 't':
        type = optarg;
        break;
      case 'y':
        output_type = optarg;
        break;
      case 'i':
        input = optarg;
        break;
      case 'o':
        output = optarg;
        break;
      case 'n':
        num_neighbors = atoi(optarg);
        break;
      case 'e':
        num_edges = atoi(optarg);
        break;
      case 'h':
        cout << help << endl;
        exit(-1);
        break;
      case 's':
        output_sparse_graph = true;
        break;
      case 'd':
        output_distances = true;
        break;
      case 'b':
        output_bidi = true;
        break;
      case 'p':
        output_topic_priority_list = true;
        break;
      case 'm':
        mmap_size = parse_mem_size(optarg);
        break;
      case 'c':
        output_link_counts = true;
        break;
    }
  }

  // Now validate the arguments;
  if (!type || !(!strcmp("xml",type) || !strcmp("index",type) || !strcmp("tokyo",type))) {
    cout << "Invalid Value for -t" << endl << help << endl;
    exit(-1);
  }

  if (!output_type || !(!strcmp("index",output_type) || !strcmp("tokyo",output_type))) {
    cout << "Invalid Value for -y: (index,tokyo) are valid" << endl << help << endl;
    exit(-1);
  }

  if (!input) {
    cout << "Option -i cannot be empty" << endl << help << endl;
    exit(-1);
  }

  if (!output) {
    cout << "Option -o cannot be empty" << endl << help << endl;
    exit(-1);
  }

  if (output_distances && !num_neighbors) {
    cout << "Invalid Value for -n" << endl << help << endl;
    exit(-1);
  }

  if (output_sparse_graph && !num_edges) {
    cout << "Invalid Value for -e" << endl << help << endl;
    exit(-1);
  }

  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  if (!strcmp(type,"xml")) {
    man->load(input);
    cout << "\nPage Nodes Generated: " << man->size() << endl;
    cout << "Images Found: " << WikipediaImage::Manager::instance()->size() << endl;
    man->to_index(output);
    //man->core_dump("/tmp/full_dump");
  } else if (!strcmp(type,"tokyo")) {
    man->load_from_tokyo_cabinet(input);
  } else {
    man->load_from_index(input);
    //man->to_index(output);
    //man->core_dump("/tmp/incremental_dump");
  }

  if (output_bidi)
    man->to_bidi_graph(output);

  if (output_sparse_graph) {
    if (output_type && !strcmp(output_type,"tokyo")) {
      man->to_sparse_graph_tokyo_cabinet_dictionary(output,mmap_size);
    } else {
      man->to_sparse_graph(output,num_edges);
    }
  }

  if (output_distances) 
    man->to_distances(output,num_neighbors);

  if (output_topic_priority_list)
    man->to_priority_topic_list(output);

  if (output_link_counts)
    man->to_link_counts(output);



  return 0;
}
