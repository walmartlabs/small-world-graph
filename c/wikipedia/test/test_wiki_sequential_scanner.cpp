#include <iostream>
#include <string>

#include "wiki_preprocessor.h"
#include "wiki_sequential_scanner.h"
#include "wikipedia_xml_parser.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

static void parse(char* title, char* text, void* traveler)
{
  size_t* count = (size_t*) traveler;
  wiki_token_t token;

  char* transformed = wiki_preprocess(text);
  char* p = transformed;
  char* pe = p + strlen(p);
  scan(&token,NULL,p,pe);
  while(token.type != END_OF_FILE) {
    if (token.stop - token.start > 0 && token.type != CRLF) {
      //cout << token_to_human_readable(&token) << "::" << string(token.start,token.stop) << endl;
    } else if (token.type == CRLF) {
      //cout << "CRLF" << endl;
    } else {
      //cout << token_to_human_readable(&token) << "::" << *token.start << endl;
    }
    count[token.type]++;
    scan(&token,&token,NULL,pe);
  }
  free(title);
  free(text);
}

BOOST_AUTO_TEST_CASE( test_template)
{
  size_t token_counts[LAST_TOKEN+ 1] = {0};
  wikipedia_xml_parse("test/fixtures/test_template.xml",parse,token_counts);
  BOOST_REQUIRE_EQUAL(3u,token_counts[TEMPLATE_BEGIN]);
  BOOST_REQUIRE_EQUAL(3u,token_counts[TEMPLATE_END]);
  BOOST_REQUIRE_EQUAL(6u,token_counts[SEPARATOR]);
}

BOOST_AUTO_TEST_CASE( test_list)
{
  size_t token_counts[LAST_TOKEN+ 1] = {0};
  wikipedia_xml_parse("test/fixtures/test_list.xml",parse,token_counts);
}

BOOST_AUTO_TEST_CASE( test_links)
{
  size_t token_counts[LAST_TOKEN+ 1] = {0};
  wikipedia_xml_parse("test/fixtures/test_links.xml",parse,token_counts);
  BOOST_REQUIRE_EQUAL(6u,token_counts[EXTERNAL_LINK_BEGIN]);
  BOOST_REQUIRE_EQUAL(9u,token_counts[EXTERNAL_LINK_END]);
  BOOST_REQUIRE_EQUAL(25u,token_counts[SPACE]);
}

BOOST_AUTO_TEST_CASE( test_category_links)
{
  size_t token_counts[LAST_TOKEN+ 1] = {0};
  wikipedia_xml_parse("test/fixtures/test_category_links.xml",parse,token_counts);
  BOOST_REQUIRE_EQUAL(2u,token_counts[CATEGORY_LINK_BEGIN]);
  BOOST_REQUIRE_EQUAL(2u,token_counts[LANGUAGE_LINK_BEGIN]);
}
