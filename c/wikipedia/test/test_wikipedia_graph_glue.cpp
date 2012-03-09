#include "wikipedia_graph_glue.h"
#include "wikipedia_graph.h"

#include <iostream>
#include <string>
#include <arpa/inet.h>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
//#include <boost/test/floating_point_comparison.hpp>

using namespace std;
using namespace boost::unit_test;

static void send_distance(list<const char*>& tuples)
{
  graph_request_t* req = (graph_request_t*) malloc(sizeof(graph_request_t));
  req->operation = 0;

  uint32_t payload_size = 4;
  for(list<const char*>::const_iterator ii = tuples.begin(); ii != tuples.end(); ii++) {
    payload_size += 4 + strlen(*ii) + 1;
  }

  req->payload = (char*) malloc(payload_size);
  req->payload_size = payload_size;
  cout << "Payload Size: " << req->payload_size << endl;
  char* cur = req->payload;

  *(int*)(cur) = htonl(tuples.size());
  cur += 4;

  for(list<const char*>::const_iterator ii = tuples.begin(); ii != tuples.end(); ii++) {
    const char* tuple = *ii;
    *(int*)(cur) = htonl(strlen(tuple) + 1);
    cur += 4;
    memcpy(cur,tuple,strlen(tuple) + 1);
    cur += strlen(tuple) + 1;
  }

  cout << "Sending Distance Payload" << endl;

  graph_response_t* res = graph_response(req);
  BOOST_REQUIRE(res != NULL);
}

BOOST_AUTO_TEST_CASE( test_distance )
{
  list<const char*> tuples;
  WikipediaGraph* graph = WikipediaGraph::instance();
  graph->load("test/fixtures/cruxlux_graph.txt");

  tuples.push_back("Curtis Spencer");
  tuples.push_back("Guha Jayachandran");
  tuples.push_back("Barack Obama");
  send_distance(tuples);
}
