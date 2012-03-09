#ifndef __RELATIONSHIP_CLIENT__
#define __RELATIONSHIP_CLIENT__

#include <string>
#include <vector>
#include <sys/types.h>
#include "cruxlux_types.h"

using namespace std;

struct Relationship {
  int site_id;
  int other_site_id;
  unsigned char distance;
};

class RelationshipClient {
  public:
    RelationshipClient(const string& host, u_int32_t port) : host_(host), port_(port) {}
    struct int_array* sites_close_to(u_int32_t site_id);
    vector<Relationship> distances(const vector<int>& ids);
    vector<Relationship> distances(const int_array_t* ids);

  protected:
    int net_connect();
    string host_;
    u_int32_t port_;
};

#endif
