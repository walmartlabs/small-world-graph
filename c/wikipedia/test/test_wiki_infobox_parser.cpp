#include <iostream>
#include <string>

#include "wiki_infobox_parser.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

static string 
read_file(const string& filename)
{
  stringstream content;
  string line;
  ifstream myfile(filename.c_str());
  if (myfile.is_open())
  {
    while (! myfile.eof() )
    {
      getline (myfile,line);
      content << line << endl;
    }
    myfile.close();
  } else {  
    cout << "Unable to open file" << endl;
    exit(-1);
  }
  return content.str();
}

BOOST_AUTO_TEST_CASE( test_infobox )
{
  /*
  string content = read_file("test/fixtures/obama_infobox.txt");
  char* p = (char*) content.c_str();
  char* pe = p + strlen(p);

  cout << "Parsing:" << p << endl;
  retrieve_images_from_infobox(p,pe);
  */
}

BOOST_AUTO_TEST_CASE( test_image_parser) 
{
  char* content = (char*) "   Baby Face.jpg  ";

  char* p = content;
  char* pe = p + strlen(p);

  char* new_image = image_match(p,pe);
  BOOST_CHECK_EQUAL(new_image, "Baby Face.jpg");
  free(new_image);

  p = (char*) "[[Image:Baby Face.jpg|300px]]";
  pe = p + strlen(p);

  new_image = image_match(p,pe);
  BOOST_CHECK(new_image == NULL);

  p = (char*) " Alain_Connes_in_2004.jpg\n";
  pe = p + strlen(p);
  new_image = image_match(p,pe);
  BOOST_CHECK_EQUAL(new_image,"Alain_Connes_in_2004.jpg");
  free(new_image);

  p = (char*) "Alain_Connes_in_2004.jpg";
  pe = p + strlen(p);
  new_image = image_match(p,pe);
  BOOST_CHECK_EQUAL(new_image,"Alain_Connes_in_2004.jpg");
  free(new_image);

  p = (char*) "John McCain official portrait with alternative background.jpg";
  pe = p + strlen(p);
  new_image = image_match(p,pe);
  BOOST_CHECK_EQUAL(new_image,"John McCain official portrait with alternative background.jpg");
  free(new_image);
}

BOOST_AUTO_TEST_CASE( test_sanitize_title )
{
  char* p;
  char* pe;
  char* sanitized;

  p = (char*) "   M   ";
  pe = p + strlen(p);

  sanitized = sanitize_title(p,pe);
  BOOST_REQUIRE_EQUAL("M", sanitized);
  free(sanitized);

  p = (char*) "   My Bad_ Title\n Cool ";
  pe = p + strlen(p);

  sanitized = sanitize_title(p,pe);

  BOOST_REQUIRE_EQUAL("My Bad  Title  Cool", sanitized);
  free(sanitized);
}
