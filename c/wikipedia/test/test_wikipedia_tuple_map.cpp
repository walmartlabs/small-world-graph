#include <iostream>
#include <string>

#include "wikipedia_tuple_map.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

struct TupleLoad {
  TupleLoad() {
    cout << "Global Loadup" << endl;
    WikipediaTupleMap* map = WikipediaTupleMap::instance();
    map->load("test/fixtures/tiny_index"); 
  }

  ~TupleLoad() {
    cout << "Global Cleanup" << endl;
    WikipediaTupleMap* map = WikipediaTupleMap::instance();
    map->clear();
    delete map;
  }
};

BOOST_GLOBAL_FIXTURE( TupleLoad );

BOOST_AUTO_TEST_CASE( test_load )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  canonical_tuple_t* canon = map->tuple_exists("Jesus Christ");
  BOOST_REQUIRE(canon);
  BOOST_REQUIRE_EQUAL(canon->image_link,"/images/Jesus_Christ.png");
  BOOST_REQUIRE_EQUAL(canon->tuple,"Jesus Christ");
  BOOST_REQUIRE(!canon->ambiguous);
}

BOOST_AUTO_TEST_CASE( test_redirect )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  canonical_tuple_t* first = map->tuple_exists("Ambiguous Canonical");
  BOOST_REQUIRE(first);
  BOOST_REQUIRE(first->ambiguous);
  BOOST_REQUIRE_EQUAL(first->tuple, "Ambiguous Canonical");

  canonical_tuple_t* canon = map->tuple_exists("Ambiguous");
  BOOST_REQUIRE(canon);
  BOOST_REQUIRE(canon->ambiguous);
  BOOST_REQUIRE_EQUAL(canon->tuple,"Ambiguous Canonical");
}

BOOST_AUTO_TEST_CASE( test_ambiguous )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  canonical_tuple_t* canon = map->tuple_exists("Ambiguous Canonical");
  BOOST_REQUIRE(canon);
  BOOST_REQUIRE(canon->ambiguous);
  BOOST_REQUIRE_EQUAL(canon->tuple,"Ambiguous Canonical");
}

BOOST_AUTO_TEST_CASE( test_whitespace )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  TupleImageList list;
  map->tuples_from_snippet("        Steve Jobs        ",list);
  BOOST_CHECK_EQUAL(list.size(), 1u);
  BOOST_CHECK_EQUAL(list[0].canonical->tuple,"Steve Jobs");
  BOOST_CHECK_EQUAL(list[0].canonical->image_link,"/images/SteveJobs.png");
}

BOOST_AUTO_TEST_CASE( test_single_tuple_in_snippet )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  TupleImageList list;
  map->tuples_from_snippet("Apple",list);
  BOOST_CHECK_EQUAL(list.size(), 1u);
  BOOST_CHECK_EQUAL(list[0].canonical->tuple,"Apple");
  BOOST_CHECK_EQUAL(list[0].canonical->image_link,"/images/redapple.png");
}

BOOST_AUTO_TEST_CASE( test_double_tuple_in_snippet )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  TupleImageList list;
  map->tuples_from_snippet("Steve Jobs",list);
  BOOST_CHECK_EQUAL(list.size(), 1u);
  BOOST_CHECK_EQUAL(list[0].canonical->tuple,"Steve Jobs");
  BOOST_CHECK_EQUAL(list[0].canonical->image_link,"/images/SteveJobs.png");
  list.clear();
  map->tuples_from_snippet("Apple Mac",list);
  BOOST_CHECK_EQUAL(list.size(), 0u);
}

BOOST_AUTO_TEST_CASE( test_tuples_from_snippet )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  TupleImageList list;
  map->tuples_from_snippet("Does Steve Jobs from Apple Even Care About The Creative Commons License",list);
  BOOST_REQUIRE_EQUAL(list.size(), 2u);
  BOOST_CHECK_EQUAL(list[0].canonical->tuple,"Creative Commons License");
  BOOST_CHECK_EQUAL(list[1].canonical->tuple,"Steve Jobs");
  list.clear();
  map->tuples_from_snippet("Steve Jobs Is Taking A Leave Of Absence From Apple Due To His Health",list);
  BOOST_CHECK_EQUAL(list.size(), 1u);
//  BOOST_CHECK_EQUAL(list[0].tuple,"Steve Jobs");
//  BOOST_CHECK_EQUAL(list[0].image_link,"/images/SteveJobs.png");
//  BOOST_CHECK_EQUAL(list[1].tuple,"Apple");
//  BOOST_CHECK_EQUAL(list[1].image_link,"/images/redapple.png");
}

// We have to test here that if we have a word like Jesus that resolves to a
// multiple word canonical tuple then we keep it
BOOST_AUTO_TEST_CASE( test_multi_word_canonical_resolution )
{
  WikipediaTupleMap* map = WikipediaTupleMap::instance();
  TupleImageList list;
  map->tuples_from_snippet("AIG Is Getting Bailed By The Fed Hank is Happy",list);
  BOOST_REQUIRE_EQUAL(list.size(),1u);
  alias_canonical_pair_t aig = list[0];
  BOOST_REQUIRE_EQUAL(aig.alias,"AIG");
  BOOST_REQUIRE_EQUAL(aig.canonical->tuple,"American Insurance Group");
}


