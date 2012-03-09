/*
 * You can run this on an index root, a twitter json, and a trending topics json
 *
 * The twitter JSON should be of the format
 *
 *
 * {
 *    "Name":"@twitter_name",
 *    "Name 2":"@twitter_name2"
 * }
 *
 * The trending topics JSON should be of the format
 *
 * ["Michael Jackson","Southwest Airlines","Jesus"]
 */

#include "wikipedia_sparse_graph.h"
#include <pillowtalk.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

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

int main(int argc, char **argv) {
  if (argc < 4) {
    cout << "Please specify a wikipedia index root, twitter famous people json file, and a list of destination topics" << endl;
    exit(-1);
  }
  char* index_root = argv[1];
  char* famous_json_file = argv[2];
  char* topics_json_file = argv[3];

  WikipediaSparseGraph* graph = WikipediaSparseGraph::instance();
  graph->load(index_root);

  string famous_json = read_file(famous_json_file);
  string topics_json = read_file(topics_json_file);

  pt_node_t* famous_people_map = pt_from_json(famous_json.c_str());
  pt_node_t* topics_array = pt_from_json(topics_json.c_str());

  pt_iterator_t* famous_people_it = pt_iterator(famous_people_map);
  pt_iterator_t* topics_it = pt_iterator(topics_array);

  vector<string> topics;

  pt_node_t* topic = NULL;
  while((topic = pt_iterator_next(topics_it,NULL)) != NULL) {
    const char* topic_str = pt_string_get(topic);
    topics.push_back(topic_str);
  }

  ofstream output("/tmp/chains.html");
  output << "<html>\n  <body>" << endl;
  const char* person_name = NULL;
  pt_node_t* twitter_name = NULL;
  while((twitter_name = pt_iterator_next(famous_people_it,&person_name)) != NULL) {
    cout << "Finding Chain for: " << person_name << endl;
    vector<string> chain;
    vector<string> singleton;
    singleton.push_back(person_name);
    graph->shortest_chain(chain,singleton,topics,5,false,false);
    if (chain.size() > 0) {
      output << "    [";
      for(PageList::const_iterator ii = chain.begin(); ii != chain.end(); ii++) {
        string elem = *ii;
        output << elem;
        if (ii != chain.end() - 1) 
          output << ",";
      }
      output << "], ";
      output << "<a href='http://www.twitter.com/" << pt_string_get(twitter_name) + 1 << "'>" << pt_string_get(twitter_name) << "</a> ";
      output << "<a href='http://www.cruxlux.com/rc/smallworld?a=" << chain[0] << "&b=" << chain[chain.size() - 1] << "'>Link</a><br/>";
      output << endl;
    }
  }

  output << "  </body>\n</html>" << endl;
  output.close();
}
