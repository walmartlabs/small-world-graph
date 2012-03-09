#include <iostream>
#include <string>

#include "wikipedia_sequential_parser.h"
#include "wikipedia_xml_parser.h"

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

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
    //cout << "Unable to open file" << endl;
    exit(-1);
  }
  return content.str();
}

static void parse_to_seq_tree(char* title, char* text, void* traveler)
{
  wiki_seq_tree_t* tree = wiki_seq_parse(text);
  *(wiki_seq_tree_t**) traveler = tree;
  free(title);
  free(text);
}

BOOST_AUTO_TEST_CASE( test_simple_comment )
{
  wiki_seq_tree_t* tree = NULL;
  wikipedia_xml_parse("test/fixtures/text_comment.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;
  int count = 0;
  char* text_str = NULL;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TEXT) {
      wiki_text_t* text = (wiki_text_t*) sec;
      count++;
      text_str = text->text;
    }
  }

  BOOST_REQUIRE_EQUAL(1,count);
  BOOST_REQUIRE_EQUAL(77u,strlen(text_str));
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_redirect )
{
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/stanford.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_link_t* link = NULL;

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == REDIRECT_SECTION) {
      wiki_redirect_t* redi = (wiki_redirect_t*) sec;
      count++;
      link = redi->link;
    }
  }

  BOOST_REQUIRE(link != NULL);
  BOOST_REQUIRE_EQUAL("Stanford_University",link->target);
  BOOST_REQUIRE_EQUAL(1,count);

  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_internal_links )
{
  wiki_link_t* links[6] = {0};
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_internal_links.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == INTERNAL_LINK) {
      links[count] = (wiki_link_t*) sec;
      count++;
    }
  }

  BOOST_REQUIRE_EQUAL(6,count);
  for(int i=0; i < count; i++) {
    BOOST_REQUIRE(links[i] != NULL);
  }

  BOOST_REQUIRE_EQUAL("Basic Link",links[0]->target);
  BOOST_REQUIRE_EQUAL("Alias Link",links[1]->target);
  BOOST_REQUIRE_EQUAL("alias",links[1]->alias);
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_templates )
{
  wiki_template_t* tpls[3] = {0};
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_template.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TEMPLATE) {
      tpls[count] = (wiki_template_t*) sec;
      count++;
    }
  }

  BOOST_REQUIRE_EQUAL(3,count);
  for(int i=0; i < count; i++) {
    BOOST_REQUIRE(tpls[i] != NULL);
  }

  BOOST_REQUIRE_EQUAL("disambig",tpls[0]->title);
  BOOST_REQUIRE_EQUAL("advanced infobox",tpls[1]->title);
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE(test_external_links)
{
  wiki_external_link_t* links[6] = {0};
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_links.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == EXTERNAL_LINK) {
      links[count] = (wiki_external_link_t*) sec;
      count++;
    }
  }

  BOOST_REQUIRE_EQUAL(6,count);
  for(int i=0; i < count; i++) {
    BOOST_REQUIRE(links[i] != NULL);
  }

  BOOST_REQUIRE_EQUAL("http://www.yahoo.com",links[0]->target);
  wiki_text_t* text = (wiki_text_t*)TAILQ_FIRST(&links[0]->sections);
  BOOST_REQUIRE_EQUAL("Link to Yahoo",text->text);

  BOOST_REQUIRE_EQUAL("ftp://ftp.com",links[1]->target);
  text = (wiki_text_t*)TAILQ_FIRST(&links[1]->sections);
  BOOST_REQUIRE_EQUAL("FTP Link",text->text);

  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_empty_link )
{
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/empty_link.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == INTERNAL_LINK) {
      count++;
    }
  }
  BOOST_REQUIRE_EQUAL(1,count);
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_ref )
{
  wiki_ref_t* ref = NULL;
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_ref.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == REF_SECTION) {
      ref = (wiki_ref_t*) sec;
      count++;
    }
  }
  BOOST_REQUIRE_EQUAL(1,count);
  sec = (wiki_section_t*) TAILQ_FIRST(&ref->sections);
  BOOST_REQUIRE(sec);
  BOOST_REQUIRE_EQUAL(TEMPLATE,sec->type);
  wiki_template_t* tpl = (wiki_template_t*) sec;
  BOOST_REQUIRE_EQUAL("cite web",tpl->title);
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_headings)
{
  wiki_heading_t* headings[4] = {0};
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_headers.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == HEADING_SECTION) {
      headings[count] = (wiki_heading_t*) sec;
      count++;
    }
  }
  BOOST_REQUIRE_EQUAL(4,count);

  sec = (wiki_section_t*) TAILQ_FIRST(&headings[0]->sections);
  BOOST_REQUIRE(sec);
  BOOST_REQUIRE_EQUAL(TEXT,sec->type);
  wiki_text_t* text = (wiki_text_t*) sec;
  BOOST_REQUIRE_EQUAL("Normal Section Header",text->text);

  sec = (wiki_section_t*) TAILQ_FIRST(&headings[1]->sections);
  BOOST_REQUIRE(sec);
  BOOST_REQUIRE_EQUAL(TEXT,sec->type);
  text = (wiki_text_t*) sec;

  BOOST_REQUIRE_EQUAL("My Header With Spaces", text->text);

  sec = (wiki_section_t*) TAILQ_FIRST(&headings[2]->sections);
  BOOST_REQUIRE(sec);
  BOOST_REQUIRE_EQUAL(TEXT,sec->type);
  text = (wiki_text_t*) sec;

  BOOST_REQUIRE_EQUAL("My Header Without Spaces", text->text);
  wiki_link_t* link = NULL;
  TAILQ_FOREACH(sec,&headings[3]->sections,entries) {
    if (sec->type == INTERNAL_LINK) {
      link =  (wiki_link_t*) sec;
    }
  }
  BOOST_REQUIRE(link != NULL);
  BOOST_REQUIRE_EQUAL("My Link",link->alias);

  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_formatting )
{
  wiki_text_t* texts[9] = {0};
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_formatting.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);

  wiki_section_t* sec = NULL;
  int count = 0;
  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TEXT) {
      wiki_text_t* text = (wiki_text_t*) sec;
      if (text->text[0] != '\n') {
        //cout << count << ":'" << text->text << "'" << endl;
        texts[count] = text;
        count++;
      }
    }
  }


  BOOST_REQUIRE_EQUAL(9,count);
  BOOST_REQUIRE_EQUAL("Bold",texts[0]->text);
  BOOST_REQUIRE_EQUAL("Italic",texts[1]->text);
  BOOST_REQUIRE_EQUAL("Bold Italic",texts[2]->text);
  BOOST_REQUIRE_EQUAL("End at the new line\nNo more italics ",texts[3]->text);
  BOOST_REQUIRE_EQUAL(" bold til end\n",texts[4]->text);
  BOOST_REQUIRE_EQUAL("Small Formatting",texts[5]->text);
  BOOST_REQUIRE_EQUAL("Big Formatting",texts[6]->text);
  BOOST_REQUIRE_EQUAL("Centered Text",texts[7]->text);

  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_category_links )
{
  wiki_category_link_t* categories[2] = {0};
  wiki_language_link_t* lang_links[2] = {0};
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_category_links.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;
  int cat_count = 0;
  int lang_count = 0;

  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == CATEGORY_LINK_SECTION) {
      categories[cat_count] = (wiki_category_link_t*) sec;
      cat_count++;
    } else if(sec->type == LANGUAGE_LINK_SECTION) {
      lang_links[lang_count] = (wiki_language_link_t*) sec;
      lang_count++;
    }
  }
  BOOST_REQUIRE_EQUAL(2,cat_count);
  BOOST_REQUIRE_EQUAL(2,lang_count);

  BOOST_REQUIRE_EQUAL("People",categories[0]->category);
  BOOST_REQUIRE_EQUAL("People",categories[1]->category);

  BOOST_REQUIRE_EQUAL("en",lang_links[0]->language);
  BOOST_REQUIRE_EQUAL("People",lang_links[0]->target);
  BOOST_REQUIRE_EQUAL("es",lang_links[1]->language);
  BOOST_REQUIRE_EQUAL("persona",lang_links[1]->target);

  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_infobox_whitespace_handling)
{
  wiki_template_t* tpl = NULL;
  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/bill_clinton_infobox.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;

  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TEMPLATE) {
      tpl = (wiki_template_t*) sec;
    }   
  }

  BOOST_REQUIRE(tpl);
  BOOST_REQUIRE_EQUAL("Infobox Officeholder",tpl->title);

  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_unclosed_link)
{
  wiki_text_t* texts[11] = {0};
  wiki_link_t* links[1] = {0};

  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/unclosed_link.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;
  int text_count = 0;
  int link_count = 0;

  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TEXT) {
      texts[text_count] = (wiki_text_t*) sec;
      text_count++;
    } else if(sec->type == INTERNAL_LINK) {
      links[link_count] = (wiki_link_t*) sec;
      link_count++;
    }
  }

  BOOST_REQUIRE_EQUAL(11,text_count);
  BOOST_REQUIRE_EQUAL("[[Barack Obama|Obama promised the ",texts[1]->text);
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_table )
{
  /*
  wiki_table_t* tables[2] = {0};

  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_table.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;
  int table_count = 0;

  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TABLE_SECTION) {
      tables[table_count] = (wiki_table_t*) sec;
      table_count++;
    }
  }

  BOOST_REQUIRE_EQUAL(2u,table_count);
  free_wiki_seq_tree(tree);
  */
}

BOOST_AUTO_TEST_CASE( test_empty_last_row )
{
  wiki_table_t* tables[1] = {0};

  wiki_seq_tree_t* tree;
  wikipedia_xml_parse("test/fixtures/test_empty_last_row_table.xml",parse_to_seq_tree,&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;
  int table_count = 0;

  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TABLE_SECTION) {
      tables[table_count] = (wiki_table_t*) sec;
      table_count++;
    }
  }

  BOOST_REQUIRE_EQUAL(1u,table_count);
  free_wiki_seq_tree(tree);
}

BOOST_AUTO_TEST_CASE( test_yamanote_tables )
{
  wiki_table_t* tables[1] = {0};

  string data = read_file("test/fixtures/yamanote.txt");
  wiki_seq_tree_t* tree = NULL;
  parse_to_seq_tree(strdup((char*)"Yamanote"),(char*) strdup(data.c_str()),&tree);

  BOOST_REQUIRE(tree != NULL);
  wiki_section_t* sec = NULL;
  int table_count = 0;
  int top_level_link_count = 0;
  int row_count = 0;
  wiki_table_t* table = NULL;

  TAILQ_FOREACH(sec,&tree->sections,entries) {
    if (sec->type == TABLE_SECTION) {
      table = (wiki_table_t*) sec;
      tables[table_count] = table;
      table_count++;
    } else if (sec->type == INTERNAL_LINK) {
      top_level_link_count++;
    }
  }
  BOOST_REQUIRE_EQUAL(1u, table_count);
  BOOST_REQUIRE_EQUAL(5u, top_level_link_count);

  wiki_row_t* row;
  TAILQ_FOREACH(row,&table->rows, entries) {
    row_count++;
  }
  BOOST_REQUIRE_EQUAL(33u, row_count);
  free_wiki_seq_tree(tree);
}
