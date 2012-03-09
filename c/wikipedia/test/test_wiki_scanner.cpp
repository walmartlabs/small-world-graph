#include <iostream>
#include <string>

#include "wiki_scanner.h"

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
    //cout << "Unable to open file" << endl;
    exit(-1);
  }
  return content.str();
}

BOOST_AUTO_TEST_CASE( test_wikipedia_scanner )
{
  string content = read_file("test/fixtures/alain_connes.txt");
  wiki_token_t token;
  char* p = (char*) strdup(content.c_str());
  char* pe = p + strlen(p);
  scan(&token,NULL,p,pe);
  char* current_link;
  while(token.type != END_OF_FILE) {
    ////cout << token.type << endl;
    switch(token.type) {
      case IMAGE_LINK_BEGIN:
        ////cout << "Image Begin:'" << string(token.start,token.stop - token.start) << "'" << endl;
        current_link = token.stop;
        break;
      case LINK_BEGIN:
        ////cout << "Link Begin:'" << string(token.start,token.stop - token.start) << "'" << endl;
        current_link = token.stop;
        break;
      case LINK_END:
        ////cout << "Link End:'" << string(token.start,token.stop - token.start) << "' Link Name: " << string(current_link,token.start - current_link) << endl;
        break;
      case ALNUM:
        //cout << "Alnum:'" << string(token.start,token.stop - token.start) << "'" << endl;
        break;
      case REDIRECT:
        //cout << "Redirect: '" << string(token.start,token.stop - token.start) << "'" << endl;
        break;
      case TEMPLATE_BEGIN:
        //cout << "Template Begin: '" << string(token.start,token.stop - token.start) << "'" << endl;
        break;
      case TEMPLATE_END:
        //cout << "Template End: '" << string(token.start,token.stop - token.start) << "'" << endl;
        break;
      case SEPARATOR:
        //cout << "SEPARATOR: '" << string(token.start,token.stop - token.start) << "'" << endl;
        break;
      case COLON:
        //cout << "COLON: '" << string(token.start,token.stop - token.start) << "'" << endl;
        break;
      default:
        break;
    }
    scan(&token,&token,NULL,pe);
  }
  free(p);
}
