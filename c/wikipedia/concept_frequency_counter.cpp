#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <boost/shared_ptr.hpp>

#include <vector>
#include <list>

#include <signal.h>
#include <google/sparse_hash_map>
#include <google/dense_hash_set>

#include "wikipedia_xml_parser.h"
#include "wiki_parser.h"
#include "wiki_title_validator.h"
#include "wikipedia_tuple_map.h"
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "PageParseQueue.h"
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#define LOOKAHEAD 5

using namespace boost;
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using google::sparse_hash_map;
using google::dense_hash_set;

/* Typedefs */
typedef sparse_hash_map<const char*,uint32_t, PaulHsiehHash, eqstr> ConceptCountMap;
typedef dense_hash_set<const char*,PaulHsiehHash,eqstr> DenseConceptSet;


/* Globals */
static size_t __total_pages = 0;
static size_t __pages_imported = 0;
static volatile sig_atomic_t gracefully_quit = 0;
static ConceptCountMap __counts;
static DenseConceptSet __dup_checker;
static WikipediaTupleMap* __tuple_map;

/* Prototypes */
static void frequency_count(const char* title,const char* raw_text);
static void output_frequencies();

static void handle_sigint(int signal) {
  if (!gracefully_quit) {
    cout << "Sent Quit Signal" << endl;
    output_frequencies();
    gracefully_quit = 1;
  } else {
    exit(0);
  }
}

static void handle_raw_page(const char* title, const char* text) {
  if (title && text) {
    char* transformed = wikitext_to_plaintext(text);
    if (transformed) {
      frequency_count(title,transformed);
      free(transformed);
    }
  } 

}

static inline bool is_duplicate_in_window(const char* concept)
{
  DenseConceptSet::const_iterator res;
  res = __dup_checker.find(concept);
  return res != __dup_checker.end();
}


static void frequency_count(const char* title,const char* raw_text)
{
  
  char* text = (char*) malloc(strlen(raw_text) + 1);
  /* First build a vector of starting positions of words in the text */
  vector<char *> word_starts;
  bool space_prev = true;
  int copy_idx = 0;
  for (uint32_t i = 0; i < strlen(raw_text); i++) {
    if (raw_text[i] == '\n' || raw_text[i] == ' ') {
      if (!space_prev) {
        space_prev = true;
        text[copy_idx++] = ' ';
      }
    }
    else {
      text[copy_idx] = raw_text[i];
      if (space_prev)
        word_starts.push_back(text+copy_idx);
      copy_idx++;
      space_prev = false;
    }
  }

  word_starts.push_back(text + strlen(text)); //just so that can universally get word endings by looking ahead 1 word and subtracting a character

  /* Then do variable width window on those words to find concepts */

  //StringIntHash counts = new Hash();


  char* furthest_window = text;
  for (unsigned int i = 0; i < word_starts.size()-1; i++) {
    char* start_word = word_starts.at(i);
    unsigned int phrase_words = 1;

    if (start_word >= furthest_window)
      __dup_checker.clear();

    while (i+phrase_words <= word_starts.size() - 1) {
      int phrase_chars = word_starts.at(i+phrase_words) - start_word - 1;
      int last_char = (start_word + phrase_chars)[0] - 1;
      while ((ispunct(last_char) || isspace(last_char))&& phrase_chars > 0) {
        phrase_chars--;
        last_char = (start_word + phrase_chars)[0] - 1;
      }

      if (phrase_chars > 0) {
        string phrase = string(start_word,phrase_chars);
        canonical_tuple_t* canon = __tuple_map->tuple_exists(phrase.c_str());
        if (canon) {
          furthest_window = start_word + phrase_chars;
          if (strcmp(canon->tuple,title)) {
            if (!is_duplicate_in_window(canon->tuple)) {
              const char* tuple = canon->tuple;
              ConceptCountMap::const_iterator res; 
              res = __counts.find(tuple);
              if (res != __counts.end()) {
                uint32_t count = res->second;
                count++;
                __counts[tuple] = count;
              } else {
                __counts[tuple] = 1;
              }
              __dup_checker.insert(tuple);
            }
          }
        } else if (phrase_words > LOOKAHEAD)
          break;
      }
      phrase_words++;
    }
  }
  free(text);
}

static void output_frequencies()
{
  ofstream file("/tmp/frequency_count");
  for(ConceptCountMap::iterator ii = __counts.begin(), end = __counts.end(); ii != end; ii++) {
    const char* tuple = ii->first;
    uint32_t count = ii->second;
    file << tuple << ":" << count << endl;
  }
  file.close();
}

int main(int argc,char** argv)
{
  struct sigaction sig_pipe_action;
  memset(&sig_pipe_action,'\0',sizeof(sig_pipe_action));
  sig_pipe_action.sa_handler = SIG_IGN;
  sigaction(SIGPIPE,&sig_pipe_action,NULL);
  signal(SIGINT,handle_sigint);

  char* host = (char*) "localhost";
  int port = 10010;
  int c;
  char* index_root = NULL;

  while ((c = getopt(argc, argv, "h:p:i:")) != -1) {
    switch (c) {
      case 'h':
        host = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        if (port == 0) {
          cout << "Invalid port" << endl;
          exit(-1);
        }
        break;
      case 'i':
        index_root = optarg;
        break;
    }
  }

  if (!index_root) {
    cout << "Please specify an index root" << endl;
    exit(-1);
  }

  shared_ptr<TSocket> socket(new TSocket(host,port));
  socket->setConnTimeout(1000);
  socket->setRecvTimeout(1000);
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  PageParseQueueClient client(protocol);

  __tuple_map = WikipediaTupleMap::instance();
  __tuple_map->load(index_root);
  __dup_checker.set_empty_key(NULL);

  for(;;) {
    try {
      if (gracefully_quit == 1) {
        cout << "Gracefully Quitting" << endl;
        exit(0);
      }
      transport->open();
      vector<RawPage> work_units;
      client.dequeue(work_units);
      transport->close();
      if(work_units.size() > 0) {
        int dequeued_amount = work_units.size();
        for(vector<RawPage>::const_iterator ii = work_units.begin(); ii != work_units.end(); ii++) {
          RawPage page = *ii;
          handle_raw_page(page.title.c_str(),page.content.c_str());
          __pages_imported++;
          fprintf(stdout,"\rDequeued %d | Pages Imported: %9d ",dequeued_amount, (int) __pages_imported);
          fflush(stdout);
        }
      } else {
        fprintf(stdout,"\rQueue is Empty");
        fflush(stdout);
        exit(0);
        sleep(10);
      }
    } catch (TException &tx) {
      transport->close();
      printf("ERROR: %s\n", tx.what());
      printf("Sleeping for 10s before retry\n");
      sleep(10);
    }
  }



  fprintf(stdout,"\rPages Imported: %9d / %d",(int)__pages_imported,(int) __total_pages);
  fflush(stdout);
  cout << endl;
  return 0;
}
