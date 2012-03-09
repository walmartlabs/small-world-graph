#include "wikipedia_neighbor_set_impl.h"
#include "benchmark.h"
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>
#include <valgrind/callgrind.h>
#include <assert.h>

using namespace std;

//static bool compare_distances(relationship_t& first, relationship_t& second);
static inline void add_to_neighbors(page_neighbors_t* page,const char* neighbor_key,unsigned char distance);

void
WikipediaNeighborSetImpl::load(const char* filename)
{
  bench_start("Load Pages");
  FILE* file = NULL;
  char buffer[4096];

  neighbors_.resize(10000000);
  page_neighbors_t** id_neighbor_table = NULL;
  size_t biggest_id_so_far = 0;
  string index_filename = string(filename) + ".pages";
  int page_count = 0;
  if ((file = fopen((char*)index_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,file)) {
      char* first = NULL;
      char* second = NULL;
      char* third = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '|') {
          buffer[i] = '\0';
          first = buffer;
          second = buffer + i + 1;
          for(int j=i; j < length; j++) {
            if (buffer[j] == '|') {
              buffer[j] = '\0';
              third = buffer + j + 1;
              buffer[length-1] = '\0';
              break;
            }
          }

          if (!first || !second || !third)
            break;

          //printf("%s|%s|%s\n",first,second,third);
          int id = atoi(first);
          page_neighbors_t* neighbor_holder = (page_neighbors_t*) calloc(1,sizeof(page_neighbors_t));
          char* key = strdup(second);
          neighbor_holder->title = key;
          neighbors_[key] = neighbor_holder;
          neighbor_holder->ambiguous = (bool) atoi(third);
          page_count++;
          if (id > (int) biggest_id_so_far) {
            id_neighbor_table = (page_neighbors_t**) realloc(id_neighbor_table,sizeof(page_neighbors_t*) * (id + 1));
            for(int k = biggest_id_so_far + 1; k < id + 1; k++) {
              id_neighbor_table[k] = NULL;
            }
            biggest_id_so_far = id;
          }
          id_neighbor_table[id] = neighbor_holder;
          break;
        }
      }
    }
    fclose(file);
  } else {
    cout << "Page File Open Error" << endl;
  }

  bench_finish("Load Pages");
  cout << "Loaded:" << page_count << " pages." << endl;

  bench_start("Load Redirects");
  string redirects_filename = string(filename) + ".redirects";
  FILE* redirects_file = NULL;
  if ((redirects_file = fopen((char*)redirects_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,redirects_file)) {
      char* first = NULL;
      char* second = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '>') {
          buffer[i] = '\0';
          first = buffer;
          buffer[length-1] = '\0';
          second = buffer + i + 1;
          if (!first || !second)
            break;

          int id = atoi(second);
          page_neighbors_t* canonical = id_neighbor_table[id];
          if (canonical) {
            char* key = strdup(first);
            neighbors_[key] = canonical;
          }
          break;
        }
      }
    }
    fclose(redirects_file);
  } else {
    cout << "Redirects File Open Error" << endl;
  }


  bench_finish("Load Redirects");

#define BIG_BUF_SIZE 10000000
  bench_start("Load Distances");
  string distance_filename = string(filename) + ".distances";
  FILE* distance_file = NULL;
  if ((distance_file = fopen((char*)distance_filename.c_str(),"rb"))) {
    /*
    IntNeighborHash::const_iterator res;
    IntNeighborHash::const_iterator end = id_neighbor_hash.end();
    */
    serialized_relationship_t* rel;
    char* large_buf = (char*) calloc(BIG_BUF_SIZE,sizeof(serialized_relationship_t));
    while(!feof(distance_file)) {
      page_neighbors_t* first = NULL, *second = NULL;
      int elems_read = fread(large_buf,sizeof(serialized_relationship_t),BIG_BUF_SIZE,distance_file);
      for(int i=0; i < elems_read; i++) {
        rel = (serialized_relationship_t*) ((char*)large_buf + (i * sizeof(serialized_relationship_t)));
        first = id_neighbor_table[rel->first_id];
        second = id_neighbor_table[rel->second_id];
        if (first && second)
          add_to_neighbors(first,second->title,rel->distance);
      }
    }

    free(large_buf);
    fclose(distance_file);
  } else {
    cout << "Distance File Open Error" << endl;
  }
  bench_finish("Load Distances");
  free(id_neighbor_table);
}

void
WikipediaNeighborSetImpl::neighbors(RelationshipList& results, const string& key)
{
  relationship_t rel;
  TitleNeighborsHash::iterator res = neighbors_.find((char*)key.c_str());
  if (res != neighbors_.end()) {
    page_neighbors_t* neighbor_holder = res->second;
    if (!neighbor_holder->ambiguous) {
      for(int i=0; i < neighbor_holder->neighbors_size; i++) {
        //cout << "Found " << key << "::" << neighbor_holder->neighbors[i].title << "::" << (int) neighbor_holder->neighbors[i].distance << endl;
        rel.first = key;
        rel.second = neighbor_holder->neighbors[i].title;
        rel.distance = neighbor_holder->neighbors[i].distance; 
        results.push_back(rel);
      }
    } else {
      cout << key << " is ambiguous" << endl;
    }
  }
}

void
WikipediaNeighborSetImpl::distances(RelationshipList& results, const TupleList& titles)
{
  bench_start("Distances");
  for(TupleList::const_iterator ii = titles.begin(); ii != titles.end(); ii++) {
    cout << "Doing search for '" << *ii << "'" << endl;
    neighbors(results,*ii);
    //final.insert(final.begin(),neighbor_list.begin(),neighbor_list.end());
  }

  bench_finish("Distances");
}

void 
WikipediaNeighborSetImpl::map(void(* fn_ptr)(const string& title,const bool ambiguous,const vector<relationship_t>& rels))
{
  for(TitleNeighborsHash::iterator ii = neighbors_.begin(); ii != neighbors_.end(); ii++) {
    page_neighbors_t* cont = ii->second;
    string title = string(ii->first);
    vector<relationship_t> rels;
    for(int i =0; i < cont->neighbors_size; i++) {
      relationship_t rel;
      rel.first = title;
      if (cont->neighbors[i].title)
        rel.second = cont->neighbors[i].title;
      rel.distance = cont->neighbors[i].distance;
      rels.push_back(rel);
    }
    fn_ptr(title,cont->ambiguous,rels);
  }
}



/* Singleton Initialization */
WikipediaNeighborSet* WikipediaNeighborSet::instance_ = NULL;
WikipediaNeighborSet* WikipediaNeighborSet::instance() {
  if (!instance_) {
    instance_ = new WikipediaNeighborSetImpl();
  }
  return instance_;
}

/* Protected Implementation */
page_neighbors_t*
WikipediaNeighborSetImpl::new_neighbor_container(const char* title)
{
  TitleNeighborsHash::iterator res = neighbors_.find((char*)title);
  if (res != neighbors_.end()) {
    return res->second;
  } else {
    page_neighbors_t* neighbor_holder = (page_neighbors_t*) calloc(1,sizeof(page_neighbors_t));
    char* key = strdup(title);
    neighbor_holder->title = key;
    neighbors_[key] = neighbor_holder;
    return neighbor_holder;
  }
}

/* Static Implementation */

/*
static bool compare_distances(relationship_t& first, relationship_t& second)
{
  return (first.distance < second.distance);
}
*/

static void add_to_neighbors(page_neighbors_t* page,const char* neighbor_key,unsigned char distance)
{
  if (page->neighbors) {
    page->neighbors = (page_edge_t*) realloc(page->neighbors,sizeof(page_edge_t) * (page->neighbors_size + 1));
    page->neighbors[page->neighbors_size].title = (char*) neighbor_key;
    page->neighbors[page->neighbors_size].distance = distance;
    page->neighbors_size++;
  } else {
    page->neighbors = (page_edge_t*) malloc(sizeof(page_edge_t));
    page->neighbors[0].title = (char*) neighbor_key;
    page->neighbors[0].distance = distance;
    page->neighbors_size = 1;
  }
}
