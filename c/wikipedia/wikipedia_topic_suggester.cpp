#include "wikipedia_topic_suggester.h"
#include <iostream>
#include <stdio.h>
#include "string_utils.h"
#include "benchmark.h"

using namespace std;

void
WikipediaTopicSuggester::load(const string& list_filename)
{
  bench_start("Load Topic Suggester");
  FILE* file = NULL;
  char buffer[4096];
  if ((file = fopen((char*)list_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,file)) {
      string line = string(buffer);
      string trimmed = trim(line);
      uint32_t max_prefix = min(+MAX_PREFIX,trimmed.size());
      //uint32_t max_prefix = min((size_t) INT_MAX,trimmed.size());
      for(uint32_t i=1; i <= max_prefix; i++) {
        string prefix = downcase(trimmed.substr(0,i));

        PrefixTitleMap::const_iterator res = prefix_map_.find(prefix.c_str());
        StringList* list = NULL;
        if (res != prefix_map_.end())
          list = res->second;
        else {
          list = new StringList();
          prefix_map_[strdup(prefix.c_str())] = list;
        }
        
        if (list->size() < MAX_WORDS_PER_KEY) {
          list->push_back(trimmed);
        }
      }
    }
  } else {
    cout << "Couldn't open topics file" << endl;
  }
  bench_finish("Load Topic Suggester");
}

void
WikipediaTopicSuggester::suggestions(const string& prefix, StringList& list)
{
  bench_start("Topic Suggestion " + prefix);
  PrefixTitleMap::const_iterator res = prefix_map_.find(prefix.c_str());
  if (res != prefix_map_.end()) {
    StringList* stored_list = res->second;
    for(StringList::const_iterator ii = stored_list->begin(), end = stored_list->end(); ii != end; ii++) {
      list.push_back(*ii);
    }
  }
  bench_finish("Topic Suggestion " + prefix);
}

/* Singleton Initialization */
WikipediaTopicSuggester* WikipediaTopicSuggester::instance_ = NULL;
WikipediaTopicSuggester* WikipediaTopicSuggester::instance() {
  if (!instance_) {
    instance_ = new WikipediaTopicSuggester();
  }
  return instance_;
}
