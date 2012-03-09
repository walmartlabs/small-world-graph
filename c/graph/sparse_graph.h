#ifndef __SPARSE_GRAPH__
#define __SPARSE_GRAPH__

#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <list>
#include <vector>

#include "equality.h"
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"
#include <google/sparse_hash_map>
#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <boost/pool/object_pool.hpp>
#include <pillowtalk.h>

using namespace std;
using namespace boost;
using google::sparse_hash_map;
using google::dense_hash_map;
using google::dense_hash_set;

struct edge_t;

struct page_t {
  uint32_t id;
  char* title;
  char* home_page;
  char* twitter;
  char* blog;
  edge_t** edges;
  uint16_t num_edges;
};

struct page_list_t {
  uint32_t length;
  page_t** pages;
};

// SOURCES
#define WIKIPEDIA 1
//

// TYPE_CATEGORY_MASKS (each mask followed by types in that category)
// can go by powers of 2, for a total of 32 categories
typedef enum {
  GENERAL=1,
  UNKNOWN_WITH_SNIPPET=2,
  UKNOWN_WITHOUT_SNIPPET=4,
  BUSINESS=8,
  GENEALOGICAL=16,
  RELIGION=32,
  EDUCATION=64,
  MOVIES=128,
  LITERATURE=256
} category_type_t;

struct edge_descriptor_t {
  uint8_t source;
  uint32_t type_category_mask;
  uint8_t type;
  char* snippet;
  uint8_t weight;
};

struct edge_t {
  page_t* begin;
  page_t* end;
  uint8_t min_weight;
  uint32_t type_categories_mask;
  uint16_t num_descriptors;
  edge_descriptor_t** descriptors;
};

typedef list<edge_t*> Chain;

struct shortest_path_t {
  page_t* end;
  uint16_t distance;
  uint8_t links;
  shortest_path_t* prev;
  shortest_path_t* next;
  bool operator<(const shortest_path_t& b) const {
    return distance < b.distance;
  }
};

struct edge_compare_t
{
  bool operator()(const edge_t* a, const edge_t* b) const
  {
    return (a->min_weight < b->min_weight);
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
typedef dense_hash_map<page_t*,shortest_path_t*,ThomasWangHashPtr,eqptr> PagePathHash;
typedef dense_hash_map<page_t*,page_t*,ThomasWangHashPtr,eqptr> PagePageHash;
typedef dense_hash_map<page_t*,edge_t*,ThomasWangHashPtr,eqptr> PageEdgeHash;
typedef dense_hash_map<page_t*,char*,ThomasWangHashPtr,eqptr> PageCharStarHash;
typedef dense_hash_map<int,shortest_path_t*,ThomasWangHash,eqptr> IntPathHash;
typedef dense_hash_set<page_t*,ThomasWangHashPtr,eqptr> PageSet;
typedef dense_hash_set<char*,PaulHsiehHash,eqstr> CharStarSet;
typedef dense_hash_map<char*,int,PaulHsiehHash,eqstr> CharStarIntHash;
typedef dense_hash_map<char*,page_list_t*,PaulHsiehHash,eqstr> TitlePagesHash;
typedef vector<string> PageList;

struct shadow_graph_t {
  page_t* root;
  list<page_t*> freeable_shadow_nodes;
  PageCharStarHash page_category_map;
  CharStarSet category_set;
  int new_max_id;

  shadow_graph_t();
  ~shadow_graph_t();
};


class BucketQueue {
  public:
    static const size_t MaxKey = 255;
    static const size_t MaxLinks = 3;
    static const size_t MaxPathLink = MaxKey * MaxLinks;
    BucketQueue(uint32_t max_page_id);
    virtual ~BucketQueue();

    shortest_path_t* delete_min();
    void insert(page_t* key, uint16_t distance, uint8_t links);
    void decrease_key(page_t* key, uint16_t new_distance, uint8_t new_links);

    /* If there is a path already to a particular key, return it, otherwise return NULL */
    shortest_path_t* d(page_t* key);
    size_t size() { return size_; }

  protected:
    void place_path_in_correct_bucket(shortest_path_t* path);
    void remove_path_from_bucket(shortest_path_t* path);

    shortest_path_t** id_to_path_map_;
    shortest_path_t* buckets_;

    boost::object_pool<shortest_path_t> pool_;
    size_t size_;
    size_t min_bucket_;
    size_t max_bucket_;
};

class SparseGraph {
  public:
    static SparseGraph* instance(); 

    page_t* page(const char* title) const;
    page_list_t* concepts(const char* name) const;

    void load(const char* filename);

    /* This method populates the list with sorted edges and their descriptors */
    void edges(Chain& _return, const string& topic);

    /* 
     * This method looks at set of source tuples and a set of destination
     * tuples and finds the shortest chain between them 
     */
    void shortest_chain(Chain& chain, const vector<string>& first_set, const vector<string>& second_set,const int max_links,const bool increasing_distances_1, const bool increasing_distances_2, const char** start_title, const char** end_title);

    void clear();

    virtual ~SparseGraph() {}

  protected:
    static SparseGraph* instance_;

    page_t** id_page_table_;
    uint32_t cur_id_;
    int max_id_;
    TitlePagesHash aliases_;
    TitlePageHash pages_;

    inline edge_descriptor_t* build_edge_descriptor_from_json(pt_node_t* ed_node);
    inline edge_t* build_edge_from_edge_json(page_t* begin,char* end_point_title,pt_node_t* edge_node);
    inline void build_page_from_open_graph_json(char* title, char* json);
    inline page_t* find_or_build_page(const char* page_name);
    inline page_t* build_shadow_page(const vector<string>& edges, const bool increasing_distances);
    inline shadow_graph_t* build_user_shadow_graph(const string& username);
    inline void append_or_build_alias_list(const char* alias, page_t* page);

    SparseGraph();
};

#endif
