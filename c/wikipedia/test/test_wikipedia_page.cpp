#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "wikipedia_page.h"

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

struct WikiPageFixture {
  WikiPageFixture() {
    WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
    man->clear();
  }

  ~WikiPageFixture() {
  }
};

BOOST_FIXTURE_TEST_SUITE(s, WikiPageFixture );

BOOST_AUTO_TEST_CASE( test_alias )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  WikipediaPage* page = man->new_page("AmericanSamoa","#REDIRECT [[American Samoa]]{{R from CamelCase}}");
  BOOST_REQUIRE(page == NULL);
}

BOOST_AUTO_TEST_CASE( test_info_box )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/alain_connes.txt");
  WikipediaPage* page = man->new_page("Alain Connes",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE(!page->ambiguous());
  BOOST_REQUIRE_EQUAL(image->title(),"Alain_Connes_in_2004.jpg");
}

BOOST_AUTO_TEST_CASE( test_embedded_info_box )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/agricultural_science.txt");
  WikipediaPage* page = man->new_page("Agricultural science",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Cropscientist.jpg");
}

BOOST_AUTO_TEST_CASE( test_spaces )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/autism.txt");
  WikipediaPage* page = man->new_page("Autism",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Autism-stacking-cans 2nd edit.jpg");
}

BOOST_AUTO_TEST_CASE( test_weird_pipe )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/actrius.txt");
  WikipediaPage* page = man->new_page("Actrius",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image == NULL);
}

BOOST_AUTO_TEST_CASE( test_free_image )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/animalia.txt");
  WikipediaPage* page = man->new_page("Animalia",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Animalia.jpg");
}

// Alabama has an info box as well as internal images
BOOST_AUTO_TEST_CASE( test_alabama )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/alabama.txt");
  WikipediaPage* page = man->new_page("Alabama",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Flag of Alabama.svg");
}

BOOST_AUTO_TEST_CASE( test_amoeboid )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/amoeboid.txt");
  WikipediaPage* page = man->new_page("Amoeboid",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Chaos diffluens.jpg");
}

BOOST_AUTO_TEST_CASE( test_vorbis)
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/vorbis.txt");
  WikipediaPage* page = man->new_page("Vorbis",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"XiphophorusLogoSVG.svg");
}

BOOST_AUTO_TEST_CASE( test_austria )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/austria.txt");
  WikipediaPage* page = man->new_page("Austria",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Flag of Austria.svg");
}

BOOST_AUTO_TEST_CASE( test_obama )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/obama.txt");
  WikipediaPage* page = man->new_page("Barack Obama",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"Barack Obama.jpg");
}

BOOST_AUTO_TEST_CASE( test_san_francisco )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/san_francisco.txt");
  WikipediaPage* page = man->new_page("San Francisco",content);
  BOOST_REQUIRE(page != NULL);
  WikipediaImage* image = page->representative_image();
  BOOST_REQUIRE(image != NULL);
  BOOST_REQUIRE_EQUAL(image->title(),"SF From Marin Highlands3.jpg");
}

BOOST_AUTO_TEST_CASE( test_load )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  man->load("test/fixtures/small_articles.xml");
  //BOOST_REQUIRE_EQUAL(217u,man->size());
  //BOOST_REQUIRE_EQUAL(217u,man->size());
  shared_ptr<WikipediaPage::Manager::Iterator> it = man->iterator();
  size_t image_count = 0;
  while(WikipediaPage* page = it->next()) {
    WikipediaImage* image = page->representative_image();
    if (image) {
      cout << page->title() << "->" << image->title() << endl;
      image_count++;
    } else {
    }
  }
  BOOST_REQUIRE_EQUAL(21u,image_count);

  WikipediaPage* alias = man->page("AmoeboidTaxa");
  BOOST_REQUIRE(alias != NULL);
  BOOST_REQUIRE_EQUAL("Amoeboid", alias->title());
}

BOOST_AUTO_TEST_CASE( test_bush )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/bush.txt");
  WikipediaPage* page = man->new_page("Bush",content);
  BOOST_REQUIRE(page != NULL);
  BOOST_REQUIRE(page->ambiguous());
}

BOOST_AUTO_TEST_CASE( test_clinton )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/clinton.txt");
  WikipediaPage* page = man->new_page("Clinton",content);
  BOOST_REQUIRE(page != NULL);
  BOOST_REQUIRE(page->ambiguous());
}

BOOST_AUTO_TEST_CASE( test_anchor_in_link )
{
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  string content = read_file("test/fixtures/clinton.txt");
  WikipediaPage* page = man->new_page("Anchor","[[Title#First Section|my title]]");
  BOOST_REQUIRE(page != NULL);
  WikipediaRawLinkList* raws = page->raw_outlinks();
  BOOST_REQUIRE_EQUAL(raws->size(), 1u);
  BOOST_REQUIRE_EQUAL(raws->front().name,"Title");
}

BOOST_AUTO_TEST_CASE( test_ambiguous_first_name)
{
  cout << "Start Chris" << endl;
  WikipediaPage::Manager* man = WikipediaPage::Manager::instance();
  man->load("test/fixtures/chris.xml");
  WikipediaPage* page = man->page("Chris");
  BOOST_REQUIRE(page != NULL);
  BOOST_REQUIRE(page->ambiguous());
  cout << "End Chris" << endl;
}

BOOST_AUTO_TEST_SUITE_END();
