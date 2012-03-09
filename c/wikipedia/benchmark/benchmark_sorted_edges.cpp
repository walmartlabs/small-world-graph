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

int main(int argc,char** argv)
{
  if (argc < 2) {
    cout << "Please specify an index root" << endl;
    exit(0);
  }

  char* index_root = argv[1];
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  man->load_from_index(index_root);

  for(;;) {
    char input[1024];
    cout << "Type in a tuple:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string first = input;

    WikipediaPage* page = man->page(input);
    if (page) {
      cout << "Page: " << first << " -> (" << page->id() << ",'" << page->title() << "')" << endl;
      WikipediaEdgeInfoList* outlinks = page->outlinks();
      cout << "=== Outlinks: " << outlinks->size() << endl;
      for(WikipediaEdgeInfoList::const_iterator ii = outlinks->begin(); ii != outlinks->end(); ii++) {
        WikipediaEdgeInfo edge = *ii;
        cout << "-- ('" << edge.end->title() << "'," << (int) edge.count << "," << edge.in_text << ")" << endl; 
      }

      WikipediaEdgeInfoList* inlinks = page->inlinks();
      cout << "=== Inlinks: " << inlinks->size() << endl;
      for(WikipediaEdgeInfoList::const_iterator ii = inlinks->begin(); ii != inlinks->end(); ii++) {
        WikipediaEdgeInfo edge = *ii;
        cout << "-- ('" << edge.begin->title() << "'," << (int) edge.count << "," << edge.in_text << ")" << endl; 
      }

      cout << "=== Sorted Edges: " << endl;
      page->print_sorted_edges();

      cout << "=== Neighbors: " << endl;
      page->print_neighbors(30);

      cout << "=== Bidi Edges: " << endl;
      page->print_bidi_edges();

    }
  }

  return 0;
}
