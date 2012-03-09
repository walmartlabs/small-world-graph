#include <iostream>
#include <string>
#include <vector>

#include "relationship_holder.h"
#include "relationship_glue.h"

#include <arpa/inet.h>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include "benchmark.h"

using namespace std;
using namespace boost::unit_test;


static void send_store(int site_id,int other_id, float distance)
{
  shared_ptr<RelationshipGlue> rg = RelationshipGlue::instance();
  RelationshipRequest* rq = new RelationshipRequest();
  RelationshipResponse* rs;
  unsigned int distance_int = (unsigned int) (distance * 10000);

  rq->payload_size = 12;
  rq->operation= 0;
  rq->payload = (char*) malloc(12);

  *(int*)(rq->payload) = htonl(site_id);
  *(int*)(rq->payload + 4) = htonl(other_id);
  *(int*)(rq->payload + 8) = htonl(distance_int);

  rs = rg->response(rq);
  BOOST_REQUIRE_EQUAL(rs->payload_size,0);
  BOOST_REQUIRE_EQUAL(rs->status,1);
  free(rq);
  free(rs);
}

static vector<int> send_sites_close_to(int site_id)
{
  shared_ptr<RelationshipGlue> rg = RelationshipGlue::instance();
  RelationshipRequest* rq = new RelationshipRequest();
  RelationshipResponse* rs;
  vector<int> results;
  int res_payload_size;

  rq->payload_size = 4;
  rq->operation = 2;
  rq->payload = (char*) malloc(4);
  *(int*)(rq->payload) = htonl(site_id);

  rs = rg->response(rq);

  res_payload_size = rs->payload_size;
  BOOST_REQUIRE_EQUAL(rs->status,1);
  for(int i = 0; i < res_payload_size; i += 4) {
    int close_site = ntohl(*(int*)(rs->payload + i));
    results.push_back(close_site);
  }
  free(rq);
  free(rs);
  return results;
}

static vector<Relationship> send_distances(vector<int> sites)
{
  shared_ptr<RelationshipGlue> rg = RelationshipGlue::instance();
  RelationshipRequest* rq = new RelationshipRequest();
  RelationshipResponse* rs;
  vector<Relationship> results;
  unsigned int res_payload_size;

  rq->payload_size = sites.size() * 4;
  rq->operation = 1;
  rq->payload = (char*) malloc(sites.size() * 4);
  for(unsigned int i=0; i < sites.size(); i++) {
    *(int*)(rq->payload + (i*4)) = htonl(sites[i]);
  }

  rs = rg->response(rq);
  res_payload_size = rs->payload_size;
  BOOST_REQUIRE_EQUAL(rs->status,1);

  for(unsigned int i = 0; i < res_payload_size; i+= 12) {
    Relationship rel;
    unsigned int site_id;
    unsigned int other_id;
    unsigned int distance_int;

    site_id = ntohl(*(int*)(rs->payload + i));
    other_id= ntohl(*(int*)(rs->payload + i + 4));
    distance_int = ntohl(*(int*)(rs->payload + i + 8));

    rel.site_id = site_id;
    rel.other_site_id = other_id;
    rel.distance = ((float)distance_int) / 10000;
    
    results.push_back(rel);
  }
  free(rq);
  free(rs);
  return results;
}

BOOST_AUTO_TEST_CASE( test_large_dataset )
{
  ifstream in_file("/home/cspenc/relationships");
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  if (!in_file) {
    BOOST_FAIL("No File");
  }

  vector<int> site_ids;

  string line;

  bench_start("Store");
  while(!in_file.eof()) {
    int site_id;
    int other_id;
    float distance;
    in_file >> line;
    sscanf(line.c_str(),"%d,%d,%f",&site_id,&other_id,&distance);
    send_store(site_id,other_id,distance);
    site_ids.push_back(site_id);
  }
  bench_finish("Store");

  vector<int> sub_set;
  for(int i=0; i < 1000; i++) {
    sub_set.push_back(site_ids[i]);
  }

  bench_start("100 distance calls");
  for(int i =0; i < 100; i++) {
    send_distances(sub_set);
  }
  bench_finish("100 distance calls");

  bench_start("100 sites_close_to calls");
  for(int i =0; i < 10; i++) {
    for(int j=0; j < 100; j++) {
      send_sites_close_to(sub_set[j]);
    }
  }
  bench_finish("100 sites_close_to calls");
  cout << rh->statistics() << endl;
}
