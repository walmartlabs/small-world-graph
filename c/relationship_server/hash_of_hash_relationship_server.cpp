#include "relationship_holder.h"
#include <iostream>
#include <algorithm>

using namespace std;

struct eqint {
  bool operator()(const int i1, const int i2) const
  {
    return i1 == i2;
  }
};

class ThomasWangHash {
  public:
    size_t operator()(unsigned int a) const
    {
      a = (a+0x7ed55d16) + (a<<12);
      a = (a^0xc761c23c) ^ (a>>19);
      a = (a+0x165667b1) + (a<<5);
      a = (a+0xd3a2646c) ^ (a<<9);
      a = (a+0xfd7046c5) + (a<<3);
      a = (a^0xb55a4f09) ^ (a>>16);
      return a;
    }
};

typedef sparse_hash_map<unsigned int, float,ThomasWangHash,eqint> SubHash;

class HashOfHashRelationshipHolderImpl : public RelationshipHolder {
  public:
    vector<Relationship> distances(const vector<int>& ids);
    CloseSiteVectorPtr sites_close_to(const int site_id);
    float distance_between(const int site_id,const int other_id);
    void store(const int site_id,const int other_id,const float distance);
    string statistics();
  protected:
    sparse_hash_map<unsigned int, shared_ptr<SubHash>,ThomasWangHash, eqint> relationships_;
};


/* Implementation */
shared_ptr<RelationshipHolder> RelationshipHolder::instance_ = shared_ptr<RelationshipHolder>();

shared_ptr<RelationshipHolder> 
RelationshipHolder::instance() {
  if (!instance_) {
    instance_ = shared_ptr<RelationshipHolder>(new HashOfHashRelationshipHolderImpl());
  }
  return instance_;
}

vector<Relationship> 
HashOfHashRelationshipHolderImpl::distances(const vector<int>& ids)
{
  vector<Relationship> rels;
  vector<int> sorted_ids;
  shared_ptr<SubHash> sub_hash;
  SubHash::const_iterator result;
  float distance;
  unsigned int i,first_id,second_id;

  unique_copy(ids.begin(),ids.end(),std::inserter(sorted_ids,sorted_ids.begin()));
  sort(sorted_ids.begin(),sorted_ids.end());

  for(i = 0; i < sorted_ids.size(); i++) {
    first_id = sorted_ids[i];
    sub_hash = relationships_[first_id];
    if (sub_hash != NULL) {
      for(unsigned int j = i + 1; j < sorted_ids.size(); j++) {
        second_id = sorted_ids[j];
        result = sub_hash->find(second_id);
        if (result != sub_hash->end()) {
          Relationship rel;
          distance = (*result).second;
          rel.site_id = first_id;
          rel.other_site_id = second_id;
          rel.distance = distance;
          rels.push_back(rel);
          //cout << "Found Relationship for " << first_id << ":" << second_id << ":" << distance << endl;
        }
      }
    }
  }

  return rels;
}

float 
HashOfHashRelationshipHolderImpl::distance_between(const int from_id,const int to_id)
{
  shared_ptr<SubHash> sub_hash = relationships_[from_id];
  float distance = 1.0f;
  if (sub_hash != NULL) {
    SubHash::const_iterator ii;
    ii = sub_hash->find(to_id);
    if (ii == sub_hash->end()) {
      return 1.0f;
    } else {
      return (*ii).second;
    }
  }
  return distance;
}

CloseSiteVectorPtr
HashOfHashRelationshipHolderImpl::sites_close_to(const int site_id)
{
  CloseSiteVectorPtr close_sites = CloseSiteVectorPtr(new CloseSiteVector());
  shared_ptr<SubHash> sub_hash;
  SubHash::const_iterator ii;

  sub_hash = relationships_[site_id];
  if (sub_hash != NULL) {
    for(ii = sub_hash->begin(); ii != sub_hash->end(); ii++) {
      close_sites->push_back((*ii).first);
    }
  }
  return close_sites;
}

void 
HashOfHashRelationshipHolderImpl::store(const int site_id,const int other_id,const float distance)
{
  shared_ptr<SubHash> sub_hash;

  sub_hash = relationships_[site_id];
  if (sub_hash == NULL) {
    sub_hash = shared_ptr<SubHash>(new SubHash());
    (*sub_hash)[other_id] = distance;
    relationships_[site_id] = sub_hash;
  } else {
    (*sub_hash)[other_id] = distance;
  }

  /* Now do the bidirectional */
  sub_hash = relationships_[other_id];
  if (sub_hash == NULL) {
    sub_hash = shared_ptr<SubHash>(new SubHash());
    (*sub_hash)[site_id] = distance;
    relationships_[other_id] = sub_hash;
  } else {
    (*sub_hash)[site_id] = distance;
  }
}

string
HashOfHashRelationshipHolderImpl::statistics()
{
  stringstream stats;
  stats << "Number of Hash Entries: " << relationships_.size() << endl;
  stats << "Number of Buckets: " << relationships_.bucket_count() << endl;
  stats << "Max Number of Buckets: " << relationships_.max_bucket_count() << endl;
  return stats.str();
}
