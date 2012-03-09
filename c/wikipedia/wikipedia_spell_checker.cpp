#include "wikipedia_spell_checker.h"
#include <iostream>

using namespace std;

void
WikipediaSpellChecker::load(const string& list_filename)
{
  string dic = list_filename + ".dic";
  string aff = list_filename + ".aff";
  speller_ = new Hunspell(aff.c_str(),dic.c_str());
  return;
}

void
WikipediaSpellChecker::suggestions(const string& word, SuggestionList& list)
{
  char **lst = NULL;
  int nresults = speller_->suggest(&lst, word.c_str());
  if (lst && nresults) {
    cout << "Found Results" << endl;
    for(int i=1; i < nresults; i++) {
      list.push_back(lst[i]);
    }
    //speller_->free_list(&lst, nresults);
  }
}

/* Singleton Initialization */
WikipediaSpellChecker* WikipediaSpellChecker::instance_ = NULL;
WikipediaSpellChecker* WikipediaSpellChecker::instance() {
  if (!instance_) {
    instance_ = new WikipediaSpellChecker();
  }
  return instance_;
}
