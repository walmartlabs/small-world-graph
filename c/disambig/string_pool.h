#ifndef __STRING_POOL_H__
#define __STRING_POOL_H__

#include <stdlib.h>
#include <google/dense_hash_set>
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"

using google::dense_hash_set;
using namespace std;

typedef dense_hash_set<char*,PaulHsiehHash,eqstr> StringSet;

class StringPool {
  public:
    StringPool(uint64_t initial_size = 0);
    virtual ~StringPool();

    char* retrieve(const char* key);

  protected:
    StringSet pool_;
};

#endif
