#include "canonicalizer_client.h"
#include <pillowtalk.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

/* Prototypes */
static void *myrealloc(void *ptr, size_t size);
static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data);
static string char2hex( char dec );
string urlencode(const string &c);

struct raw_curl_data_t {
  char* data;
  int len;
  char* effective_url;
  long response_code;

  ~raw_curl_data_t() {
    free(data);
    free(effective_url);
  }
};

/*----------------------------------------------
---- P u b l i c  I m p l e m e n t a t i o n --
----------------------------------------------*/

CanonicalizerClient::CanonicalizerClient() {}

void 
CanonicalizerClient::base_url(const string& url)
{
  base_url_ = url;
}

void
CanonicalizerClient::canonicals(const vector<string>& candidates, vector<CanonicalMeta>& canonicals) throw(CanonicalizerClientException)
{
  CURL *curl;
  CURLcode ret;
  raw_curl_data_t* result = (raw_curl_data_t*) calloc(1,sizeof(raw_curl_data_t));

  stringstream url;
  url << base_url_;
  url << "/conceptualize?candidates=";
  if (candidates.size() > 0) {
    for(vector<string>::const_iterator ii = candidates.begin(), end = candidates.end() - 1; ii != end; ii++) {
      url << urlencode(*ii) << "|";
    }
    url << urlencode(*(candidates.end() - 1));
  }

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
  ret = curl_easy_perform(curl); /* ignores error */
  if ((!ret || ret == CURLE_OPERATION_TIMEDOUT) && result->len > 0) {
    pt_node_t* node = pt_from_json(result->data);
    if (node) {
      pt_iterator_t* iter = pt_iterator(pt_map_get(node,"payload"));
      if (iter) {
        pt_node_t* canon = NULL;
        while((canon = pt_iterator_next(iter,NULL)) != NULL) {
          CanonicalMeta meta;
          meta.canonical = pt_string_get(pt_map_get(canon,"concept"));
          meta.text_rep = pt_string_get(pt_map_get(canon,"text_rep"));
          meta.score = pt_integer_get(pt_map_get(canon,"score"));
          canonicals.push_back(meta);
        }
      }
      free(iter);
    }
    pt_free_node(node);
    delete result;
    curl_easy_cleanup(curl);
  } else {
    delete result;
    curl_easy_cleanup(curl);
    throw CanonicalizerClientException("Could not connect to Canonicalizer Service");
  }
}

CanonicalizerClient* CanonicalizerClient::instance_ = NULL;
CanonicalizerClient* CanonicalizerClient::instance() {
  if (!instance_) {
    instance_= new CanonicalizerClient();
    curl_global_init(CURL_GLOBAL_ALL);
  }
  return instance_;
}

//------------------------------------------------
//-- S t a t i c    I m p l e m e n t a t i o n --
//------------------------------------------------
static void *myrealloc(void *ptr, size_t size)
{
  /* There might be a realloc() out there that doesn't like reallocing
     NULL pointers, so we take care of it here */
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

static size_t
write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct raw_curl_data_t *mem = (struct raw_curl_data_t*)data;

  mem->data = (char *)myrealloc(mem->data, mem->len + realsize + 1);
  if (mem->data) {
    memcpy(&(mem->data[mem->len]), ptr, realsize);
    mem->len += realsize;
    mem->data[mem->len] = 0;
  }
  return realsize;
}

static string char2hex( char dec )
{
  char dig1 = (dec&0xF0)>>4;
  char dig2 = (dec&0x0F);
  if ( 0<= dig1 && dig1<= 9) dig1+=48;    //0,48inascii
  if (10<= dig1 && dig1<=15) dig1+=97-10; //a,97inascii
  if ( 0<= dig2 && dig2<= 9) dig2+=48;
  if (10<= dig2 && dig2<=15) dig2+=97-10;

  string r;
  r.append( &dig1, 1);
  r.append( &dig2, 1);
  return r;
}

//based on javascript encodeURIComponent()
string urlencode(const string &c)
{
  string escaped="";
  int max = c.length();
  for(int i=0; i<max; i++)
  {
    if ( (48 <= c[i] && c[i] <= 57) ||//0-9
      (65 <= c[i] && c[i] <= 90) ||//abc...xyz
      (97 <= c[i] && c[i] <= 122) || //ABC...XYZ
      (c[i]=='~' || c[i]=='!' || c[i]=='*' || c[i]=='(' || c[i]==')' || c[i]=='\'')
    )
    {
      escaped.append( &c[i], 1);
    }
    else
    {
      escaped.append("%");
      escaped.append( char2hex(c[i]) );//converts char 255 to string "ff"
    }
  }
  return escaped;
}
