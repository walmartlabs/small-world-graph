#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include "dbpedia_ontology.h"
#include "benchmark.h"
#include <valgrind/callgrind.h>

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Specify an dbpedia ontology file" << endl;
    exit(0);
  }

  DbpediaOntology ontology(argv[1]);
}
