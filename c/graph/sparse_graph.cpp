#include "sparse_graph.h"
#include "benchmark.h"
#include <algorithm>
#include <vector>
#include <queue>
#include <tcutil.h>
#include <tchdb.h>
#include <tcbdb.h>
#include <pillowtalk.h>

using namespace std;
using namespace boost;

static inline void add_to_edge_descriptors(edge_t* edge, edge_descriptor_t* ed);
static inline void add_to_edges(page_t* start, edge_t* edge);
static inline void add_to_edges(page_t* start, page_t* end,uint8_t weight);
static inline void free_page_t(page_t* page);

/* Allocate the buckets */
BucketQueue::BucketQueue(uint32_t max_page_id)
{
  max_bucket_ = MaxKey * MaxLinks;
  buckets_ = (shortest_path_t*) calloc(max_bucket_ + 1,sizeof(shortest_path_t));
  //pages_.set_empty_key(NULL);
  id_to_path_map_ = (shortest_path_t**) calloc(max_page_id + 1, sizeof(shortest_path_t*));
  size_ = 0;
  min_bucket_ = max_bucket_;
}

BucketQueue::~BucketQueue()
{
  free(buckets_);
  free(id_to_path_map_);
}

/* Put it in the right bucket */
void
BucketQueue::insert(page_t* key,uint16_t distance, uint8_t links)
{
  //shortest_path_t* path = new shortest_path_t();
  shortest_path_t* const path = pool_.malloc();
  path->end = key;
  path->distance = distance;
  path->links = links;
  id_to_path_map_[key->id] = path;

  //if (links > 5) 
  //  cout << "Inserted ('" << key->title << "'," << distance << "," << links << ")" << endl;
  place_path_in_correct_bucket(path);
  size_++;

  if(distance < min_bucket_)
    min_bucket_ = distance;
  //cout << "New Minimum Bucket: " << min_bucket_ << endl;
}

void
BucketQueue::decrease_key(page_t* key, uint16_t new_distance, uint8_t new_links)
{
  shortest_path_t* path = id_to_path_map_[key->id];
  if (path) {
    path->distance= new_distance;
    path->links = new_links;

    // We aren't in the queue anymore, just chilling in the table, so abort
    // here
    if(path->prev == NULL) {
      return;
    }

    remove_path_from_bucket(path);
    place_path_in_correct_bucket(path);
  }
}

/* Find the smallest bucket and remove the head of the list 
 * If the path removed was the last element in the list, update the min_bucket_ 
 * to the latest min_bucket
 */
shortest_path_t* 
BucketQueue::delete_min()
{
  shortest_path_t* path = buckets_[min_bucket_].next;
  if (path) {
    remove_path_from_bucket(path);
    if (path->next == NULL) {
      //cout << "Updating Min Bucket" << endl;
      size_t i =0 ;
      for(i=min_bucket_; i < max_bucket_; i++) {
        if (buckets_[i].next) {
          break;
        }
      }
      min_bucket_ = i;
      //cout << "Updated Min Bucket:" << min_bucket_ <<  endl;
    }
    size_--;
    //cout << "Delete Min ('" << path->end->title << "'," << path->distance << "," << path->links << ")" << endl;
    //cout << "New Minimum Bucket: " << min_bucket_ << endl;
    return path;
  } else {
    return NULL;
  }
}

shortest_path_t* 
BucketQueue::d(page_t* key)
{
  return id_to_path_map_[key->id];
}

/* Bucket Queue Protected Impl */

void 
BucketQueue::remove_path_from_bucket(shortest_path_t* path)
{
  path->prev->next = path->next;
  if (path->next)
    path->next->prev = path->prev;
  path->prev = NULL;
  path->next = NULL;
}

void
BucketQueue::place_path_in_correct_bucket(shortest_path_t* path)
{
  shortest_path_t* list = &buckets_[path->distance];
  path->prev = list;
  path->next = list->next;
  list->next = path;
  if (path->next)
    path->next->prev = path;

  if (path->distance < min_bucket_)
    min_bucket_ = path->distance;
}


/* Sparse Graph Public Impl */

SparseGraph::SparseGraph()
{
  max_id_ = 1;
  aliases_.set_empty_key(NULL);
}

page_t*
SparseGraph::page(const char* title) const
{
  TitlePageHash::const_iterator res = pages_.find((char*)title);
  if (res != pages_.end()) {
    return res->second;
  } else {
    return NULL;
  }
}

page_list_t* 
SparseGraph::concepts(const char* name) const
{
  TitlePagesHash::const_iterator res = aliases_.find((char*) name);
  if (res != aliases_.end())
    return res->second;
  else
    return NULL;
}

inline page_t* 
SparseGraph::find_or_build_page(const char* page_name)
{
  TitlePageHash::const_iterator res = pages_.find((char*) page_name);
  if (res != pages_.end()) {
    return res->second;
  } else {
    page_t* page = (page_t*) calloc(1,sizeof(page_t));
    page->id = max_id_;
    page->title = strdup(page_name);
    pages_[page->title] = page;
    max_id_++;
    return page;
  }
}

inline void
SparseGraph::append_or_build_alias_list(const char* alias, page_t* page)
{
  TitlePagesHash::const_iterator res = aliases_.find((char*)alias);
  if (res == aliases_.end()) {
    page_list_t* page_list = (page_list_t*) malloc(sizeof(page_list_t));
    page_list->length = 1;
    page_list->pages = (page_t**) malloc(sizeof(page_t*));
    page_list->pages[0] = page;
    aliases_[strdup(alias)] = page_list;
  } else {
    page_list_t* page_list = res->second;
    page_list->pages = (page_t**) realloc(page_list->pages,(page_list->length + 1) * sizeof(page_t*));
    page_list->pages[page_list->length++] = page;
  }
}

/*
 * This function uses pillowtalk to parse the open graph format
 */
void
SparseGraph::build_page_from_open_graph_json(char* title,char* json)
{
  pt_node_t* root = pt_from_json(json);
  if (root) {
    page_t* page = find_or_build_page(title);

    const char* common_name = pt_string_get(pt_map_get(root,"common_name"));
    if (common_name) {
      append_or_build_alias_list(common_name,page);
    }

    pt_node_t* web_sites_map = pt_map_get(root,"web_sites");
    if (web_sites_map) {
      pt_node_t* home_page = pt_map_get(web_sites_map,"home_page");
      if (home_page) {
        page->home_page = strdup(pt_string_get(home_page));
      }

      pt_node_t* twitter = pt_map_get(web_sites_map,"twitter");
      if (twitter) {
        page->twitter = strdup(pt_string_get(twitter));
      }
    }
    pt_iterator_t* alias_it = pt_iterator(pt_map_get(root,"aliases"));
    pt_node_t* alias_node = NULL;
    while((alias_node = pt_iterator_next(alias_it,NULL))) {
      const char* alias = pt_string_get(alias_node);
      append_or_build_alias_list(alias,page);
    }
    free(alias_it);

    pt_iterator_t* edge_it = pt_iterator(pt_map_get(root,"edges"));
    const char* end_point;
    pt_node_t* edge_node = NULL;
    while((edge_node = pt_iterator_next(edge_it,&end_point))) {
      edge_t* edge = build_edge_from_edge_json(page,(char*) end_point,edge_node);
      add_to_edges(page,edge);
    }
    free(edge_it);

    pt_free_node(root);
  }
}

void
SparseGraph::load(const char* filename)
{
  int ecode;
  bench_start("Load");
  TCHDB* hdb = tchdbnew();
  if(!tchdbtune(hdb,-1,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if(!tchdbopen(hdb, filename, HDBOREADER)){
    ecode = tchdbecode(hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  uint64_t num_records = tchdbrnum(hdb);
  pages_.resize(num_records);
  aliases_.resize(num_records);

  if (!tchdbiterinit(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "iterator error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  int key_len;
  int val_len;
  char* key;
  uint64_t count = 1;

  fprintf(stdout,"\rLoading: %9d / %d", (int) count, (int) num_records);
  fflush(stdout);

  while((key = (char*) tchdbiternext(hdb,&key_len))) {
    char* value = (char*) tchdbget(hdb,key,key_len,&val_len);
    if (value) {
      build_page_from_open_graph_json(key,value);
    }
    free(key);
    free(value);

    if (count % 1000 == 0) {
      fprintf(stdout,"\rLoading: %9d / %d", (int) count, (int) num_records);
      fflush(stdout);
    }
    count++;
  }

  fprintf(stdout,"\n");
  fflush(stdout);

  tchdbdel(hdb);
}

void
SparseGraph::clear()
{
  // TODO fix this for edge descriptors
  for(TitlePageHash::iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
    if (ii->first == ii->second->title) {
      free(ii->second->edges);
      free(ii->second->title);
      free(ii->second);
    } else {
      free(ii->first);
    }
  }
  pages_.clear();
  pages_.resize(0);
}

static inline bool already_scanned(page_t* node, const PageSet& scanned_set)
{
  PageSet::const_iterator scanned_already = scanned_set.find(node);
  return (scanned_already != scanned_set.end());
}

/* 
 * Return NULL if the edges vector is empty
 */
inline page_t* 
SparseGraph::build_shadow_page(const vector<string>& edge_names, const bool increasing_distances)
{
  if (edge_names.size() == 0) {
    return NULL;
  }

  TitlePagesHash::const_iterator aliases_end = aliases_.end();

  TitlePageHash::const_iterator pages_end = pages_.end();
  page_t* shadow = (page_t*) calloc(1,sizeof(page_t));
  shadow->title = (char*) strdup("Shadow Node XX");
  int dist = 0;
  for(vector<string>::const_iterator ii = edge_names.begin(); ii != edge_names.end(); ii++) {
    string tuple = *ii;
    TitlePagesHash::const_iterator res = aliases_.find((char*) tuple.c_str());
    if (res != aliases_end) {
      page_list_t* pages = res->second;
      for(uint32_t i =0; i < pages->length; i++) {
        page_t* page = pages->pages[i];
        add_to_edges(shadow,page,dist);
        if (increasing_distances)
          dist += 10;
      }
    }
  }
  return shadow;
}


void 
SparseGraph::edges(Chain& _return, const string& topic)
{
  bench_start("edges");
  TitlePageHash::const_iterator res = pages_.find((char*) topic.c_str());
  if (res != pages_.end()) {
    page_t* page = res->second;
    for(uint16_t i=0; i < page->num_edges; i++) {
      edge_t* edge = page->edges[i];
      _return.push_back(edge);
    }
  }
  bench_finish("edges");
}

void 
SparseGraph::shortest_chain(Chain& chain, const vector<string>& first_set, const vector<string>& second_set, const int max_links, const bool increasing_distances_1, const bool increasing_distances_2, const char** start_title, const char** end_title)
{
  bench_start("Shortest Page Chain");
  page_t* src_shadow = build_shadow_page(first_set, increasing_distances_1);
  page_t* dst_shadow = build_shadow_page(second_set, increasing_distances_2);

  // Bail out if either of the shadows are disconnected
  if (!src_shadow || !dst_shadow || src_shadow->num_edges == 0 || dst_shadow->num_edges == 0)
    return;

  TitlePageHash::const_iterator end = pages_.end();

  BucketQueue queue(max_id_);
  BucketQueue r_queue(max_id_);

  PageSet f_scanned;
  PageSet r_scanned;

  f_scanned.set_empty_key(NULL);
  r_scanned.set_empty_key(NULL);

  /*
  PagePageHash predecessors;
  PagePageHash successors;
  */

  PageEdgeHash predecessors;
  PageEdgeHash successors;

  predecessors.set_empty_key(NULL);
  successors.set_empty_key(NULL);

  page_t* middle = NULL;

  queue.insert(src_shadow,0,0);
  r_queue.insert(dst_shadow,0,0);

  predecessors[src_shadow] = NULL;
  successors[dst_shadow] = NULL;

  uint32_t sigma = INT_MAX;
  uint32_t top_f = 0;
  uint32_t top_r = 0;

  page_t* intersect = NULL;

  int real_max_links = max_links + 2;

  while(queue.size() > 0 || r_queue.size() > 0) {
    shortest_path_t* f_path = queue.delete_min();
    if (f_path) {
      page_t* f_page= f_path->end;

      //cout << "F Scanning " << f_page->title << " | " << f_path->distance << " | " << (int) f_path->links << endl;

      top_f = f_path->distance;

      uint16_t dist_current = f_path->distance;
      uint8_t links_current = f_path->links;

      f_scanned.insert(f_page);
      if (already_scanned(f_page,r_scanned)) {
        //cout << "F Already Scanned " << f_page->title << " in reverse direction (u=" << sigma << ")" << endl;
        shortest_path_t* r_path = r_queue.d(f_page);
        uint32_t full_path_len = f_path->distance + r_path->distance;
        if (full_path_len < sigma) {
          sigma = full_path_len;
          intersect = f_page;
          //cout << "F Updating Sigma Forward to " << sigma << " through " << f_page->title << endl;
          middle = intersect;
          goto assemble_path;
        }
      }

      for(uint16_t i=0; i < f_page->num_edges; i++) {
        edge_t* edge = f_page->edges[i];
        page_t* end = edge->end;
        size_t relaxed_path_dist = dist_current + edge->min_weight;
        size_t relaxed_path_links = links_current + 1;

        if (relaxed_path_dist > sigma)
          continue;

        // Todo move this out of the loop
        if ((int)relaxed_path_links > real_max_links)
          continue;

        shortest_path_t* existing_path = queue.d(end);
        if (existing_path) {
          if (relaxed_path_dist < existing_path->distance) {
            predecessors[end] = edge;
            queue.decrease_key(end,relaxed_path_dist,relaxed_path_links);
          }
        } else {
          predecessors[end] = edge;
          //cout << "F Inserting " << end->title << " into queue with " << relaxed_path_dist << endl;
          queue.insert(end,relaxed_path_dist,relaxed_path_links);
        }

        if (already_scanned(end,r_scanned)) {
          shortest_path_t* r_path = r_queue.d(end);
          uint32_t full_path_len = relaxed_path_dist + r_path->distance;
          if (full_path_len < sigma) {
            sigma = full_path_len;
            intersect = end;
            //cout << "F Updating Sigma Forward to " << sigma << " through " << end->title << endl;
          }
        }
      }
    }

    shortest_path_t* r_path = r_queue.delete_min();
    if (r_path) {
      page_t* r_page= r_path->end;
      //cout << "R Scanning " << r_page->title << " | " << r_path->distance << " | " << (int) r_path->links << endl;

      top_r = r_path->distance;
      if(top_r + top_f > sigma) {
        middle = intersect;
        goto assemble_path;
      }
      //if (r_page == src_shadow) {
        // Found a forward dijkstra to the destination
        //cout << "Arrived at Reverse Source" << endl;
      //}

      r_scanned.insert(r_page);

      if (already_scanned(r_page,f_scanned)) {
        //cout << "R Already Scanned " << r_page->title << " in forward direction (u=" << sigma << ")" << endl;
        shortest_path_t* f_path = queue.d(r_page);
        uint32_t full_path_len = f_path->distance + r_path->distance;
        if (full_path_len < sigma) {
          sigma = full_path_len;
          intersect = r_page;
          //cout << "R Updating Sigma Forward to " << sigma << " through " << r_page->title << endl;
          middle = intersect;
          goto assemble_path;
        }
      }

      uint16_t dist_current = r_path->distance;
      uint8_t links_current = r_path->links;

      for(uint16_t i=0; i < r_page->num_edges; i++) {
        edge_t* edge = r_page->edges[i];
        page_t* end = edge->end;
        size_t relaxed_path_dist = dist_current + edge->min_weight;
        size_t relaxed_path_links = links_current + 1;

        if (relaxed_path_dist > sigma)
          continue;

        if ((int)relaxed_path_links > real_max_links)
          continue;

        shortest_path_t* existing_path = r_queue.d(end);
        if (existing_path) {
          if (relaxed_path_dist < existing_path->distance) {
            successors[end] = edge;
            r_queue.decrease_key(end,relaxed_path_dist,relaxed_path_links);
          }
        } else {
          successors[end] = edge;
          //cout << "R Inserting " << end->title << " into queue with " << relaxed_path_dist << endl;
          r_queue.insert(end,relaxed_path_dist,relaxed_path_links);
        }

        if (already_scanned(end,f_scanned)) {
          shortest_path_t* r_path = r_queue.d(end);
          uint32_t full_path_len = relaxed_path_dist + r_path->distance;
          if (full_path_len < sigma) {
            sigma = full_path_len;
            intersect = end;
            //cout << "R Updating Sigma Reverse to " << sigma << " through " << end->title << endl;
          }
        }
      }
    }
  }

assemble_path:
  // Now trace back through previous.
  if (middle) {
    //cout << "Assembling Path from Middle:" << middle->title << endl;
    page_t* cur = middle;
    *start_title = cur->title;
    *end_title = cur->title;
    //path.push_front(middle);
    if (cur != dst_shadow) {
      for(;;) {
        //cout << "Searching for " << cur->title << " in successors" << endl;
        PageEdgeHash::const_iterator res = successors.find(cur);
        if (res != successors.end()) {
          edge_t* edge = res->second;
          cur = edge->begin;
          if (cur == dst_shadow)
            break;
          *end_title = cur->title;
          chain.push_back(edge);
        } else {
          break;
        }
      }
    }
    cur = middle;
    if (cur != src_shadow) {
      for(;;) {
        //cout << "Searching for " << cur->title << " in predecessors" << endl;
        PageEdgeHash::const_iterator res = predecessors.find(cur);
        if (res != predecessors.end()) {
          edge_t* edge = res->second;
          cur = edge->begin;
          if (cur == src_shadow)
            break;
          *start_title = cur->title;
          chain.push_front(edge);
        } else {
          //cout << "No Predecessor Chain Found" << endl;
          break;
        }
      }
    }

    //debug_chain_output(path,middle);
  }
  free_page_t(src_shadow);
  free_page_t(dst_shadow);
  bench_finish("Shortest Page Chain");
}


/* Singleton Initialization */
SparseGraph* SparseGraph::instance_ = NULL;
SparseGraph* SparseGraph::instance() {
  if (!instance_) {
    instance_ = new SparseGraph();
  }
  return instance_;
}

/* Static Implementation */

static inline void add_to_edges(page_t* start, edge_t* edge)
{
  if (start->edges) {
    if (start->num_edges < 65535) {
      start->edges= (edge_t**) realloc(start->edges,sizeof(edge_t*) * (start->num_edges+ 1));
      start->edges[start->num_edges] = edge;
      start->num_edges++;
    }
  } else {
    start->edges = (edge_t**) malloc(sizeof(page_t*));
    start->edges[0] = edge;
    start->num_edges = 1;
  }
}

static void add_to_edges(page_t* start,page_t* end,uint8_t weight)
{
  edge_t* new_edge = (edge_t*) malloc(sizeof(edge_t));
  new_edge->begin = start;
  new_edge->end = end;
  new_edge->min_weight = weight;
  if (start->edges) {
    if (start->num_edges < 65535) {
      start->edges= (edge_t**) realloc(start->edges,sizeof(edge_t*) * (start->num_edges+ 1));
      start->edges[start->num_edges] = new_edge;
      start->num_edges++;
    }
  } else {
    start->edges = (edge_t**) malloc(sizeof(page_t*));
    start->edges[0] = new_edge;
    start->num_edges = 1;
  }
}

static void free_page_t(page_t* page)
{
  if (page) {
    for(uint16_t i=0; i < page->num_edges; i++) {
      free(page->edges[i]);
    }
    free(page->title);
    free(page->edges);
    free(page);
  }
}


/*
 * This adds an edge descriptor to an edge's set.  Also if it is the first edge
 * descriptor it sets the minimum weight on the edge accordingly
 */
static inline void add_to_edge_descriptors(edge_t* edge, edge_descriptor_t* ed)
{
  if (edge->descriptors) {
    edge->descriptors= (edge_descriptor_t**) realloc(edge->descriptors,sizeof(edge_descriptor_t*) * (edge->num_descriptors+ 1));
    edge->descriptors[edge->num_descriptors] = ed;
    edge->num_descriptors++;
  } else {
    edge->descriptors = (edge_descriptor_t**) malloc(sizeof(edge_descriptor_t*));
    edge->descriptors[0] = ed;
    edge->num_descriptors = 1;
    edge->min_weight = ed->weight;
  }
}

edge_descriptor_t* 
SparseGraph::build_edge_descriptor_from_json(pt_node_t* ed_node)
{
  edge_descriptor_t* ed = (edge_descriptor_t*) calloc(1,sizeof(edge_descriptor_t));
  ed->weight = pt_integer_get(pt_map_get(ed_node,"distance"));
  ed->snippet = strdup(pt_string_get(pt_map_get(ed_node,"snippet")));
  return ed;
}
/*
 * Build an edge and all of its edge descriptors from the JSON
 */
edge_t* 
SparseGraph::build_edge_from_edge_json(page_t* begin,char* end_point_title,pt_node_t* edge_node)
{
  page_t* edge_page = find_or_build_page(end_point_title);
  edge_t* edge = (edge_t*) calloc(1,sizeof(edge_t));
  edge->begin = begin;
  edge->end = edge_page;

  pt_iterator_t* ed_it = pt_iterator(pt_map_get(edge_node,"descriptors"));
  pt_node_t* ed_node = NULL;
  while((ed_node = pt_iterator_next(ed_it,NULL))) {
    edge_descriptor_t* ed = build_edge_descriptor_from_json(ed_node);
    add_to_edge_descriptors(edge,ed);
  }
  free(ed_it);

  return edge;
}
