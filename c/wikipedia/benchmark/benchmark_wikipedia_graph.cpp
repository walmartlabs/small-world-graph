#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "wikipedia_graph.h"
#include "benchmark.h"
#include <valgrind/callgrind.h>

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  WikipediaGraph* graph = WikipediaGraph::instance();
  if (argc < 2) {
    cout << "Specify an index root" << endl;
    exit(0);
  }
  graph->load(argv[1]);

  vector<string> chain;
  vector<string> referring_pages;

  for(int i=0; i < 10; i++) {
    CALLGRIND_START_INSTRUMENTATION;
    graph->page_chain(chain,referring_pages,"Obama","Yahoo");
    CALLGRIND_STOP_INSTRUMENTATION;
    cout << "== Chain ==" << endl;
    for(vector<string>::const_iterator ii = chain.begin(), end = chain.end(); ii != end; ii++) {
      cout << *ii << endl;
    }

    cout << "== Referring Pages ==" << endl;
    for(vector<string>::const_iterator ii = referring_pages.begin(), end = referring_pages.end(); ii != end; ii++) {
      cout << *ii << endl;
    }
  }
}
