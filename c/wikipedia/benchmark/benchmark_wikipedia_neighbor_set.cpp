#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "wikipedia_neighbor_set.h"
#include "benchmark.h"

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  bench_start("Wiki Client");
  WikipediaNeighborSet* neighbor_set= WikipediaNeighborSet::instance();
  neighbor_set->load(argv[1]);

  for(;;) {
    char input[1024];
    cout << "Type in a pipe delinated set of tuples: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string input_value = input;

    vector<string> split_input;

    split(split_input,input_value,is_any_of("|"));

    cout << "Input Size:" << split_input.size() << endl;

    RelationshipList rl;
    neighbor_set->distances(rl,split_input);
    if (rl.size() == 0) {
      cout << "Couldn't find distances for " << input << endl;
      continue;
    }

    for(RelationshipList::const_iterator ii = rl.begin(); ii != rl.end(); ii++) {
      relationship_t di = *ii;
      if (di.first.size() > 0)
        cout << di.first << " | " << di.second << " | " << (int) di.distance << endl;
    }
  }

  bench_finish("Wiki Client");
}
