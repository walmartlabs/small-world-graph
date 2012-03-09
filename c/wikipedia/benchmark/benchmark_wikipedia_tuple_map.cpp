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
  TupleImageList list;
  mapper->tuples_from_snippet("Lionel Richie, Jackson's collaborator on the anthem \"We Are the World,\" sang a gospel classic, \"Jesus is Love.\"",list);
  for(TupleImageList::const_iterator ii = list.begin(); ii != list.end(); ii++) {
    cout << (*ii).canonical->tuple << endl;
  }
  list.clear();

  for(;;) {
    char input[1024];
    cout << "Type in a pipe delinated set of tuples: ";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }
    canonical_tuple_t* canon = mapper->tuple_exists(input);
    if (canon && canon->image_link) {
      cout << "'" << input << "' -> ('" << canon->tuple << "','" << canon->image_link << "," << (bool) canon->ambiguous << ")" << endl;
    } else if(canon) {
      cout << "'" << input << "' -> ('" << canon->tuple << "',NULL," << (bool) canon->ambiguous << ")" << endl;
    } else {
      cout << "Didn't find any tuple for '" << input << "'" << endl;
    }
  }
  return 0;
}
