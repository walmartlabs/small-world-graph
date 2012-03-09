#ifndef __WIKIPEDIA_GRAPH__
#define __WIKIPEDIA_GRAPH__

#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <list>

#include "equality.h"
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"
#include <google/sparse_hash_map>
#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include "wikipedia_types.h"

using namespace std;
using google::sparse_hash_map;
using google::dense_hash_map;
using google::dense_hash_set;

struct page_t {
  char* title;
  page_t** inlinks;
  int inlinks_size;
  page_t** outlinks;
  int outlinks_size;
};

struct edge_t {
  page_t* first;
  page_t* second;
  size_t distance;
};

struct page_array_t {
  page_t** array;
  size_t length;
};

struct shortest_path_t {
  page_t* end;
  size_t distance;
  size_t links;
  bool operator<(const shortest_path_t& b) const {
    return distance < b.distance;
  }
};

struct edge_compare_t
{
  bool operator()(const edge_t* a, const edge_t* b) const
  {
    return (a->distance < b->distance);
  }
};

struct shortest_path_compare_t
{
  bool operator()(const shortest_path_t* a, const shortest_path_t* b) const
  {
    return (a->distance > b->distance);
  }
};

typedef pair<page_t*,unsigned char> OneWayDistance;
typedef sparse_hash_map<char*,page_t*,PaulHsiehHash,eqstr> TitlePageHash;
typedef dense_hash_map<page_t*,int,ThomasWangHashPtr,eqptr> PageIntHash;
typedef dense_hash_map<page_t*,shortest_path_t,ThomasWangHashPtr,eqptr> PagePathHash;
typedef dense_hash_map<page_t*,page_t*,ThomasWangHashPtr,eqptr> PagePageHash;
typedef dense_hash_set<page_t*,ThomasWangHashPtr,eqptr> PageSet;
typedef vector<string> PageList;

class WikipediaGraph {
  public:
    static WikipediaGraph* instance(); 

    page_t* page(const char* title);

    void load(const char* filename);

    void page_chain(PageList& chain, PageList& referring_pages, const string& first, const string& second);

    void clear();

    virtual ~WikipediaGraph() {}

  protected:
    static WikipediaGraph* instance_;

    TitlePageHash pages_;

    WikipediaGraph() {}
};

#endif
