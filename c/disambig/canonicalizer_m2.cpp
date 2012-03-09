#include <zmq.hpp>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pillowtalk.h>
#include "m2pp.hpp"
#include "thread_safe_queue.h"
#include "concept_resolver.h"

/* Typedefs */
class ReqRes {
  public:
    m2pp::request req;
    string res;
};

typedef vector<char*> SnippetList;

/* Globals */

#define NUM_DOWNLOAD_THREADS 4
static ThreadSafeQueue<m2pp::request> __req_queue(1000);
static ThreadSafeQueue<ReqRes> __res_queue(1000);
static ConceptResolver* __cr = NULL;

/* Static methods */

static void conceptualize(m2pp::request& res);
static void split_into_vector(SnippetList& vec, char* input);

/* Pull off Image's from the download queue to download them and put them on the resize queue*/
static void* download(void*) {
  signal(SIGPIPE,SIG_IGN);
  for(;;) {
    m2pp::request req = __req_queue.dequeue();

    printf("In Thread: %u\n", (unsigned int) pthread_self());
    /*
    int my = 0;
    for(int i=0; i < 100000000000; i++) {
      my += 1;
    }
    */

    conceptualize(req);

		std::ostringstream response;
		response << "<pre>" << std::endl;
		response << "SENDER: " << req.sender << std::endl;
		response << "IDENT: " << req.conn_id << std::endl;
		response << "PATH: " << req.path << std::endl;
		response << "BODY: " << req.body << std::endl;
    /*
		for (std::vector<m2pp::header>::iterator it=req.headers.begin();it!=req.headers.end();it++) {
			response << "HEADER: " << it->first << ": " << it->second << std::endl;
		}
    */
		response << "</pre>" << std::endl;

		//std::cout << response.str();
    /*
    ReqRes rr;
    rr.req = req;
    rr.res = response.str();
    __res_queue.enqueue(rr);
    */
  }
  return 0;
}

static void* respond(void* arg) {
  signal(SIGPIPE,SIG_IGN);
  m2pp::connection* conn = (m2pp::connection*) arg;
  for(;;) {
    ReqRes rr = __res_queue.dequeue();
		conn->reply_http(rr.req, rr.res);
  }
  return 0;
}

int main(int argc,char** argv) {
  char* aliases = NULL;
  char* neighbors = NULL;
  char* pub_address = (char*) "tcp://127.0.0.1:9999";
  char* sub_address = (char*) "tcp://127.0.0.1:9998";
  int c = 0;
  int error;

  signal(SIGPIPE,SIG_IGN);

  struct option long_options[] = {
    {"aliases", required_argument, 0, 'a'},
    {"neighbors", required_argument, 0, 'n'},
    {"pub-address", required_argument, 0, 'p'},
    {"sub-address", required_argument, 0, 's'},
    {0,0,0,0}
  };

  const char* help_string = "\
-a,  --aliases=TCH                  the location of the aliases.tch file\n\
-h,  --help                         show this message\n\
-n,  --neighbors=TCH                the location of the neighbors.tch file\n\
-p,  --pub-address=ADDRESS          the pub address, the default is tcp://127.0.0.1:9999\n\
-s,  --sub-address=ADDRESS          the pub address, the default is tcp://127.0.0.1:9998\n\
\
";
  int option_index = 0;

  while (1) {
    c = getopt_long(argc, argv, "a:n:p:s:",long_options,&option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        printf("%s\n",long_options[option_index].name);
        break;
      case 'a':
        printf("%s\n",optarg);
        aliases = optarg;
        break;
      case 'n':
        neighbors = optarg;
        break;
      case 'p':
        pub_address = optarg;
        break;
      case 's':
        sub_address = optarg;
        break;
      case ':':
        printf("%s",help_string);
        exit(-2);
        break;
      case 'h':
        printf("%s",help_string);
        exit(0);
        break;
      case '?':
        printf("Unknown Argument\n");
        printf("%s",help_string);
        exit(-2);
        break;
      default:
        abort();
    }
  }

  if (!aliases || !neighbors) {
    printf("%s",help_string);
    exit(-2);
  }

  __cr = new ConceptResolver(aliases, neighbors);


	std::string sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481";

  pthread_t download_threads[NUM_DOWNLOAD_THREADS];
  pthread_t response_thread;

  //m2pp::connection conn(sender_id, "tcp://127.0.0.1:9999", "tcp://127.0.0.1:9998");
	m2pp::connection conn(sender_id, pub_address,sub_address);

  for(int i=0; i< NUM_DOWNLOAD_THREADS; i++) {
    error = pthread_create(&download_threads[i], NULL, /* default attributes please */ download,NULL); 
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
  }

  error = pthread_create(&response_thread,NULL,respond,&conn);
  if (0 != error) 
    fprintf(stderr, "Couldn't run respond thread: errno %d\n", error);

	while (1) {
		m2pp::request req = conn.recv();
    printf("Pulled Request of Queue\n");
    __req_queue.enqueue(req);
	}

  for(int i=0; i< NUM_DOWNLOAD_THREADS; i++) {
    error = pthread_join(download_threads[i], NULL);
  }
  
  error = pthread_join(response_thread, NULL);
  if (0 != error) {
    fprintf(stderr, "Couldn't join thread number, errno %d\n", error);
  }

	return 0;
}

//------------------------------------------------
//-- S t a t i c    I m p l e m e n t a t i o n --
//------------------------------------------------
static void conceptualize(m2pp::request& req)
{
  printf("Conceptualize\n");
  //const char* cand = chb_rq_param_get(req,"candidates");
  char* cand = (char* )"Amazon|Apple|Steve Jobs|Google|United States|America|Yahoo|iPhone|iPad";
  char* cand_copy = NULL;

  if (!cand) 
    cand_copy = strdup("");
  else
    cand_copy = strdup(cand);

  SnippetList candidates;
  split_into_vector(candidates,cand_copy);

  ResolvedConceptList concepts;
  __cr->resolve(candidates,concepts);

  pt_node_t* result = pt_array_new();
  for(ResolvedConceptList::const_iterator ii = concepts.begin(); ii != concepts.end(); ii++) {
    resolved_concept_t conc = *ii;
    pt_node_t* entry = pt_map_new();
    pt_map_set(entry,"text_rep", pt_string_new(conc.text_rep));
    pt_map_set(entry,"concept", pt_string_new(conc.canonical));
    pt_map_set(entry,"score", pt_integer_new(conc.score));
    cout << "Concept:" << conc.text_rep << ":" << conc.canonical << ":" << conc.score <<  endl;
    pt_array_push_back(result,entry);
  }

  pt_node_t* payload = pt_map_new();
  pt_map_set(payload,"payload",result);

  char* json_str = pt_to_json(payload,0);
  pt_free_node(payload);

  ReqRes rr;
  rr.req = req;
  rr.res = json_str;

  free(json_str);

  __res_queue.enqueue(rr);
}

static void split_into_vector(SnippetList& vec, char* input)
{
  if (input) {
    int len = strlen(input);
    char* snippet = (char*) input;
    for(int i=0; i < len + 1; i++) {
      if (input[i] == '|' || input[i] == '\0') {
        input[i] = '\0';
        vec.push_back(snippet);
        snippet = (char*) input + i + 1;
      }
    }
  }
}
