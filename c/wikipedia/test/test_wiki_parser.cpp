#include <iostream>
#include <string>

#include "wiki_parser.h"
#include "wikipedia_xml_parser.h"

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

static void parse_to_tree(char* title, char* text, void* traveler)
{
  wiki_parse_tree_t* tree = wiki_parse(text);
  *(wiki_parse_tree_t**) traveler = tree;
}

static void convert_to_text(char* title, char* text, void* traveler)
{
  char* plaintext = wikitext_to_plaintext(text);
  *(char**) traveler = plaintext;
}

BOOST_AUTO_TEST_CASE( test_obama )
{
  string content = read_file("test/fixtures/obama.txt");
  char* p = (char*) content.c_str();
  wiki_parse_tree_t* tree = wiki_parse(p);
  //print_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_anarchism )
{
  string content = read_file("test/fixtures/anarchism.txt");
  char* p = (char*) content.c_str();
  wiki_parse_tree_t* tree = wiki_parse(p);
  //print_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_catherine_howard)
{
  string content = read_file("test/fixtures/catherine.txt");
  char* p = (char*) content.c_str();
  //printff("Start Catherine Howard\n");
  wiki_parse_tree_t* tree = wiki_parse(p);
  //print_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_messed_up)
{
  string content = read_file("test/fixtures/messed_up.txt");
  char* p = (char*) content.c_str();
  wiki_parse_tree_t* tree = wiki_parse(p);
  BOOST_REQUIRE(tree->redirect == NULL);
  BOOST_REQUIRE_EQUAL(0u,tree->outlinks.len);
  BOOST_REQUIRE_EQUAL(0u,tree->image_links.len);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_bad_link )
{
  string content = read_file("test/fixtures/bad_link.txt");
  char* p = (char*) content.c_str();
  wiki_parse_tree_t* tree = wiki_parse(p);
  BOOST_REQUIRE_EQUAL(9u,tree->outlinks.len);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_math )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/math.xml",parse_to_tree,&tree);
}

BOOST_AUTO_TEST_CASE( test_current_image )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/mavericks.xml",parse_to_tree,&tree);
  BOOST_REQUIRE_EQUAL(tree->outlinks.len,215u);
  BOOST_REQUIRE_EQUAL(tree->image_links.len,2u);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_comment_wrapped_image )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/image_comment.xml",parse_to_tree,&tree);
  //print_tree(tree);
  BOOST_REQUIRE_EQUAL(tree->image_links.len,0u);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_strange_unicode )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/strange_unicode.xml",parse_to_tree,&tree);
  //print_tree(tree);
  //BOOST_REQUIRE_EQUAL(tree->image_links.len,0);
  BOOST_REQUIRE_EQUAL(tree->image_links.links[0]->name,"ORL-DisneyRRStat.JPG");
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_lowercase_redirect )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/redirect.xml",parse_to_tree,&tree);
  BOOST_REQUIRE(tree->redirect != NULL);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_stanford_redirect )
{
  wiki_parse_tree_t* tree = NULL;
  wikipedia_xml_parse("test/fixtures/stanford.xml",parse_to_tree,&tree);
  BOOST_REQUIRE(tree != NULL);
  BOOST_REQUIRE_EQUAL(tree->redirect,"Stanford University");
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_john_mccain)
{
  wiki_parse_tree_t* tree = NULL;
  wikipedia_xml_parse("test/fixtures/johnmccain.xml",parse_to_tree,&tree);
  BOOST_REQUIRE(tree != NULL);
  //print_parse_tree(tree);
  BOOST_REQUIRE_EQUAL(tree->image_links.len,17u);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_complex_wikitext_to_plaintext )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/johnmccain.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(strlen(plaintext),63741u);
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_wikitext_to_plaintext )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/simple_entry.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(strlen(plaintext),382u);
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_simple_mccain )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/simplemccain.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(strlen(plaintext),1446u);
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_tiger )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/tiger.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(strlen(plaintext),2331u);
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_strawberry_shortcake)
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/strawberry_shortcake.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(strlen(plaintext),3257u);
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_redirect )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/stanford.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext == NULL);
}

BOOST_AUTO_TEST_CASE( test_autism )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/autism.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  BOOST_REQUIRE_EQUAL(strlen(plaintext),44462u);
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_sheryl_crow )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/sheryl_crow.xml",parse_to_tree,&tree);
  //print_parse_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_juan )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/juan.xml",parse_to_tree,&tree);
  //print_parse_tree(tree);
  BOOST_REQUIRE(tree->ambiguous);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_bill_clinton )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/bill_clinton.xml",parse_to_tree,&tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_chelsea_clinton )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/chelsea_clinton.xml",parse_to_tree,&tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_simple_bill_clinton)
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/simple_bill_clinton.xml",parse_to_tree,&tree);
  //print_parse_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_simple_sugar )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/simple_sugar.xml",parse_to_tree,&tree);
  //print_parse_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_sugarcane )
{
  wiki_parse_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/sugarcane.xml",parse_to_tree,&tree);
  //print_parse_tree(tree);
  free_parse_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_proper_anchor_conversion )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/new_obama.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  free(plaintext);
}


BOOST_AUTO_TEST_CASE( test_addition)
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/addition.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  //cout << plaintext << endl;
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_trouble_parts_of_addition)
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/bad_parts_of_addition.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  //cout << plaintext << endl;
  free(plaintext);
}

BOOST_AUTO_TEST_CASE( test_adobe_illustrator )
{
  char* plaintext = NULL;
  wikipedia_xml_parse("test/fixtures/adobe_illustrator.xml",convert_to_text,&plaintext);
  BOOST_REQUIRE(plaintext != NULL);
  free(plaintext);
}
