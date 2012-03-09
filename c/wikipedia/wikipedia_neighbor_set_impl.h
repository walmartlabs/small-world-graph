#ifndef __WIKIPEDIA_NEIGHBOR_SET_IMPL__
#define __WIKIPEDIA_NEIGHBOR_SET_IMPL__

#include "wikipedia_neighbor_set.h"

#include "equality.h"
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"
#include <google/sparse_hash_map>
#include <google/dense_hash_map>

using namespace std;
using google::sparse_hash_map;
using google::dense_hash_map;

//typedef struct page_edge_t page_edge_t;

typedef struct {
  char* title;
  unsigned char distance;
}page_edge_t;

typedef struct {
  char* title;
  bool ambiguous;
  page_edge_t* neighbors;
  unsigned char neighbors_size;
} page_neighbors_t;

typedef sparse_hash_map<char*,page_neighbors_t*,PaulHsiehHash,eqstr> TitleNeighborsHash;
typedef dense_hash_map<int,page_neighbors_t*,ThomasWangHash,eqint> IntNeighborHash;

class WikipediaNeighborSetImpl : public WikipediaNeighborSet {
  public:
    void load(const char* filename);
    void neighbors(RelationshipList& results, const string& key);
    void distances(RelationshipList& results, const TupleList& tuples);
    void map(void(* fn_ptr)(const string& title,const bool ambiguous,const vector<relationship_t>& rels));

  protected:
    TitleNeighborsHash neighbors_;
    page_neighbors_t* new_neighbor_container(const char* title);
};

#endif 
