#include "string_pool.h"
#include <string.h>

StringPool::StringPool(uint64_t initial_size) 
{
  pool_.set_empty_key(NULL);
  pool_.resize(initial_size);
}

StringPool::~StringPool()
{
  for(StringSet::iterator ii = pool_.begin(); ii != pool_.end(); ii++) {
    char* key = *ii;
    free(key);
  }
  pool_.clear();
}

/* If it exists in the set return the existing, otherwise, put it in */
char* 
StringPool::retrieve(const char* key)
{
  if (!key)
    return NULL;

  StringSet::const_iterator res = pool_.find((char*) key);

  if (res != pool_.end()) {
    return *res;
  } else {
    char* pooled_key = strdup(key);
    pool_.insert(pooled_key);
    return pooled_key;
  }
}
