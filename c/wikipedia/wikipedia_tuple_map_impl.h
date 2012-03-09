#ifndef __WIKIPEDIA_TUPLE_MAP_IMPL__
#define __WIKIPEDIA_TUPLE_MAP_IMPL_

#include "wikipedia_tuple_map.h"
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "word_list.h"
#include <google/dense_hash_map>
#include <google/sparse_hash_map>

using namespace std;
using google::sparse_hash_map;

typedef sparse_hash_map<const char*,canonical_tuple_t*, PaulHsiehHash, eqstr> TupleImageHash;

typedef struct {
  char* start;
  char* finish;
  bool found;
} start_finish_str;

class WikipediaTupleMapImpl : public WikipediaTupleMap {
  public:
    WikipediaTupleMapImpl();
    void load(const char* index_root);

    void tuples_from_snippet(const char* snippet, TupleImageList& list, bool want_full_if_multiple_capitalized = false);

    canonical_tuple_t* tuple_exists(const char* tuple) const;

    size_t size();
    void clear();

    void map(void(* fn_ptr)(const char*, const canonical_tuple_t*));

    virtual ~WikipediaTupleMapImpl() {}
  protected:
    void load_pages(const char* index_root);
    void load_redirects(const char* index_root);
    void load_images(const char* index_root);
    void load_redirect_overrides(const char* index_root);
    inline bool append_tuple_if_exists(start_finish_str* word, TupleImageList& list, bool do_canonical_ngram_check = false);

    TupleImageHash tuples_;
    canonical_tuple_t** id_tuple_table_;
    WordList* stop_words_;
};

#endif
