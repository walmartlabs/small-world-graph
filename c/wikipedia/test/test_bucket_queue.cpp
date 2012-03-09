#include <iostream>
#include <string>

#include "wikipedia_sparse_graph.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_insert )
{
  BucketQueue queue(100*30);

  for(int i=100; i >= 0; i--) {
    for(int j=0; j < 30; j++) {
      page_t* page = new page_t();
      page->id = i * j;
      page->title = (char*) "Hello World";
      queue.insert(page,i,1);
    }
  }

  BOOST_REQUIRE_EQUAL(queue.size(), 3030u);
  shortest_path_t* path;

  while((path = queue.delete_min()));

  BOOST_REQUIRE_EQUAL(queue.size(), 0u);
}

BOOST_AUTO_TEST_CASE( test_decrease_key)
{
  BucketQueue queue(3);

  page_t* curtis = new page_t();
  curtis->id = 1;
  curtis->title = (char*) "Curtis";
  queue.insert(curtis,10,1);

  page_t* guha = new page_t();
  guha->id = 2;
  guha->title = (char*) "Guha";
  queue.insert(guha,30,1);

  page_t* sarah = new page_t();
  sarah->id = 3;
  sarah->title = (char*) "Sarah";
  queue.insert(sarah,60,1);

  BOOST_REQUIRE_EQUAL(queue.size(),3u);

  shortest_path_t* path;
  path = queue.delete_min();

  BOOST_REQUIRE_EQUAL(path->end->title,"Curtis");

  queue.decrease_key(sarah,1,1);

  BOOST_REQUIRE_EQUAL(queue.size(),2u);

  path = queue.delete_min();
  BOOST_REQUIRE_EQUAL(path->end,sarah);
  BOOST_REQUIRE_EQUAL(path->distance,1u);

  BOOST_REQUIRE_EQUAL(queue.size(),1u);

  path = queue.delete_min();
  BOOST_REQUIRE_EQUAL(path->end,guha);

  queue.decrease_key(curtis,5,1);
  path = queue.delete_min();
  BOOST_REQUIRE(path == NULL);

  path = queue.d(curtis);
  BOOST_REQUIRE_EQUAL(path->end, curtis);
  BOOST_REQUIRE_EQUAL(path->distance, 5u);


  free(curtis);
  free(guha);
  free(sarah);
}
