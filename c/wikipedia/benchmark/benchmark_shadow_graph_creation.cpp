#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "wikipedia_sparse_graph.h"
#include "benchmark.h"
#include <valgrind/callgrind.h>

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  WikipediaSparseGraph* graph = WikipediaSparseGraph::instance();
  if (argc < 2) {
    cout << "Specify an index root" << endl;
    exit(0);
  }
  graph->load(argv[1]);

  vector<ChainLink> chain;
  vector<string> first_set,second_set;

  bench_start("Chain Suite");

  cout << "== Shortest Chain with User == (teddybear) && (Steve Jobs)" << endl;
  first_set.push_back("Steve Jobs");
  for(int i=0; i < 1; i++) {
    graph->shortest_chain_with_user(chain,"teddybear",first_set,BucketQueue::MaxLinks, false);
    for(vector<ChainLink>::const_iterator ii = chain.begin(), end = chain.end(); ii != end; ii++) {
      cout << (*ii).title << "|" << (*ii).category << endl;
    }
    chain.clear();
  }

  cout << "== Shortest Chain with User == (teddybear) && (Justin Timberlake)" << endl;
  first_set.clear();
  first_set.push_back("Justin Timberlake");
  for(int i=0; i < 1; i++) {
    graph->shortest_chain_with_user(chain,"teddybear",first_set,BucketQueue::MaxLinks, false);
    for(vector<ChainLink>::const_iterator ii = chain.begin(), end = chain.end(); ii != end; ii++) {
      cout << (*ii).title << "|" << (*ii).category << endl;
    }
    chain.clear();
  }

  bench_finish("Chain Suite");
}
