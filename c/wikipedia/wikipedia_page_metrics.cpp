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

int main(int argc,char** argv)
{
  if (argc < 3) {
    cout << "Please specify an outlinks tokyo cabinet file and a page key" << endl;
    exit(0);
  }

  char* index_root = argv[1];
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  man->load_from_tokyo_cabinet(index_root);

  cout << "a\ta_inlinks\ta_outlinks\tb\tb_inlinks\tb_outlinks\tcommon_count\tbidi\tdistance" << endl;
  for(int i=2; i < argc; i++) {
    fprintf(stderr,"Searching for %s\n", argv[i]);
    WikipediaPage* page = man->page(argv[i]);
    if (page)
      page->print_sorted_edge_statistics();
  }
}
