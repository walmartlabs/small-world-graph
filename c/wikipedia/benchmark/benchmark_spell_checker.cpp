#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "wikipedia_spell_checker.h"
#include "benchmark.h"

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Specify an dic file" << endl;
    exit(0);
  }

  WikipediaSpellChecker* spell = WikipediaSpellChecker::instance();
  bench_start("Load");
  spell->load(argv[1]);
  bench_finish("Load");

  SuggestionList list;
  bench_start("Obima");
  spell->suggestions("Obima",list);
  bench_finish("Obima");

  cout << "Suggestions" << endl;
  for(SuggestionList::const_iterator ii = list.begin(); ii != list.end(); ii++) {
    cout << *ii << endl;
  }

  list.clear();

  bench_start("Appl");
  spell->suggestions("Appl",list);
  bench_finish("Appl");

  cout << "Suggestions" << endl;
  for(SuggestionList::const_iterator ii = list.begin(); ii != list.end(); ii++) {
    cout << *ii << endl;
  }
}
