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

static void print_page(WikipediaPage* page) 
{ 
  if (page) {
    cout << "Page: " << page->title() << " -> (" << page->id() << ",'" << page->title() << "')" << endl;
    WikipediaEdgeInfoList* outlinks = page->outlinks();
    cout << "=== Outlinks: " << outlinks->size() << endl;
    for(WikipediaEdgeInfoList::const_iterator ii = outlinks->begin(); ii != outlinks->end(); ii++) {
      WikipediaEdgeInfo edge = *ii;
      cout << "-- ('" << edge.end->title() << "'," << (uint32_t) edge.count << "," << edge.in_text << ")" << endl; 
    }

    WikipediaEdgeInfoList* inlinks = page->inlinks();
    cout << "=== Inlinks: " << inlinks->size() << endl;
    for(WikipediaEdgeInfoList::const_iterator ii = inlinks->begin(); ii != inlinks->end(); ii++) {
      WikipediaEdgeInfo edge = *ii;
      cout << "-- ('" << edge.begin->title() << "'," << (uint32_t) edge.count << "," << edge.in_text << ")" << endl; 
    }
  } else {
    printf("NULL Page\n");
  }
}

int main(int argc,char** argv)
{
  if (argc < 2) {
    cout << "Please specify an index root" << endl;
    exit(0);
  }

  char* index_root = argv[1];
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  man->load_from_tokyo_cabinet(index_root);

  for(;;) {
    char input[1024];
    cout << "Type in a first page title:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string first = input;
    WikipediaPage* first_page = man->page(first);
    if (first_page) {
      print_page(first_page);
    } else {
      printf("Nothing found for %s\n", first.c_str());
      continue;
    }


    cout << "Type in a second page title:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string second = input;
    WikipediaPage* second_page = man->page(second);
    if (second_page)
      print_page(second_page);
    else {
      printf("Nothing found for %s\n", second.c_str());
      continue;
    }

    cout << "Press Enter to Compute:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    uint32_t distance = man->distance_between(first,second);
    printf("Distance between:%u\n", (unsigned int) distance);

    cout << "Press Enter to Compute All Sorted Edges:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    first_page->print_sorted_edges();
  }

  return 0;
}
