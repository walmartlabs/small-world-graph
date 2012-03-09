#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "sparse_graph.h"
#include "benchmark.h"

using namespace std;
using namespace boost;

static void output_page(page_t* page);


static void output_page_list(page_list_t* list)
{
  for(uint32_t i = 0; i < list->length; i++) {
    output_page(list->pages[i]);
  }
}
/*
 * Output all the edges and edge descriptors for a given node
 */
static void output_page(page_t* page) 
{
  cout << "Page: (" << page->id << ",'" << page->title << "')" << endl;
  cout << "=== Edges ===" << endl;
  for(size_t i=0; i < page->num_edges; i++) {
    edge_t* edge = page->edges[i];
    cout << "--> ('" << edge->end->title << "'," << (int) edge->min_weight << ")" << endl;
    for(size_t j=0; j < edge->num_descriptors; j++) {
      edge_descriptor_t* ed = edge->descriptors[j];
      cout << "----> ('" << ed->snippet << "'," << (int) ed->weight << ")" << endl;
    }
  }
}

static void output_edge(edge_t* edge) {
  char* snippet = NULL;
  if (edge->num_descriptors > 0) {
    snippet = edge->descriptors[0]->snippet;
  } else {
    snippet = strdup("");
  }
  cout << "Edge: (" << edge->begin->title << "," << edge->end->title << "," << (int) edge->min_weight << ",'" << snippet << "')" << endl;
}

int main(int argc, char** argv) {
  SparseGraph* graph = SparseGraph::instance();
  if (argc < 2) {
    cout << "Specify an index root" << endl;
    exit(0);
  }
  graph->load(argv[1]);

  for(;;) {
    char input[1024];
    cout << "Type in a first tuple set (| delineated):";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string first = input;

    page_list_t* page_list = graph->concepts(input);
    if (page_list) {
      output_page_list(page_list);
    } else {
      cout << "Page by title '" << input << "' not found" << endl;
    }

    cout << "Type in a second tuple set (| delineated):";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string second = input;
    page_list = graph->concepts(input);
    if (page_list) {
      output_page_list(page_list);
    } else {
      cout << "Page by title '" << input << "' not found" << endl;
    }


    cout << "Press Enter to Compute the Chain";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    vector<string> first_set;
    vector<string> second_set;

    first_set.push_back(first);
    second_set.push_back(second);


    Chain chain;

    const char* start = NULL;
    const char* end = NULL;

    graph->shortest_chain(chain,first_set,second_set,8,false,false, &start, &end);
    cout << "== Chain ==" << endl;
    int chain_length = 0;
    for(Chain::const_iterator ii = chain.begin(), end = chain.end(); ii != end; ii++) {
      edge_t* edge = *ii;
      chain_length += edge->min_weight;
      output_edge(edge);
    }
    cout << "Total Length:" << chain_length << endl;

  }
}
