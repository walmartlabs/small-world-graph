#ifndef __WIKIPEDIA_TOPIC_SUGGESTER__
#define __WIKIPEDIA_TOPIC_SUGGESTER__

#include <string>
#include <vector>

#include <google/sparse_hash_map>
#include <google/sparse_hash_set>
#include "paul_hsieh_hash.h"
#include "equality.h"

using namespace std;
using google::sparse_hash_map;
using google::sparse_hash_set;

typedef vector<string> StringList;

typedef sparse_hash_set<const char*,PaulHsiehHash,eqstr> StringPool;
typedef sparse_hash_map<const char*,StringList*,PaulHsiehHash,eqstr> PrefixTitleMap;


class WikipediaTopicSuggester {
  public:
    static const size_t MAX_PREFIX = 5;
    static const size_t MAX_WORDS_PER_KEY = 20;

    static WikipediaTopicSuggester* instance(); 

    void load(const string& list_filename);

    /* This will put all the tuples from a snippet with their corresponding images */
    void suggestions(const string& prefix, StringList& list);

    virtual ~WikipediaTopicSuggester() {}

  protected:
    static WikipediaTopicSuggester* instance_;
    StringPool string_pool_;
    PrefixTitleMap prefix_map_;

    WikipediaTopicSuggester() {}
};

#endif
