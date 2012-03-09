#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "wikipedia_graph.h"
#include "benchmark.h"

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  WikipediaGraph* graph = WikipediaGraph::instance();
  if (argc < 2) {
    cout << "Specify an index root" << endl;
    exit(0);
  }
  graph->load(argv[1]);

  for(;;) {
    char input[1024];
    cout << "Type in a pipe delinated set of 2 tuples: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string input_value = input;

    vector<string> split_input;

    split(split_input,input_value,is_any_of("|"));

    cout << "Input Size:" << split_input.size() << endl;


    page_t* page = graph->page(split_input[0].c_str());
    if (page) {
      cout << "Page 1:" << page->title << endl;

      cout << "=== Outlinks ===" << endl;
      for(int i=0; i < page->outlinks_size; i++) {
        cout << "--> '" << page->outlinks[i]->title << "'" << endl;
      }
    }

    if (split_input.size() > 1) {
      page = graph->page(split_input[1].c_str());
      if (page) {
        cout << "Page 2:" << page->title << endl;

        cout << "=== Outlinks ===" << endl;
        for(int i=0; i < page->outlinks_size; i++) {
          cout << "--> '" << page->outlinks[i]->title << "'" << endl;
        }
      }

      vector<string> chain;
      vector<string> referring_pages;
      graph->page_chain(chain,referring_pages,split_input[0],split_input[1]);

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
}
