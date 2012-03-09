/* Support Large File */
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <expat.h>
#include <string>

#include "wikipedia_page.h"

#include <google/sparse_hash_map>
#include "paul_hsieh_hash.h"
#include "equality.h"
#include <getopt.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

int main(int argc,char** argv)
{
  if (argc < 3) {
    cout << "Please specify an index root and a subgraph output root" << endl;
    exit(0);
  }

  char* index_root = argv[1];
  char* output_root = argv[2];
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  man->load_from_index(index_root);

  for(;;) {
    char input[1024];
    cout << "Type in a tuple:";
    cin.getline(input,1024);
    if (cin.eof()) {
      cout << endl;
      exit(0);
    }

    string first = input;
    vector<string> split_input;
    split(split_input,input,is_any_of("|"));

    man->to_subgraph(output_root,split_input);
  }

  return 0;
}
