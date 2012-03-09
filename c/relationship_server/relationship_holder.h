#ifndef __RELATIONSHIP_HOLDER_H
#define __RELATIONSHIP_HOLDER_H

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>
#include "thrift/site_relationships_types.h"

using namespace std;
using namespace boost;

typedef vector<int32_t> CloseSiteVector;

class RelationshipHolder {
  public:
    static shared_ptr<RelationshipHolder> instance(); 
    virtual void distances(vector<SiteRelationship>& rels,const vector<int>& ids) = 0;
    virtual void sites_close_to(CloseSiteVector& sites, const int site_id) = 0;

    virtual unsigned char distance_between(const int site_id,const int other_id) = 0;
    virtual size_t size() = 0;
    virtual void store(const int site_id,const int other_id,const unsigned char distance) = 0;
    virtual void destroy(const int site_id,const int other_id) = 0;
    virtual void persist(const string& filename) = 0;
    virtual void load(const string& filename) = 0;

    virtual void calculate_distance_between(const string& site,const string& other) = 0;
    virtual void calculate_distance_between(const int site_id,const int other_id) = 0;

    /* 
     * This reads in the rotation file on the fly
     */
    virtual void rotate(const string& filename) = 0;

    /*
     * This is the YAML file that contains information about the current
     * Cruxlux cluster, namely where sphinx/mysql cluster nodes are.
     */
    virtual void cluster_config_file(const string& filename) = 0;
    virtual void build() = 0;
    virtual void clear() = 0;

    /* This method dumps out the relationships in sorted order for debugging purposes */
    virtual void full_sorted_output() = 0;
    virtual string statistics() = 0;
    virtual ~RelationshipHolder() {}

  protected:
    static shared_ptr<RelationshipHolder> instance_;
    RelationshipHolder() {}
};

#endif
