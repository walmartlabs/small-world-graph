/* Support Large File */
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <list>

#include "benchmark.h"
#include "wikipedia_graph.h"
#include "wikipedia_neighbor_set.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

/* Global Variables */

#include "wikipedia_types.h"
static WikipediaGraph* __graph;
static WikipediaNeighborSet* __neighbors;

static void do_page() 
{
  char input[1024];
  cout << "Type in a page title: ";
  cin.getline(input,1024);
  if (cin.eof()) {
    cout << endl;
    return;
  }

  page_t* page = __graph->page(input);
  if (page) {
    cout << "Wrote " << page->title << " info to /tmp/wiki_output" << endl;
    ofstream outfile("/tmp/wiki_output");
    outfile << "Result for Query " << input << "\nPage: " << page->title << "\nOutlinks: " << page->outlinks_size << " Inlinks: " << page->inlinks_size << endl;
    outfile << "---Outlinks: " << endl;
    for(int i=0; i < page->outlinks_size; i++) {
      outfile << page->outlinks[i]->title << endl;
    }

    outfile << "---Inlinks: " << endl;
    for(int i=0; i < page->inlinks_size; i++) {
      outfile << page->inlinks[i]->title << endl;
    }

    outfile.close();
  } else {
    cout << "Couldn't find page '" << input << "'" << endl;
  }
}

static void do_distances()
{
  char input[1024];

  cout << "Type in a comma delinated set of tuples: ";
  cin.getline(input,1024);
  if (cin.eof()) {
    cout << endl;
    return;
  }

  vector<string> split_input;
  split(split_input,input,is_any_of(","));

  RelationshipList rl;
  __neighbors->distances(rl,split_input);
  if (rl.size() == 0) {
    cout << "Couldn't find distances for " << input << endl;
    return;
  }

  cout << "Wrote " << rl.size() << " distances to outfile" << endl;
  ofstream outfile("/tmp/wiki_output");
  for(RelationshipList::const_iterator ii = rl.begin(); ii != rl.end(); ii++) {
    relationship_t di = *ii;
    if (di.first.size() > 0)
      outfile << di.first << " | " << di.second << " | " << (int) di.distance << endl;
  }
  outfile.close();
}

static void do_save_neighbors()
{
  char input[1024];
  string output_file;
  uint32_t limit = 0;

  for(;;) {
    cout << "Type in an outfile: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      return;
    }
    output_file = input;
    if (output_file.size() > 0)
      break;
  }


  for(;;) {
    cout << "Type in a neighbor limit: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      return;
    }
    limit = atoi(input);
    if (limit != 0)
      break;
  }
}

static void do_load_neighbors()
{
  char input[1024];
  string input_file;
  uint32_t limit = 0;

  for(;;) {
    cout << "Type in an input file: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      return;
    }
    input_file = input;
    if (input_file.size() > 0)
      break;
  }

  for(;;) {
    cout << "Type in a neighbor limit: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      return;
    }
    limit = atoi(input);
    if (limit != 0)
      break;
  }

  __neighbors->load(input_file.c_str());
}

int main(int argc,char** argv)
{
  __graph = WikipediaGraph::instance();
  __neighbors = WikipediaNeighborSet::instance();

  char input[256];
  for(;;) {
    cout << "What operation do you want to do\n0: Page Info\n1: Tuple Distances\n2: Save Neighbors\n3: Load Neighbors" << endl;
    cout << "Type in a number: ";
    cin.getline(input,256);
    if (cin.eof()) {
      cout << endl;
      break;
    }
    int choice = atoi(input);
    switch(choice) {
      case 0:
        do_page();
        break;
      case 1:
        do_distances();
        break;
      case 2:
        do_save_neighbors();
        break;
      case 3:
        do_load_neighbors();
        break;
    }

  }

  return 0;
}
