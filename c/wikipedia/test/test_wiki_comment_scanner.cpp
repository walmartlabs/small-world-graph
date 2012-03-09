#include <iostream>
#include <string>

#include "wiki_comment_scanner.h"
#include "wikipedia_xml_parser.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

static void count_comment_blocks(char* title, char* text, void* traveler)
{
  size_t* count = (size_t*) traveler;
  wiki_comment_token_t token;
  char* p = text;
  char* pe = p + strlen(p);
  comment_scan(&token,NULL,p,pe);
  while(token.type != END_OF_FILE) {
    count[token.type]++;
    comment_scan(&token,&token,NULL,pe);
  }
  free(title);
  free(text);
}

BOOST_AUTO_TEST_CASE( test_comment_scanning )
{
  size_t token_counts[SITENAME + 1] = {0};
  wikipedia_xml_parse("test/fixtures/text_comment.xml",count_comment_blocks,token_counts);
  BOOST_REQUIRE_EQUAL(1u,token_counts[COMMENT_BEGIN]);
}

BOOST_AUTO_TEST_CASE( test_magic_word_scanning )
{
  size_t token_counts[SITENAME + 1] = {0};
  wikipedia_xml_parse("test/fixtures/test_links.xml",count_comment_blocks,token_counts);
  BOOST_REQUIRE_EQUAL(1u,token_counts[SERVER]);
}
