#include <iostream>
#include <string>
#include <exception>

#include "wiki_preprocessor.h"
#include "wikipedia_xml_parser.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

static string project_root()
{
  char my_path[4096];  
  getcwd(my_path,4096); 
  string full_path = string(my_path) + "/" + string(boost::unit_test::framework::master_test_suite().argv[0]);
  size_t bin_start = full_path.find("/bin/");
  if (bin_start != string::npos)
    return full_path.substr(0,bin_start);
  else
    throw std::runtime_error("Couldn't find root path");
}

static void preprocess(char* title, char* text, void* traveler)
{
  char** result = (char**) traveler;
  *result = wiki_preprocess(text);
  free(title);
  free(text);
}

BOOST_AUTO_TEST_CASE( test_text_comment )
{
  char* plaintext = NULL;
  wikipedia_xml_parse((project_root() + "/test/fixtures/text_comment.xml").c_str(),preprocess,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(77u,strlen(plaintext));
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_complex )
{
  char* plaintext = NULL;
  wikipedia_xml_parse((project_root() + "/test/fixtures/bill_clinton.xml").c_str(),preprocess,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(121999u,strlen(plaintext));
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_magic_word)
{
  char* plaintext = NULL;
  wikipedia_xml_parse((project_root() + "/test/fixtures/test_links.xml").c_str(),preprocess,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  cout << plaintext << endl;
  free(plaintext);
}
