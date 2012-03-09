#include "relationship_holder.h"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <sys/types.h>
#include "thomas_wang_hash.h"
#include "sixty_four_bit_hash.h"
#include "paul_hsieh_hash.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "sphinxclient.h"
#include <mysql/mysql.h>
#include "benchmark.h"
#include <google/sparse_hash_map>
#include <google/dense_hash_map>
#include "equality.h"
#include "death.h"
#include <pthread.h>
#include "cruxlux_config.h"

#define DIST_RANGE 256
#define HALF_DIST_RANGE_SHIFT 7 // Log(256/2, 2)
#define STORE_THRESHOLD 249
#define CLOSE_THRESHOLD 160
#define SPHINX_LIMIT 100000
#define PRESIZE 10000000

using namespace std;
using google::dense_hash_map;
using google::sparse_hash_map;
using namespace boost::posix_time;

static bool finished = false;
static string total_readable = "";
static unsigned int num_sites = 0;
static u_int32_t x_;
static u_int32_t y_;

struct id_array {
  int* ids;
  size_t length;
};

struct url_array {
  char** urls;
  size_t length;
};

struct int_array {
  int* array;
  size_t length;
};

typedef sparse_hash_map<u_int64_t,unsigned char,SixtyFourBitHash, eqlong> RelationshipHash;
typedef sparse_hash_map<unsigned int,int_array,ThomasWangHash, eqint> CloseHash;
typedef dense_hash_map<const char*, int, PaulHsiehHash, eqstr> UrlIdHash;
typedef dense_hash_map<u_int64_t,short, SixtyFourBitHash, eqlong> InlinksExistenceHash;

typedef vector<pair<const char*,int> > SphinxChunk;

class SixtyFourRelationshipHolderImpl : public RelationshipHolder {
  public:
    void distances(vector<SiteRelationship>& rels,const vector<int>& ids);
    void sites_close_to(CloseSiteVector& sites, const int site_id);
    unsigned char distance_between(const int site_id,const int other_id);
    size_t size();
    void store(const int site_id,const int other_id,const unsigned char distance);
    void destroy(const int site_id,const int other_id);
    void persist(const string& filename);
    void load(const string& filename);
    void rotate(const string& rotate_filename);
    void cluster_config_file(const string& filename);
    void build();
    void clear();
    void full_sorted_output();

    void calculate_distance_between(const string& site,const string& other);
    void calculate_distance_between(const int site_id,const int other_id);

    string statistics();
    SixtyFourRelationshipHolderImpl() {
      relationships_ = new RelationshipHash(PRESIZE);
      close_sites_ = new CloseHash(PRESIZE);
      relationships_->set_deleted_key(composite_key(0,0));
      close_sites_->set_deleted_key(0);
      ids_to_urls_.urls = NULL;
      ids_to_urls_.length = 0;
    }

    virtual ~SixtyFourRelationshipHolderImpl() {
      clear();
      delete relationships_;
      delete close_sites_;
    }
  protected:
    inline void update_close_sites(CloseHash* ch,const int site_id,const int other_id);
    inline void remove_close_sites(CloseHash* ch,const int site_id,const int other_id);

    void generate_mappings();
    void generate_inlink_hash();
    void sphinx_query_chunks(vector<SphinxChunk>&);

    /* This is the speedy inline version that does the work with the globals */
    inline void calculate_one_distance();

    string sphinx_host_;
    int sphinx_port_;
    string mysql_host_;
    int mysql_port_;
    string mysql_db_;
    string mysql_user_;
    string mysql_password_;

    RelationshipHash* relationships_;
    CloseHash* close_sites_;
    struct url_array ids_to_urls_;
    UrlIdHash urls_to_ids_;

    /* This array contains all the site ids linking to a particular site (the index) */
    struct id_array* inlinks_;
    InlinksExistenceHash inlinks_existence_hash_;
};


/* Implementation */

shared_ptr<RelationshipHolder> RelationshipHolder::instance_ = shared_ptr<RelationshipHolder>();

shared_ptr<RelationshipHolder> 
RelationshipHolder::instance() {
  if (!instance_) {
    instance_ = shared_ptr<RelationshipHolder>(new SixtyFourRelationshipHolderImpl());
  }
  return instance_;
}

static void* progress_thread(void*)
{
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  ptime start = microsec_clock::local_time();
  ptime finish;
  u_int64_t iter = 0;
  while(!finished) {
    iter = 0;
    int x_snap = x_;
    int y_snap = y_;
    for(int i = 0; i < x_snap; i++) {
      iter += (num_sites - i);
    }
    iter += y_snap;
    finish= microsec_clock::local_time();
    cout.setf(ios::fixed,ios::floatfield);
    cout.precision(2);
    cout << "Computed " << (double) iter / 1000000 << "M iterations out of " << total_readable << " Hash Size: " << rh->size() << " Elapsed Time (" << finish -start << ")" <<  endl; 
    sleep(60);
  }
  return NULL;
}

size_t 
SixtyFourRelationshipHolderImpl::size()
{
  return relationships_->size();
}

void
SixtyFourRelationshipHolderImpl::distances(vector<SiteRelationship>& rels,const vector<int>& ids)
{
  RelationshipHash::const_iterator result;
  unsigned char distance;
  unsigned int i,first_id,second_id;
  vector<int> sorted_ids;

  unique_copy(ids.begin(),ids.end(),std::inserter(sorted_ids,sorted_ids.begin()));
  sort(sorted_ids.begin(),sorted_ids.end());

  for(i = 0; i < sorted_ids.size(); i++) {
    first_id = sorted_ids[i];
    for(unsigned int j = i + 1; j < sorted_ids.size(); j++) {
      second_id = sorted_ids[j];
      result = relationships_->find(composite_key(first_id,second_id));
      if (result != relationships_->end()) {
        SiteRelationship rel;
        distance = (*result).second;
        rel.first = first_id;
        rel.second = second_id;
        rel.distance = distance;
        rels.push_back(rel);
      }
    }
  }
}

unsigned char
SixtyFourRelationshipHolderImpl::distance_between(const int from_id,const int to_id)
{
  RelationshipHash::const_iterator result;
  u_int64_t key;
  if (from_id < to_id)
    key = composite_key(from_id,to_id);
  else
    key = composite_key(to_id,from_id);
  result = relationships_->find(key);
  if (result == relationships_->end()) {
    return 255;
  } else {
    return (*result).second;
  }
}


void
SixtyFourRelationshipHolderImpl::sites_close_to(CloseSiteVector& results, const int site_id)
{
  set<int> unique_sites;

  CloseHash::const_iterator result;
  result = close_sites_->find(site_id);
  if (result != close_sites_->end()) {
    struct int_array ids;
    ids = (*result).second;
    for(unsigned int i=0; i < ids.length; i++) {
      unique_sites.insert(ids.array[i]);
    }

    for(set<int>::const_iterator ii = unique_sites.begin(); ii != unique_sites.end(); ii++) {
      results.push_back(*ii);
    }
  }
}

inline void
SixtyFourRelationshipHolderImpl::update_close_sites(CloseHash* ch, const int site_id,const int other_id)
{
  CloseHash::iterator result;
  result = ch->find(site_id);
  if (result != ch->end()) {
    struct int_array* ids_ptr = NULL;
    int* dup, *end;
    ids_ptr = &((*result).second);
    // first search for a duplicate
    end = ids_ptr->array + ids_ptr->length;
    dup = find(ids_ptr->array,end, other_id);
    if (dup == end) {
      ids_ptr->array = (int*) realloc(ids_ptr->array,(ids_ptr->length + 1) * sizeof(int));
      ids_ptr->array[ids_ptr->length] = other_id;
      ids_ptr->length += 1;
    }
  } else {
    struct int_array ids;
    ids.length = 1;
    ids.array = (int*) malloc(sizeof(int));
    ids.array[0] = other_id;
    (*ch)[site_id] = ids;
  }
}

inline void
SixtyFourRelationshipHolderImpl::remove_close_sites(CloseHash* ch,const int site_id,const int other_id)
{
  CloseHash::iterator result;
  result = ch->find(site_id);
  if (result != ch->end()) {
    struct int_array* ids_ptr = NULL;
    int* val, *end, *existing;
    ids_ptr = &((*result).second);
    // first search for the value
    if (ids_ptr->length == 0)
      return;
    end = ids_ptr->array + ids_ptr->length;
    val = find(ids_ptr->array,end,other_id);
    if (val != end) {
      if (ids_ptr->length > 1) {
        existing = ids_ptr->array;
        ids_ptr->array = (int*) malloc(sizeof(int) * ids_ptr->length - 1);
        int first_block_size = val - existing;
        ids_ptr->length--;
        memcpy(ids_ptr->array,existing,first_block_size);
        memcpy((char*)(ids_ptr->array) + first_block_size,((char*)val) + sizeof(int),(ids_ptr->length * sizeof(int)) - first_block_size);
        free(existing);
      } else {
        ids_ptr->length = 0;
        free(ids_ptr->array);
        ids_ptr->array = NULL;
      }
    }
  }
}

void 
SixtyFourRelationshipHolderImpl::store(const int site_id,const int other_id,const unsigned char distance)
{
  if (site_id < other_id)
    (*relationships_)[composite_key(site_id,other_id)] = distance;
  else
    (*relationships_)[composite_key(other_id,site_id)] = distance;

  update_close_sites(close_sites_,site_id,other_id);
  update_close_sites(close_sites_,other_id,site_id);
}

void 
SixtyFourRelationshipHolderImpl::destroy(const int site_id,const int other_id)
{
  RelationshipHash::iterator result;
  u_int64_t key;
  if (site_id < other_id)
    key = composite_key(site_id,other_id);
  else
    key = composite_key(other_id,site_id);

  result = relationships_->find(key);
  if (result != relationships_->end()) {
    relationships_->erase(result);
    remove_close_sites(close_sites_,site_id,other_id);
    remove_close_sites(close_sites_,other_id,site_id);
  }
}

/* 
 * Just persist the non pointer data in relationships_.  We can build up the
 * the close sites one by iterating through the sparse hash.
 */
void 
SixtyFourRelationshipHolderImpl::persist(const string& filename)
{
  FILE* fp = fopen(filename.c_str(),"w");
  if (fp != NULL) {
    relationships_->write_metadata(fp);
    relationships_->write_nopointer_data(fp);
    fclose(fp);
  }
}

void 
SixtyFourRelationshipHolderImpl::load(const string& filename)
{
  FILE* fp = fopen(filename.c_str(),"r");
  CloseHash::iterator result;
  bench_start("Load");
  if (fp != NULL) {
    bench_start("Load Relationships");
    relationships_->read_metadata(fp);
    relationships_->read_nopointer_data(fp);
    fclose(fp);
    bench_finish("Load Relationships");

    bench_start("Load Close Sites");
    for (RelationshipHash::iterator it = relationships_->begin();it != relationships_->end(); ++it) {
      // The key is stored in the sparse_hash_map as a pointer
      uint64_t composite = it->first;
      unsigned char distance = it->second;
      struct decomposed_key dk = decompose_key(composite);
      if (distance <= CLOSE_THRESHOLD) {
        update_close_sites(close_sites_,dk.site_id,dk.other_id);
        update_close_sites(close_sites_,dk.other_id,dk.site_id);
      }
    }
    bench_finish("Load Close Sites");
  }
  bench_finish("Load");
}

void 
SixtyFourRelationshipHolderImpl::rotate(const string& rotation_filename)
{
  FILE* fp = fopen(rotation_filename.c_str(),"r");
  RelationshipHash* rel_copy = new RelationshipHash();
  CloseHash* ch_copy = new CloseHash();
  CloseHash::iterator close_sites_it;
  for(close_sites_it = close_sites_->begin(); close_sites_it != close_sites_->end(); close_sites_it++) {
    int_array ids = close_sites_it->second;
    if (ids.array) {
      free(ids.array);
    }
  }
  delete relationships_;
  delete close_sites_;
  if (fp != NULL) {
    rel_copy->read_metadata(fp);
    rel_copy->read_nopointer_data(fp);
    fclose(fp);
    for (RelationshipHash::iterator it = rel_copy->begin();it != rel_copy->end(); ++it) {
      // The key is stored in the sparse_hash_map as a pointer
      uint64_t composite = it->first;
      unsigned char distance = it->second;
      struct decomposed_key dk = decompose_key(composite);
      if (distance <= CLOSE_THRESHOLD) {
        update_close_sites(ch_copy,dk.site_id,dk.other_id);
        update_close_sites(ch_copy,dk.other_id,dk.site_id);
      }
    }
  }
  // Now change the pointers
  relationships_ = rel_copy;
  close_sites_ = ch_copy;
}

void 
SixtyFourRelationshipHolderImpl::clear()
{
  relationships_->clear();
  CloseHash::iterator close_sites_it;
  for(close_sites_it = close_sites_->begin(); close_sites_it != close_sites_->end(); close_sites_it++) {
    int_array ids = close_sites_it->second;
    if (ids.array && ids.length > 0) {
      free(ids.array);
    }
    ids.array = NULL;
  }
  close_sites_->clear();
}

void
SixtyFourRelationshipHolderImpl::cluster_config_file(const string& filename)
{
  shared_ptr<CruxluxConfig> cc = CruxluxConfig::instance();
  cc->filename(filename);
  sphinx_host_ = cc->value("sphinx","host");
  sphinx_port_ = atoi(cc->value("sphinx","port").c_str());
  mysql_host_ = cc->value("database","host");
  mysql_port_ = atoi(cc->value("database","port").c_str());
  mysql_db_ = cc->value("database","db");
  mysql_user_ = cc->value("database","user");
  mysql_password_ = cc->value("database","password");
}

void 
SixtyFourRelationshipHolderImpl::generate_mappings(void) 
{
  MYSQL mysql;
  MYSQL* rc;
  MYSQL_RES *res;
  MYSQL_ROW row;

  char query[200];
  bench_start("Buildup Mappings");
  urls_to_ids_.set_empty_key(NULL);
  mysql_init(&mysql);
  rc = mysql_real_connect(&mysql,mysql_host_.c_str(),mysql_user_.c_str(),mysql_password_.c_str(),mysql_db_.c_str(),mysql_port_,NULL,0);
  if (rc == NULL) {
    death("Failure to Connect to Mysql");
  }

  sprintf(query,"SELECT MAX(id) from external_sites");
  mysql_real_query(&mysql,query,(unsigned int)strlen(query));

  res = mysql_use_result(&mysql);
  if ((row = mysql_fetch_row(res))) {
    int size = atoi(row[0]) + 1;
    ids_to_urls_.length = size;
    ids_to_urls_.urls = (char**) calloc(size,sizeof(char*));
    inlinks_ = (struct id_array*) calloc(size,sizeof(struct id_array));
  } else {
    death("No Max ID Found");
  }
  mysql_free_result(res);
  sprintf(query,"SELECT id,name from external_sites");
  mysql_real_query(&mysql,query,(unsigned int)strlen(query));
  res = mysql_use_result(&mysql);
  while((row = mysql_fetch_row(res))) {
    int id = atoi(row[0]);
    char* name = strdup(row[1]);
    ids_to_urls_.urls[id] = name;
    urls_to_ids_[name] = id;
  }
  mysql_free_result(res);
  mysql_close(&mysql);
  cout << "Map Size:" << urls_to_ids_.size() << endl;
  bench_finish("Buildup Mappings");
}

void
SixtyFourRelationshipHolderImpl::sphinx_query_chunks(vector<SphinxChunk>& chunks)
{
  sphinx_client * client;
	sphinx_result * res;
  sphinx_bool ec;
  int rc;
  int chunk_counter = 0;
  vector<SphinxChunk>::const_iterator chunk_it;
  SphinxChunk::const_iterator pair_it;

  for(chunk_it = chunks.begin(); chunk_it != chunks.end(); chunk_it++) {
    chunk_counter++;
    client = sphinx_create ( SPH_FALSE );
    if ( !client )
      death( "failed to create client" );

    ec = sphinx_set_server(client,sphinx_host_.c_str(),sphinx_port_);
    if (ec != SPH_TRUE) {
      death( "failed to set sphinx server" );
    }
    ec = sphinx_set_limits(client,0,SPHINX_LIMIT,SPHINX_LIMIT,0);
    if (ec != SPH_TRUE) {
      death( "failed to set sphinx limits" );
    }
    SphinxChunk chunk = *chunk_it;
    for(pair_it = chunk.begin(); pair_it != chunk.end(); pair_it++) {
      rc = sphinx_add_query(client,pair_it->first,"external_sites",NULL);
      if (rc == -1) {
        cout << sphinx_error(client) << endl;
        death("Failed to Add Query");
      }
    }

    res = sphinx_run_queries(client);
    if (res) {
      for(int i=0; i < sphinx_get_num_results(client); i++) {
        if (res[i].status == SEARCHD_OK) {
          int site_id = chunk[i].second;
          if ((unsigned int) site_id < ids_to_urls_.length) {
            if (res[i].num_matches > 0) {
              inlinks_[site_id].ids = (int*) malloc(sizeof(int) * (res[i].num_matches + 1));
              inlinks_[site_id].length = res[i].num_matches + 1;
              for ( int j=0; j<res[i].num_matches; j++ )
              {
                int id = (int)sphinx_get_id (&res[i], j );
                inlinks_[site_id].ids[j] = id;
                inlinks_existence_hash_[composite_key(id,site_id)] = 1;
              }
            } else {
              inlinks_[site_id].ids = (int*) malloc(sizeof(int));
              inlinks_[site_id].length = 1;
            }
            inlinks_[site_id].ids[inlinks_[site_id].length - 1] = site_id;
          }
          //inlinks_existence_hash_[composite_key(site_id,site_id)] = 1;
        } else {
          death("Problem with searchd: %s,%s", res->error, res->warning);
        }
      }    
    } else {
      cout << "Querying on Chunk" << endl;
      cout << "Client Error: " << sphinx_error(client);
      cout << "Client Warning: " << sphinx_warning(client);
      death("Unknown Problem with searchd");
    }
    sphinx_destroy ( client );
  }
}

void 
SixtyFourRelationshipHolderImpl::generate_inlink_hash(void)
{
  UrlIdHash::const_iterator ii;

  inlinks_existence_hash_.set_empty_key(composite_key(0,0));

  bench_start("Sphinx Queries");

  vector<SphinxChunk> chunks;
  SphinxChunk chunk;

  for(ii = urls_to_ids_.begin(); ii != urls_to_ids_.end(); ++ii) {
    if (chunk.size() == 32) {
      chunks.push_back(chunk);
      chunk.clear();
    }
    chunk.push_back(pair<const char*,int>(ii->first,ii->second));
  }
  chunks.push_back(chunk);

  sphinx_query_chunks(chunks);
  bench_finish("Sphinx Queries");
}

inline void 
SixtyFourRelationshipHolderImpl::calculate_one_distance()
{
  int links_both;
  InlinksExistenceHash::const_iterator result;

  int links_to_x = inlinks_[x_].length;
  int links_to_y = inlinks_[y_].length;

  if (links_to_x == 0 || links_to_y == 0) {
    return;
  }

  if (links_to_x == 1) {
    result = inlinks_existence_hash_.find(composite_key(x_,y_));
    if (result != inlinks_existence_hash_.end()) {
      unsigned char dist = DIST_RANGE/2 - DIST_RANGE/2/links_to_y; //DIST_RANGE * [1 - (1 + 1/links_to_y) / 2]
      if (dist == 0) {
        dist = 1;
      }
      if (dist <= STORE_THRESHOLD)
        (*relationships_)[composite_key(x_,y_)] = dist;
    } 
  } else if (links_to_y == 1) {
    result = inlinks_existence_hash_.find(composite_key(y_,x_));
    if (result != inlinks_existence_hash_.end()) {
      unsigned char dist = DIST_RANGE/2 - DIST_RANGE/2/links_to_x; //DIST_RANGE * [1 - (1/links_to_x + 1) / 2]
      if (dist == 0) {
        dist = 1;
      }
      if (dist <= STORE_THRESHOLD)
        (*relationships_)[composite_key(x_,y_)] = dist;
    } 
  } else {

    links_both = 0;
    for(int k = 0; k < links_to_x; k++) {
      int id_to_test;
      id_to_test = inlinks_[x_].ids[k];
      if (id_to_test == (int) y_) {
        links_both++;
      } else {
        result = inlinks_existence_hash_.find(composite_key(id_to_test,y_));
        if (result != inlinks_existence_hash_.end()) {
          links_both++;
        }
      }
    }

    if (links_both > 0) {
      int dist = DIST_RANGE - (links_both<<HALF_DIST_RANGE_SHIFT)/links_to_x - (links_both<<HALF_DIST_RANGE_SHIFT)/links_to_y; //DIST_RANGE * [1 - 0.5 * (links_both/links_to_x+links_both/links_to_y)]
      if (dist == 0)
        (*relationships_)[composite_key(x_,y_)] = (unsigned char) 1;
      else if (dist <= STORE_THRESHOLD)
        (*relationships_)[composite_key(x_,y_)] = (unsigned char) dist;
    }
  }
}

void 
SixtyFourRelationshipHolderImpl::build()
{
  generate_mappings();
  generate_inlink_hash();

  bench_start("Calculations");

  InlinksExistenceHash::const_iterator result;

  double total = 0;
  
  for(int i=ids_to_urls_.length; i >= 0; i--) {
    total += i;
  }

  num_sites = ids_to_urls_.length;
  string unit_type = "";
  stringstream readable;
  total /= 1000000;
  if (total > 1000) {
    total /= 1000;
    readable << total << "B"; 
  } else {
    readable << total << "M";
  }

  total_readable = readable.str();

  finished = false;
  pthread_t tid;
  pthread_create(&tid,NULL,progress_thread,NULL);


  for(x_=0; x_ < ids_to_urls_.length; x_++) {
    for(y_= x_ + 1; y_ < ids_to_urls_.length; y_++) {
      calculate_one_distance();
    }
  }

  finished = true;
  bench_finish("Calculations");
  cout << statistics() << endl;
}

static bool compare_keys(pair<uint64_t,unsigned char> a,pair<uint64_t,unsigned char> b)
{
  struct decomposed_key a_key = decompose_key(a.first);
  struct decomposed_key b_key = decompose_key(b.first);
  if (a_key.site_id == b_key.site_id)
    return a_key.other_id < b_key.other_id;
  else
    return a_key.site_id < b_key.site_id;
} 
static bool compare_distances(pair<uint64_t,unsigned char> a,pair<uint64_t,unsigned char> b)
{
  return a.second < b.second;
} 

static bool compare_id_distances(pair<int,unsigned char> a,pair<int,unsigned char>b)
{
  return a.second < b.second;
}
void 
SixtyFourRelationshipHolderImpl::full_sorted_output()
{
  list<pair<uint64_t,unsigned char> > distances;// = new list<pair<uint64_t,unsigned char> >();
  list<pair<uint64_t,unsigned char> >::const_iterator di;

  if (ids_to_urls_.length == 0)
    generate_mappings();

  for (RelationshipHash::iterator it = relationships_->begin();it != relationships_->end(); ++it) {
    distances.push_back(*it);
  }

  cout << "== Linked List Size: " << distances.size() << endl;
  distances.sort(compare_keys);
  for(di = distances.begin(); di != distances.end(); di++) {
    uint64_t composite = di->first;
    struct decomposed_key dk = decompose_key(composite);
    unsigned char distance = di->second;
    cout << dk.site_id << ":" << ids_to_urls_.urls[dk.site_id] << " || " << dk.other_id << ":" << ids_to_urls_.urls[dk.other_id] << " = " << (int) distance << endl;
  }

  cout << "== Done Outputting Distance Pairs == " << endl;

  distances.clear();

  list<pair<int,unsigned char> > id_distances;
  list<pair<int,unsigned char> >::const_iterator iid;
  RelationshipHash::const_iterator result;
  cout << "== Start Close Sites ==" << endl;
  for (CloseHash::iterator it = close_sites_->begin(); it != close_sites_->end(); ++it) {
    struct int_array* ids = &(it->second);
    cout << "==" << ids_to_urls_.urls[it->first] << " :: " << ids->length << " related sites" << endl;
    for (unsigned int i=0; i < ids->length; i++) {
      if (it->first < (unsigned int) ids->array[i])
        result = relationships_->find(composite_key(it->first,ids->array[i]));
      else
        result = relationships_->find(composite_key(ids->array[i],it->first));
      if (result != relationships_->end()) {
        pair<int,unsigned char> entry = pair<int,unsigned char>(ids->array[i],result->second);
        id_distances.push_back(entry);
      } else {
        printf("ERROR:: Searching for key:(%d:%d) for %s:%s\n", it->first,ids->array[i],ids_to_urls_.urls[it->first],ids_to_urls_.urls[ids->array[i]]);
      }
    }
    id_distances.sort(compare_id_distances);
    for(iid = id_distances.begin(); iid != id_distances.end(); iid++) {
      cout << "\t" << (int) iid->second << " :: " << ids_to_urls_.urls[iid->first] << endl;
    }
    id_distances.clear();
  }
}

void 
SixtyFourRelationshipHolderImpl::calculate_distance_between(const string& site,const string& other)
{
  generate_mappings();
  generate_inlink_hash();

  int x = 0,y = 0;

  UrlIdHash::const_iterator result;

  result = urls_to_ids_.find(site.c_str());
  if (result != urls_to_ids_.end()) {
    x = result->second;
  } else {
    death("Unknown Site:%s", site.c_str());
  }

  result = urls_to_ids_.find(other.c_str());
  if (result != urls_to_ids_.end()) {
    y = result->second;
  } else {
    death("Unknown Site:%s", site.c_str());
  }

  calculate_distance_between(x,y);
}

void 
SixtyFourRelationshipHolderImpl::calculate_distance_between(const int site_id,const int other_id)
{
  x_ = site_id;
  y_ = other_id;
  calculate_one_distance();
}

string
SixtyFourRelationshipHolderImpl::statistics()
{
  stringstream stats;
  stats << "Number of Hash Entries: " << relationships_->size() << endl;
  stats << "Number of Buckets: " << relationships_->bucket_count() << endl;
  stats << "Max Number of Buckets: " << relationships_->max_bucket_count() << endl;
  return stats.str();
}
