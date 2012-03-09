#ifndef __WIKIPEDIA_SPELL_CHECKER_H__
#define __WIKIPEDIA_SPELL_CHECKER_H__

#include <string>
#include <vector>
#include "hunspell/hunspell.hxx"

using namespace std;

typedef vector<string> SuggestionList;

class WikipediaSpellChecker {
  public:
    static WikipediaSpellChecker* instance(); 

    void load(const string& list_filename);

    /* This will put all the tuples from a snippet with their corresponding images */
    void suggestions(const string& word, SuggestionList& list);

    virtual ~WikipediaSpellChecker() {}

  protected:
    static WikipediaSpellChecker* instance_;
    Hunspell* speller_;

    WikipediaSpellChecker() {}
};

#endif
