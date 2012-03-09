#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <iostream>
#include <raptor.h>
#include <string.h>
#include <map>  
#include <list>
#include <vector>
#include <algorithm>
#include <sstream>
#include "equality.h"
#include "paul_hsieh_hash.h"
#include <google/sparse_hash_set>

using namespace std;
using google::sparse_hash_set;

typedef sparse_hash_set<char*,PaulHsiehHash,eqstr> EntityTitleSet;

class DbpediaOntology {
  public:
    /* Load in a typenames file */
    DbpediaOntology(const char* filename);

    bool is_entity(const char* title);

  protected:
    EntityTitleSet entities_;

};
