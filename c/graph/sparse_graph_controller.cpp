#include <stdio.h>
#include <string.h>
#include "chibi.h"
#include <pillowtalk.h>

#include "sparse_graph.h"

#define MAX_PATH_LENGTH 6

/* Prototypes */
static char* path_to_json(const Chain& chain,const char* start, const char* end);
static void split_into_vector(vector<string>& vec, const char* input);


static void status(chibi handle, const chb_request req, chb_response res, void* vstar)
{
  SparseGraph* graph = (SparseGraph*) chb_global_get(handle,"graph");
  if (graph) {
    chb_rs_set(res,strdup("SUPERDUPERON"));
  } else {
    chb_rs_set(res,strdup("INPAIN"));
  }
}

static void invalid_api(chibi handle, const chb_request req, chb_response res, void* vstar)
{
  chb_rs_set(res,strdup("Invalid API Call"));
}

static void edges(chibi handle, const chb_request req, chb_response res, void* vstar)
{
  const char* query = chb_rq_param_get(req,"q");
  if (query) {
    chb_rs_set(res,strdup(query));
  } else
    chb_rs_set(res,strdup("[]"));
}

static void path(chibi handle, const chb_request req, chb_response res, void* vstar)
{
  const char* a = chb_rq_param_get(req,"a");
  const char* b = chb_rq_param_get(req,"b");
  SparseGraph* graph = (SparseGraph*) chb_global_get(handle,"graph");

  if (graph && a && b) {
    vector<string> a_set;
    split_into_vector(a_set,a);

    vector<string> b_set;
    split_into_vector(b_set, b);

    Chain chain;
    const char* resolved_a = NULL;
    const char* resolved_b = NULL;
    graph->shortest_chain(chain,a_set,b_set,MAX_PATH_LENGTH,false,false, &resolved_a,&resolved_b);

    char* json = path_to_json(chain,resolved_a,resolved_b);

    chb_rs_set(res,json);

  } else {
    chb_rs_set(res,strdup("[]"));
  }
}

extern "C" void chibi_init(chibi handle) {
  pt_init();
  SparseGraph* graph = SparseGraph::instance();
  const char* graph_file = chb_prop_get(handle,"graph_file");
  if (graph_file)
    graph->load(graph_file);
  else
    chb_abort("No Graph File specified");


  chb_global_set(handle,"graph",graph);
  chb_router_register(handle,"/path",path);
  chb_router_register(handle,"/edges",edges);
  chb_router_register(handle,"/status",status);
  chb_router_register(handle,"/api/chain",path);
  chb_router_register_generic(handle,invalid_api);
}


// -------------------------------------
// ------ Private Implementation -------
// -------------------------------------

/*
 * This function takes an A and B and finds the path between them
 */
static char* path_to_json(const Chain& chain, const char* start, const char* end)
{
  pt_node_t* result = pt_map_new();
  pt_node_t* payload = pt_map_new();
  pt_node_t* edges = pt_array_new();
  char* cur_start = (char*) start;
  for(Chain::const_iterator ii = chain.begin(); ii != chain.end(); ii++) {
    edge_t* edge = *ii;
    pt_node_t* chainlink = pt_map_new();
    if (!strcmp(cur_start,edge->begin->title)) {
      pt_map_set(chainlink,"start", pt_string_new(edge->begin->title));
      pt_map_set(chainlink,"end", pt_string_new(edge->end->title));
      cur_start = edge->end->title;
    } else {
      pt_map_set(chainlink,"start", pt_string_new(edge->end->title));
      pt_map_set(chainlink,"end", pt_string_new(edge->begin->title));
      cur_start = edge->begin->title;
    }
    pt_map_set(chainlink,"distance", pt_integer_new(edge->min_weight));
    pt_map_set(chainlink,"text", pt_string_new(edge->descriptors[0]->snippet));
    pt_array_push_back(edges,chainlink);
  }
  if (start)
    pt_map_set(payload,"start",pt_string_new(start));
  if (end)
    pt_map_set(payload,"end",pt_string_new(end));
  pt_map_set(payload,"edges",edges);
  pt_map_set(result,"payload",payload);
  char* json = pt_to_json(result,0);
  pt_free_node(result);
  return json;;
}

/*
 * Split input into a set
 */
static void split_into_vector(vector<string>& vec, const char* input)
{
  if (input) {
    int len = strlen(input);
    char* title = (char*) input;
    for(int i=0; i < len + 1; i++) {
      if (input[i] == '|' || input[i] == '\0') {
        string elem = string(title,input + i - title);
        vec.push_back(elem);
        title = (char*) input + i + 1;
      }
    }
  }
}
