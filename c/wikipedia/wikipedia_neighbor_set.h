#ifndef __WIKIPEDIA_NEIGHBOR_SET__
#define __WIKIPEDIA_NEIGHBOR_SET__

#define _FILE_OFFSET_BITS 64

#include <string>
#include <vector>

#include "wikipedia_types.h"

using namespace std;

typedef vector<string> TupleList;
typedef vector<relationship_t> RelationshipList;

class WikipediaNeighborSet {
  public:
    static WikipediaNeighborSet* instance(); 

    virtual void load(const char* filename) = 0;

    virtual void neighbors(RelationshipList& results, const string& key) = 0;

    virtual void distances(RelationshipList& results, const TupleList& tuples) = 0;

    virtual void map(void(* fn_ptr)(const string& title,const bool ambiguous,const vector<relationship_t>& rels)) = 0;
    
    virtual ~WikipediaNeighborSet() {}

  protected:
    static WikipediaNeighborSet* instance_;

    WikipediaNeighborSet() {}
};

#endif
