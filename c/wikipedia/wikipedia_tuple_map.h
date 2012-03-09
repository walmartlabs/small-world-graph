#ifndef __WIKIPEDIA_TUPLE_MAP__
#define __WIKIPEDIA_TUPLE_MAP__

#include <stdlib.h>
#include <vector>

using namespace std;

typedef struct {
  const char* tuple;
  const char* image_link;
  unsigned char ambiguous;
} canonical_tuple_t;

typedef struct {
  const char* alias;
  canonical_tuple_t* canonical;
} alias_canonical_pair_t;

typedef vector<alias_canonical_pair_t> TupleImageList;

class WikipediaTupleMap {
  public:
    static WikipediaTupleMap* instance(); 

    virtual void load(const char* index_root) = 0;

    /* This will put all the tuples from a snippet with their corresponding images */
    virtual void tuples_from_snippet(const char* snippet, TupleImageList& list, bool want_full_if_multiple_capitalized = false) = 0;

    /* This method returns false if no existence, true on existence, and the
     * image_link pointer will point to the image_link if it exists */
    virtual canonical_tuple_t* tuple_exists(const char* tuple) const = 0;

    virtual void clear() = 0;
    virtual size_t size() = 0;

    virtual void map(void(* fn_ptr)(const char*, const canonical_tuple_t*)) = 0;

    virtual ~WikipediaTupleMap() {}

  protected:
    static WikipediaTupleMap* instance_;

    WikipediaTupleMap() {}
};

#endif
