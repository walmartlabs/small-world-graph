#include <iostream>
#include <string>
#include <vector>

#include "relationship_holder.h"
#include <sys/resource.h>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "benchmark.h"
#include "sixty_four_bit_hash.h"

using namespace std;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_configuration )
{
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  rh->cluster_config_file("cluster_benchmark.yml");
}

BOOST_AUTO_TEST_CASE( test_sites_close_to )
{
  float distance;
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  CloseSiteVector results;
  vector<SiteRelationship> rels;
  vector<int> sites;

  /* First test some key stuff */
  u_int64_t key = composite_key(100,200);
  struct decomposed_key dk = decompose_key(key);
  BOOST_REQUIRE_EQUAL(dk.site_id,100);
  BOOST_REQUIRE_EQUAL(dk.other_id ,200);

  /* First test some key stuff */
  key = composite_key(10000000,200000000);
  dk = decompose_key(key);
  BOOST_REQUIRE_EQUAL(dk.site_id,10000000);
  BOOST_REQUIRE_EQUAL(dk.other_id ,200000000);

  rh->store(1,2,127);
  rh->store(3,4,64);
  rh->store(1,5,32);
  rh->store(5,3,50);

  BOOST_REQUIRE_EQUAL(rh->size(),(unsigned int) 4);

  distance = rh->distance_between(1,2);
  BOOST_CHECK_EQUAL(distance,127);

  distance = rh->distance_between(1,3);
  BOOST_CHECK_EQUAL(distance,255);

  distance = rh->distance_between(3,4);
  BOOST_CHECK_EQUAL(distance,64);

  /* Now check bidirectional */
  distance = rh->distance_between(4,3);
  BOOST_CHECK_EQUAL(distance,64);

  /* Now check sites close to */
  rh->sites_close_to(results,1);
  BOOST_CHECK_EQUAL(results.size(),(unsigned int)2);
  results.clear();

  /* Now check distance tuples */
  rh->sites_close_to(results,1);
  BOOST_CHECK_EQUAL(results.size(),(unsigned int)2);
  results.clear();

  sites.clear();
  sites.push_back(3);
  sites.push_back(1);
  sites.push_back(5);
  rh->distances(rels,sites);
  BOOST_CHECK_EQUAL(rels.size(),(unsigned int)2);

  sites.clear();
  sites.push_back(2);
  sites.push_back(1);
  sites.push_back(5);
  sites.push_back(3);

  rels.clear();
  rh->distances(rels,sites);
  BOOST_REQUIRE_EQUAL(rels.size(),(unsigned int)3);
  BOOST_CHECK_EQUAL(rels[0].first,1);
  BOOST_CHECK_EQUAL(rels[0].second,2);
  BOOST_CHECK_EQUAL(rels[0].distance,127);
  BOOST_CHECK_EQUAL(rels[1].first,1);
  BOOST_CHECK_EQUAL(rels[1].second,5);
  BOOST_CHECK_EQUAL(rels[1].distance,32);
  BOOST_CHECK_EQUAL(rels[2].first,3);
  BOOST_CHECK_EQUAL(rels[2].second,5);
  BOOST_CHECK_EQUAL(rels[2].distance,50);
  rels.clear();

  /* Let's test an update */
  rh->store(1,2,20);
  sites.clear();
  sites.push_back(1);
  sites.push_back(2);
  rh->distances(rels,sites);
  BOOST_REQUIRE_EQUAL(rels.size(), (unsigned int) 1);
  BOOST_CHECK_EQUAL(rels[0].first,1);
  BOOST_CHECK_EQUAL(rels[0].second,2);
  BOOST_CHECK_EQUAL(rels[0].distance,20);

}

BOOST_AUTO_TEST_CASE( deletion ) {
  unsigned char distance;
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  CloseSiteVector results;
  vector<SiteRelationship> rels;
  vector<int> sites;

  rh->store(1,2,127);
  rh->store(3,4,64);
  rh->store(1,5,32);
  rh->store(5,3,50);

  /* Let's test a deletion */
  rh->destroy(1,2);
  distance = rh->distance_between(1,2);
  BOOST_CHECK_EQUAL(distance,255);
  rh->sites_close_to(results,1);
  BOOST_CHECK_EQUAL(results.size(),1u);
  results.clear();
  rh->sites_close_to(results,2);
  BOOST_CHECK_EQUAL(results.size(),0u);
  results.clear();

  /* Now another one to bring it down to 0 */
  rh->destroy(1,5);
  distance = rh->distance_between(1,2);
  BOOST_CHECK_EQUAL(distance,255);
  rh->sites_close_to(results,1);
  BOOST_CHECK_EQUAL(results.size(),0u);

}

BOOST_AUTO_TEST_CASE( uniqueness ) {
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  CloseSiteVector results;
  vector<SiteRelationship> rels;
  vector<int> sites;

  /* First test some key stuff */
  u_int64_t key = composite_key(100,200);
  struct decomposed_key dk = decompose_key(key);
  BOOST_CHECK_EQUAL(dk.site_id,100);
  BOOST_CHECK_EQUAL(dk.other_id ,200);

  rh->clear();

  rh->store(1,2,127);
  rh->store(3,4,64);
  rh->store(1,5,32);
  rh->store(5,3,50);

  /* Now check sites close to */
  rh->sites_close_to(results,1);
  BOOST_CHECK_EQUAL(results.size(),(unsigned int)2);
  results.clear();

  /* Now ensure the number doesn't go up */
  rh->store(1,2,240);
  rh->sites_close_to(results,1);
  BOOST_REQUIRE_EQUAL(rh->distance_between(1,2),240);
  BOOST_REQUIRE_EQUAL(results.size(),(unsigned int)2);
}

BOOST_AUTO_TEST_CASE( persistence_test ) {
  unsigned char distance;
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  CloseSiteVector results;
  vector<SiteRelationship> rels;
  vector<int> sites;

  rh->clear();

  rh->store(1,2,127);
  rh->store(3,4,64);
  rh->store(1,5,32);
  rh->store(5,3,50);

  rh->sites_close_to(results,1);
  BOOST_REQUIRE_EQUAL(results.size(),(unsigned int)2);
  results.clear();


  rh->persist("/tmp/persistence_test");

  rh->clear();

  rh->load("/tmp/persistence_test");

  distance = rh->distance_between(1,2);
  BOOST_CHECK_EQUAL(distance,127);

  distance = rh->distance_between(1,3);
  BOOST_CHECK_EQUAL(distance,255);

  distance = rh->distance_between(3,4);
  BOOST_CHECK_EQUAL(distance,64);

  /* Now check bidirectional */
  distance = rh->distance_between(4,3);
  BOOST_CHECK_EQUAL(distance,64);

  /* Now check sites close to */
  rh->sites_close_to(results,1);
  BOOST_REQUIRE_EQUAL(results.size(),(unsigned int)2);
  results.clear();

  /* Now check distance tuples */
  rh->sites_close_to(results,1);
  BOOST_REQUIRE_EQUAL(results.size(),(unsigned int)2);
  results.clear();
}

BOOST_AUTO_TEST_CASE( destroy_test ) {
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  CloseSiteVector results;
  rh->store(1,2,127);
  rh->store(3,4,64);
  rh->store(1,5,32);
  rh->store(5,3,50);

  BOOST_REQUIRE_EQUAL(rh->size(), (unsigned int) 4);

  rh->destroy(1,2);

  BOOST_REQUIRE_EQUAL(rh->size(), (unsigned int) 3);

  rh->sites_close_to(results,1);
  BOOST_REQUIRE_EQUAL(results.size(), (unsigned int) 1);
  rh->destroy(1,2);
  rh->destroy(3,4);
  rh->destroy(1,5);
  rh->destroy(5,3);

  BOOST_REQUIRE_EQUAL(rh->size(), (unsigned int) 0);
}

BOOST_AUTO_TEST_CASE( build_test ) {
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  rh->cluster_config_file("cluster_test.yml");
  rh->clear();
  rh->build();
  BOOST_CHECK_EQUAL(rh->size(),694677u);
  rh->persist("/tmp/persisted_test");
  rh->store(998,999,50);
  BOOST_CHECK_EQUAL(rh->size(),694678u);
  rh->rotate("/tmp/persisted_test");
  BOOST_CHECK_EQUAL(rh->size(),694677u);
}
