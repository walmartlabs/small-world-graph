#include <iostream>
#include <string>

#include "wikipedia_page.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

BOOST_AUTO_TEST_CASE( test_construction )
{
  WikipediaImage::Manager* man = WikipediaImage::Manager::instance();

  WikipediaImage* image = man->new_image("World at War.jpg");

  WikipediaImage* image2 = man->new_image("World at War.jpg");

  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"World at War.jpg");
  BOOST_REQUIRE_EQUAL(image,image2);
}


BOOST_AUTO_TEST_CASE( test_link )
{
  WikipediaImage::Manager* man = WikipediaImage::Manager::instance();

  WikipediaImage* image = man->new_image("Alain_Connes_in_2004.jpg");

  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Alain_Connes_in_2004.jpg");
  BOOST_REQUIRE_EQUAL(image->link(), "/b/b8/Alain_Connes_in_2004.jpg");
}

BOOST_AUTO_TEST_CASE( test_link_spaces )
{
  WikipediaImage::Manager* man = WikipediaImage::Manager::instance();

  WikipediaImage* image = man->new_image("Autism-stacking-cans 2nd edit.jpg");

  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Autism-stacking-cans 2nd edit.jpg");
  BOOST_REQUIRE_EQUAL(image->link(), "/d/d1/Autism-stacking-cans_2nd_edit.jpg");
}
