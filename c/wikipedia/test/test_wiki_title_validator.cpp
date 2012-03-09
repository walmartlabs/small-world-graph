#include <iostream>
#include <string>

#include "wiki_title_validator.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_title )
{
  char* p = NULL;
  char* pe = NULL;
  bool useful = false;

  p = (char*) "Curtis Spencer";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(useful);

  p = (char*) "Template:Weird Stuff";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);

  p = (char*) "2008";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);

  p = (char*) "January 14";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);

  p = (char*) "Dec 14";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);

  p = (char*) "14";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);

  p = (char*) "2 (number)";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);

  p = (char*) "February";
  pe = p + strlen(p);
  useful = useful_title(p,pe);
  BOOST_CHECK(!useful);
}

