#include "wikipedia_tuple_map.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc,char** argv) 
{
  if (argc < 2) {
    printf("Please specify a file root\n");
    exit(-1);
  }
  char* file_root = argv[1];
  WikipediaTupleMap* mapper = WikipediaTupleMap::instance();
  mapper->load(file_root);
  for(;;) {
    char input[1024];
    cout << "Type in a snippet: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }
    TupleImageList list;
    mapper->tuples_from_snippet(input,list);
    for(TupleImageList::const_iterator ii = list.begin(); ii != list.end(); ii++) {
      alias_canonical_pair_t acp = *ii;
      cout << acp.alias << " -> " << acp.canonical->tuple << ":" << (bool) acp.canonical->ambiguous << endl;
    }
  }
  return 0;
}
