#include "wikipedia_tuple_map_impl.h"
#include "benchmark.h"
#include <sys/stat.h>
#include "wikipedia_stopword_list.h"
#include "string_utils.h"
#include <valgrind/callgrind.h>

WikipediaTupleMapImpl::WikipediaTupleMapImpl()
{
  id_tuple_table_ = NULL;
  tuples_.set_deleted_key(NULL);
}

void 
WikipediaTupleMapImpl::load(const char* index_root)
{
  stop_words_ = new WordList(__wikipedia_stopwords);
  cout << sizeof(canonical_tuple_t) << endl;

  load_pages(index_root);
  load_redirects(index_root);
  load_images(index_root);
  load_redirect_overrides(index_root);

  delete stop_words_;
  stop_words_ = NULL;
}

/* This method returns false if no existence, true on existence, and the
 * image_link pointer will point to the image_link if it exists */
canonical_tuple_t* 
WikipediaTupleMapImpl::tuple_exists(const char* tuple) const
{
  TupleImageHash::const_iterator res;
  res = tuples_.find(tuple);
  if (res != tuples_.end()) {
    return res->second;
  } else {
    return NULL;
  }
}

/* 
 * This method looks up a word in the tuple map and appends it to the list if
 * it exists.  It assumes that the word start points to a null terminated
 * string.  If you passt the canonical ngram check, then it will ensure that
 * the canonical is at least a bigram (this helps prevent against too many stop
 * word unigrams)
 */
inline bool 
WikipediaTupleMapImpl::append_tuple_if_exists(start_finish_str* word, TupleImageList& list, bool do_canonical_ngram_check)
{
  TupleImageHash::const_iterator res;
  res = tuples_.find(word->start);
  //cout << "Searching for '" << word->start << "'" << endl;
  if (res != tuples_.end()) {
    word->found = true;
    canonical_tuple_t* result = res->second;
    alias_canonical_pair_t pair;
    pair.canonical = result;
    pair.alias = res->first;
    //cout << "Found:'" << result->tuple << "'" << endl;
    if (do_canonical_ngram_check) {
      bool more_than_one_word = false;
      int j = 0;
      while(result->tuple[j] != '\0') {
        if (result->tuple[j] == ' ') {
          more_than_one_word = true;
          break;
        }
        j++;
      }
      if (more_than_one_word) {
        //cout << "MATCH: '" << word->start << "' --> '" << result->tuple << "' Ambiguous:" << (bool) result->ambiguous << endl;
        list.push_back(pair);
      }
    } else {
      //cout << "MATCH: '" << word->start << "' --> '" << result->tuple << "' Ambiguous:" << (bool) result->ambiguous << endl;
      list.push_back(pair);
    }
    return true;
  } else {
    return false;
  }
}

/* 
 * Some crazy NLP that can do a sliding window on a snippet looking for tuples
 *
 * Step 1.  Convert sentence in start_finish_str* of words (this is for
 * performance reasons, so we don't need to create multiple strings)
 *
 * Step 2.  Do sliding ngram windows (starting with 3 down to 2 down to 1), and
 * keep track of words that are part of ngrams that hit in the tuples_ hash
 *
 * Step 3.  On the unigram window, only take a unigram if it is an alias of a
 * canonical URL that is longer than one word
 */
void 
WikipediaTupleMapImpl::tuples_from_snippet(const char* snippet, TupleImageList& list, bool want_full_if_multiple_capitalized) /* variable want_full_if_multiple_capitalized now actually just means want_exact_match_unless_starts_with_"anything"_etc. */
{
  int len = strlen(snippet);
  char* dup = (char*) malloc(sizeof(char) * len + 1);
  memcpy(dup,snippet,len + 1);
  cout << "Snippet: " << snippet << endl;
  char* front = dup;
  char* cur = dup;
  int word_count = 0;
  //int capitalized_word_count = 0;
  start_finish_str* words = 0;
  int i=0;
  // leading whitespace
  while(dup[i] == ' ') {
    i++;
  }
  cur += i;
  while(i <= len) {
    if (dup[i] == ' ' || dup[i] == '-' || dup[i] == ',' || dup[i] == ';' || dup[i] == '"') {
      words = (start_finish_str*) realloc(words,(word_count + 1) * sizeof(start_finish_str));
      words[word_count].start = cur;
      /*
      if (*cur >= 65 && *cur <= 90)
        capitalized_word_count++;
      */
      words[word_count].finish = front + i;
      words[word_count].found = false;
      word_count++;
      i++;
      while(dup[i] == ' ' || dup[i] == '-' || dup[i] == ',' || dup[i] == ';' || dup[i] == '"') {
        i++;
      }
      cur = front + i;
    } else if (dup[i] == '\0') {
      words = (start_finish_str*) realloc(words,(word_count + 1) * sizeof(start_finish_str));
      words[word_count].start = cur;
      /*
      if (*cur >= 65 && *cur <= 90)
        capitalized_word_count++;
      */
      words[word_count].finish = front + i;
      words[word_count].found = false;
      word_count++;
    }
    i++;
  }

  if (word_count < 9) {
    *words[word_count-1].finish = '\0';
    if (append_tuple_if_exists(&words[0],list)) {
      free(words);
      free(dup);
      return;
    }
  }

  //if (want_full_if_multiple_capitalized && capitalized_word_count > 1) {
  if (want_full_if_multiple_capitalized && (strcmp("i like",snippet) && strcmp("I like",snippet) && strcmp("everything",snippet) && strcmp("Everything",snippet) && strcmp("anything",snippet) && strcmp("Anything",snippet))) {
    free(words);
    free(dup);
    return;
  }

  /* don't need this anymore because of above block 
  if (word_count == 1) {
    *words[0].finish = '\0';
    append_tuple_if_exists(&words[0],list);
    free(words);
    free(dup);
    return;
  }
  */

  if (word_count == 2) {
    /*  don't need this anymore either because of above block
    *words[1].finish = '\0';
    bool bigram_found = append_tuple_if_exists(&words[0],list);
    if (!bigram_found) {
    */
    *words[0].finish = '\0';
    append_tuple_if_exists(&words[0],list,true);
    append_tuple_if_exists(&words[1],list,true);
    //}
    free(words);
    free(dup);
    return;
  }

  // Do quadgram sliding window
  for(int i=0; i < word_count - 3; i++) {
    char orig = *(words[i+3].finish);
    *(words[i+3].finish) = '\0';
    bool quadgram_found = append_tuple_if_exists(&words[i],list);
    *(words[i+3].finish) = orig;
    if (quadgram_found) {
      words[i].found = true;
      words[i+1].found = true;
      words[i+2].found = true;
      words[i+3].found = true;
      i += 3;
    }
  }

  // Do trigram sliding window
  for(int i=0; i < word_count - 2; i++) {
    char orig = *(words[i+2].finish);
    *(words[i+2].finish) = '\0';
    bool trigram_found = append_tuple_if_exists(&words[i],list);
    *(words[i+2].finish) = orig;
    if (trigram_found) {
      words[i].found = true;
      words[i+1].found = true;
      words[i+2].found = true;
      i += 2;
    }
  }

  // Do Bigram sliding window of pairs of unfound words in trigram window
  for(int i=0; i < word_count - 1; i++) {
    if (!words[i].found && !words[i+1].found) {
      char orig = *(words[i+1].finish);
      *(words[i+1].finish) = '\0';
      bool bigram_found = append_tuple_if_exists(&words[i],list);
      *(words[i+1].finish) = orig;
      if (bigram_found) {
        words[i].found = true;
        words[i+1].found = true;
        i++;
      }
    }
  }

  // Do unigram window.  Only take a unigram in this case if it maps to a
  // canonical tuple with more than one word.  This is to prevent an overflow
  // of possible stop words
  for(int i=0; i < word_count; i++) {
    if (!words[i].found && (words[i].start[0]>=65 && words[i].start[0]<=90)) {
      char orig = *(words[i].finish);
      *(words[i].finish) = '\0';
      append_tuple_if_exists(&words[i],list,true);
      *(words[i].finish) = orig;
    }
  }
  free(words);
  free(dup);
}

size_t 
WikipediaTupleMapImpl::size()
{
  return tuples_.size();
}

void 
WikipediaTupleMapImpl::clear()
{
  for(TupleImageHash::iterator ii = tuples_.begin(), end = tuples_.end(); ii != end; ii++) {
    char* tuple = (char*) ii->first;
    canonical_tuple_t* canonical = (canonical_tuple_t*) ii->second;
    if (strcmp(tuple,canonical->tuple)) {
      tuples_.erase(ii);
      free(tuple);
    }
  }

  // Only canonicals should be left
  for(TupleImageHash::iterator ii = tuples_.begin(), end = tuples_.end(); ii != end; ii++) {
    char* tuple = (char*) ii->first;
    canonical_tuple_t* canonical = (canonical_tuple_t*) ii->second;
    free((char*) canonical->image_link);
    free(canonical);
    free(tuple);
  }
}


void 
WikipediaTupleMapImpl::map(void(* fn_ptr)(const char*, const canonical_tuple_t*))
{
  TupleImageHash::const_iterator end = tuples_.end();
  for(TupleImageHash::const_iterator ii = tuples_.begin(); ii != end; ii++) {
    fn_ptr(ii->first,ii->second);
  }
}

/* Entry Point into the Library */
WikipediaTupleMap* WikipediaTupleMap::instance_ = NULL;
WikipediaTupleMap* WikipediaTupleMap::instance() {
  if (!instance_) {
    instance_ = new WikipediaTupleMapImpl();
  }
  return instance_;
}

/* Static Implementation */
void 
WikipediaTupleMapImpl::load_pages(const char* index_root)
{
  bench_start("Load Pages");
  FILE* file = NULL;
  char buffer[4096];

  tuples_.resize(10000000); // optimization
  id_tuple_table_ = NULL;
  size_t biggest_id_so_far = 0;
  string index_filename = string(index_root) + ".pages";
  int page_count = 0;
  if ((file = fopen((char*)index_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,file)) {
      char* first = NULL;
      char* second = NULL;
      char* third = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '|') {
          buffer[i] = '\0';
          first = buffer;
          second = buffer + i + 1;
          for(int j=i; j < length; j++) {
            if (buffer[j] == '|') {
              buffer[j] = '\0';
              third = buffer + j + 1;
              buffer[length-1] = '\0';
              break;
            }
          }

          if (!first || !second || !third)
            break;

          int id = atoi(first);
          int ambiguous = atoi(third);
          char* key = strdup(second);
          downcase(second);

          if (id > (int) biggest_id_so_far) {
            id_tuple_table_ = (canonical_tuple_t**) realloc(id_tuple_table_,sizeof(canonical_tuple_t*) * (id + 1));
            for(int k = biggest_id_so_far + 1; k < id + 1; k++) {
              id_tuple_table_[k] = NULL;
            }
            biggest_id_so_far = id;
          }
          if (!stop_words_->match(second)) {
            canonical_tuple_t* pair = (canonical_tuple_t*) malloc(sizeof(canonical_tuple_t));
            pair->tuple = key;
            pair->image_link = NULL;
            pair->ambiguous = (ambiguous == 1);
            tuples_[key] = pair;
            page_count++;
            id_tuple_table_[id] = pair;
          } else {
            //cout << "Skipping " << key << endl;
            free(key);
          }
          break;
        }
      }
    }
    fclose(file);
  } else {
    cout << "Page File Open Error" << endl;
  }

  bench_finish("Load Pages");
  cout << "Loaded:" << page_count << " pages." << endl;
}

void 
WikipediaTupleMapImpl::load_redirects(const char* index_root)
{
  CALLGRIND_START_INSTRUMENTATION;
  bench_start("Load Redirects");
  string redirects_filename = string(index_root) + ".redirects";
  char buffer[4096];
  FILE* redirects_file = NULL;
  if ((redirects_file = fopen((char*)redirects_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,redirects_file)) {
      char* first = NULL;
      char* second = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '>') {
          buffer[i] = '\0';
          first = buffer;
          buffer[length-1] = '\0';
          second = buffer + i + 1;
          if (!first || !second)
            break;

          int id = atoi(second);

          canonical_tuple_t* pair = id_tuple_table_[id];
          if (pair) {
            char* key = strdup(first);
            downcase(first);
            if (!stop_words_->match(first)) {
              tuples_[key] = pair;
            } else {
              free(key);
            }
          }
          break;
        }
      }
    }
    fclose(redirects_file);
  } else {
    cout << "Redirects File Open Error" << endl;
  }
  bench_finish("Load Redirects");
  CALLGRIND_STOP_INSTRUMENTATION;
}

void 
WikipediaTupleMapImpl::load_redirect_overrides(const char* index_root)
{
  bench_start("Load Redirect Overrides");
  string redirects_filename = string(index_root) + ".overrides";
  char buffer[4096];
  FILE* redirects_file = NULL;
  if ((redirects_file = fopen((char*)redirects_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,redirects_file)) {
      char* first = NULL;
      char* second = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '>') {
          buffer[i] = '\0';
          first = buffer;
          buffer[length-1] = '\0';
          second = buffer + i + 1;
          if (!first || !second)
            break;

          TupleImageHash::const_iterator canonical_it = tuples_.find(second);
          if (canonical_it != tuples_.end()) {
            canonical_tuple_t* canon = canonical_it->second;
            // Now look for the redirect
            TupleImageHash::const_iterator redirect_it = tuples_.find(first);
            if (redirect_it != tuples_.end()) {
              const char* key = redirect_it->first;
              cout << "Redirecting Existing: " << key << " to " << canon->tuple << endl;
              tuples_[key] = canon;
            } else {
              char* new_key = strdup(first);
              cout << "Redirecting New: " << new_key << " to " << canon->tuple << endl;
              tuples_[new_key] = canon;
            }
          }
          break;
        }
      }
    }
    fclose(redirects_file);
  } else {
    cout << "Overrides File Open Error" << endl;
  }
  bench_finish("Load Redirect Overrides");
}

void 
WikipediaTupleMapImpl::load_images(const char* index_root)
{
  bench_start("Load Images");
  FILE* file = NULL;
  char buffer[4096];
  string raw_images_filename = string(index_root) + ".images";
  string validated_images_filename = string(index_root) + ".images.validated";
  string images_filename = "";
  struct stat stats;
  if (!stat(validated_images_filename.c_str(),&stats)) {
    images_filename = validated_images_filename;
  } else {
    images_filename = raw_images_filename;
  }
  cout << "Loading: " << images_filename << endl;
  if ((file = fopen((char*)images_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,file)) {
      char* first = NULL;
      char* second = NULL;
      int length = strlen(buffer);
      for(int i = 0; i < length; i++) {
        if (buffer[i] == '|') {
          buffer[i] = '\0';
          first = buffer;
          second = buffer + i + 1;
          buffer[length-1] = '\0';

          TupleImageHash::const_iterator res;
          res = tuples_.find(first);
          if (res != tuples_.end()) {
            char* image = strdup(second);
            canonical_tuple_t* canonical = res->second;
            canonical->image_link = image;
          }
          break;
        }
      }
    }
  } else {
    cout << "File Open Error" << endl;
  }
  bench_finish("Load Images");
}
