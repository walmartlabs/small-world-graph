
#ifndef __RELATIONSHIP_GLUE
#define __RELATIONSHIP_GLUE

#include <string>
#include <exception>
#include <boost/shared_ptr.hpp>
#include "relationship_holder.h"

using namespace std;
using namespace boost;

class RelationShipProtocolException : public exception {
};

class RelationshipRequest {
  public:
    /* Network layer needs to take care of setting these in host endianess */
    int operation;
    int payload_size;
    /* This is big endian in the payload */
    char* payload;
};

class RelationshipResponse {
  public:
    /* Network layer needs to take care of setting these in host endianess */
    int status;
    int payload_size;
    /* This is big endian in the payload */
    char* payload;
};

class RelationshipGlue {
  public:
    static const unsigned int HEADER_SIZE = 8;
    static shared_ptr<RelationshipGlue> instance(); 
    RelationshipResponse* response(RelationshipRequest* rq);

  protected:
    static shared_ptr<RelationshipGlue> instance_;
    shared_ptr<RelationshipHolder> rh_;
    RelationshipGlue() {
      rh_ = RelationshipHolder::instance();
    }

    RelationshipResponse* store(RelationshipRequest* rm);
    RelationshipResponse* distances(RelationshipRequest* rm);
    RelationshipResponse* sites_close_to(RelationshipRequest* rm);
    RelationshipResponse* rotate(RelationshipRequest* rm);
    RelationshipResponse* destroy(RelationshipRequest* rm);
};

#endif
