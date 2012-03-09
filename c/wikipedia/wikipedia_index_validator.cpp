#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "wikipedia_neighbor_set.h"
#include "benchmark.h"

using namespace std;
using namespace boost;

static void verify_relationships(const string& title,const bool ambiguous, const vector<relationship_t>& rels)
{
  for(vector<relationship_t>::const_iterator ii = rels.begin(); ii != rels.end(); ii++) {
    if (ii->first == ii->second) {
      cout << "Error: '" << ii->first << "' is related to itself." << endl;
    }
  }
}

int main(int argc, char** argv) {
  WikipediaNeighborSet* neighbor_set= WikipediaNeighborSet::instance();
  neighbor_set->load(argv[1]);
  neighbor_set->map(verify_relationships);
}
