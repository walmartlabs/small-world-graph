#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "wikipedia_topic_suggester.h"
#include "benchmark.h"
#include "string_utils.h"

using namespace std;
using namespace boost;

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "Specify a list file" << endl;
    exit(0);
  }

  WikipediaTopicSuggester* suggest= WikipediaTopicSuggester::instance();
  bench_start("Load");
  suggest->load(argv[1]);
  bench_finish("Load");

  for(;;) {
    char input[1024];
    cout << "Type in a tuple:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string prefix = trim(string(input));

    StringList list;
    bench_start("Suggestions");
    suggest->suggestions(prefix,list);
    bench_finish("Suggestions");

    cout << "Suggestions" << endl;
    for(StringList::const_iterator ii = list.begin(); ii != list.end(); ii++) {
      cout << *ii << endl;
    }
  }

}
