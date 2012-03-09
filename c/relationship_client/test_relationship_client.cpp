#include <iostream>
#include <string>
#include <vector>

#include "relationship_client.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_sites_close_to )
{
  RelationshipClient rc("192.168.0.3",5555);
  int_array* ids = rc.sites_close_to(100);
  if (ids != NULL)
    cout << ids->length << endl;
}

BOOST_AUTO_TEST_CASE( distances )
{
  RelationshipClient rc("192.168.0.3",5555);
  vector<int> ids;
  ids.push_back(42);
  ids.push_back(760);
  ids.push_back(42);
  vector<Relationship> rels = rc.distances(ids);
  cout << rels.size() << endl;
}

BOOST_AUTO_TEST_CASE( distances_raw )
{
  RelationshipClient rc("192.168.0.3",5555);
  int_array_t ids;
  ids.array = (uint32_t*)malloc(sizeof(uint32_t) * 3);
  ids.length = 3;
  ids.array[0] = 42;
  ids.array[1] = 760;
  ids.array[2] = 42;
  vector<Relationship> rels = rc.distances(&ids);
  cout << rels.size() << endl;
}
