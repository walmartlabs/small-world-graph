/*
 * You can run this on an index root and a topics json
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
  if (argc < 3) {
    cout << "Please specify a wikipedia index root, and a list of topics" << endl;
    exit(-1);
  }
  char* index_root = argv[1];
  char* topics_json_file = argv[2];

  WikipediaSparseGraph* graph = WikipediaSparseGraph::instance();
  graph->load(index_root);

  string topics_json = read_file(topics_json_file);
  pt_node_t* topics_array = pt_from_json(topics_json.c_str());

  pt_iterator_t* topics_it = pt_iterator(topics_array);

  vector<string> topics;

  pt_node_t* topic = NULL;
  while((topic = pt_iterator_next(topics_it,NULL)) != NULL) {
    const char* topic_str = pt_string_get(topic);
    topics.push_back(topic_str);
  }

  ofstream output("/tmp/topic_chains.html");
  output << "<html>\n  <body>" << endl;

  for(vector<string>::const_iterator ii = topics.begin(); ii != topics.end(); ii++) {
    string origin = *ii;
    for(vector<string>::const_iterator jj = topics.begin(); jj != topics.end(); jj++) {
      string destination = *jj;
      if (destination != origin) {
        vector<string> chain; 
        vector<string> origin_set;
        vector<string> dest_set;
        origin_set.push_back(origin);
        dest_set.push_back(destination);
        graph->shortest_chain(chain,origin_set,dest_set,5,false,false);
        if (chain.size() > 0) {
          output << "    [";
          for(PageList::const_iterator ii = chain.begin(); ii != chain.end(); ii++) {
            string elem = *ii;
            output << elem;
            if (ii != chain.end() - 1) 
              output << ",";
          }
          output << "], ";
          output << "<a href='http://www.cruxlux.com/rc/smallworld?a=" << chain[0] << "&b=" << chain[chain.size() - 1] << "'>Link</a><br/>";
          output << endl;
        }
      }
    }
  }
  output << "  </body>\n</html>" << endl;
  output.close();
}
