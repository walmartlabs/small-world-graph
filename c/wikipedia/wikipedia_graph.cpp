#include "wikipedia_graph.h"
#include "wikipedia_types.h"
#include "benchmark.h"
#include <algorithm>
#include <vector>
#include <queue>

using namespace std;

static inline void add_to_inlinks(page_t* page,page_t* inlink);
static inline void add_to_outlinks(page_t* page,page_t* outlink);
static inline page_array_t* connected_pages(page_t* page);

page_t*
WikipediaGraph::page(const char* title)
{
  TitlePageHash::const_iterator res = pages_.find((char*)title);
  if (res != pages_.end()) {
    return res->second;
  } else {
    return NULL;
  }
}


void
WikipediaGraph::load(const char* filename)
{
  bench_start("Load");
  FILE* file = NULL;
  char buffer[4096];
  string line;
  page_t blank;
  page_t* current_page;
  blank.title = (char*) "";
  current_page = &blank;

  bench_start("Load Pages");
  page_t** id_page_table = NULL;
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
          char* key = strdup(second);
          page_t* page = (page_t*) calloc(1,sizeof(page_t));
          page->title = key;
          pages_[key] = page;
          page_count++;
          if (id > (int) biggest_id_so_far) {
            id_page_table = (page_t**) realloc(id_page_table,sizeof(page_t*) * (id + 1));
            for(int k = biggest_id_so_far + 1; k < id + 1; k++) {
              id_page_table[k] = NULL;
            }
            biggest_id_so_far = id;
          }
          id_page_table[id] = page;
          break;
        }
      }
    }
    fclose(file);
  } else {
    cout << "Page File Open Error" << endl;
  }
  bench_finish("Load Pages");

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

          TitlePageHash::iterator res = pages_.find((char*)second);
          if (res != pages_.end()) {
            page_t* canonical = res->second;
            pages_[strdup(first)] = canonical;
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
  bench_start("Load Outlinks");
  string outlink_filename = string(filename) + ".outlinks";
  FILE* outlink_file = NULL;
  if ((outlink_file = fopen((char*)outlink_filename.c_str(),"rb"))) {
    serialized_outlink_t* rel;
    char* large_buf = (char*) calloc(BIG_BUF_SIZE,sizeof(serialized_relationship_t));
    while(!feof(outlink_file)) {
      page_t* first = NULL, *second = NULL;
      int elems_read = fread(large_buf,sizeof(serialized_outlink_t),BIG_BUF_SIZE,outlink_file);
      for(int i=0; i < elems_read; i++) {
        rel = (serialized_outlink_t*) ((char*)large_buf + (i * sizeof(serialized_outlink_t)));
        first = id_page_table[rel->first_id];
        second = id_page_table[rel->second_id];
        if (first && second) {
          add_to_outlinks(first,second);
          add_to_inlinks(second,first);
        }
      }
    }

    free(large_buf);
    fclose(outlink_file);
  } else {
    cout << "Outlink File Open Error" << endl;
  }
  bench_finish("Load Outlinks");
  free(id_page_table);

  bench_finish("Load");
}


void
WikipediaGraph::clear()
{
  for(TitlePageHash::iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
    if (ii->first == ii->second->title) {
      free(ii->second->inlinks);
      free(ii->second->outlinks);
      free(ii->second->title);
      free(ii->second);
    } else {
      free(ii->first);
    }
  }
  pages_.clear();
  pages_.resize(0);
}

/* We do a modified version of Dijkstra's Algorithm here */
void 
WikipediaGraph::page_chain(PageList& chain, PageList& referring_pages, const string& first, const string& second)
{
  bench_start("Page Chain");
  TitlePageHash::const_iterator res = pages_.find((char*) first.c_str());
  TitlePageHash::const_iterator end = pages_.end();
  if (res != end) {
    page_t* first = res->second;
    res = pages_.find((char*) second.c_str());
    if (res != end) {
      page_t* second = res->second;
      priority_queue<shortest_path_t*,vector<shortest_path_t*>,shortest_path_compare_t> queue;
      PagePathHash path_distances;
      PagePageHash previous;
      PageSet visited;

      previous.set_empty_key(NULL);
      path_distances.set_empty_key(NULL);
      visited.set_empty_key(NULL);

      shortest_path_t* identity = (shortest_path_t*) calloc(1,sizeof(shortest_path_t));
      identity->distance = 0;
      identity->end = first;
      queue.push(identity);

      while(queue.size() > 0) {
        shortest_path_t* path = queue.top();
        queue.pop();
        page_t* current = path->end;
        //cout << "Popped ('" << current->title << "'," << path->distance << ")" << endl;
        free(path);

        if (current == second) {
          //cout << "Short Circuit" << endl;
          break;
        }

        PageSet::const_iterator is_visited = visited.find(current);
        if (is_visited != visited.end()) {
          //cout << "Skipping:" << current->title << endl;
          continue;
        }

        visited.insert(current);

        int dist_current = 0;
        int links_current = 0;
        PagePathHash::const_iterator res = path_distances.find(current);
        if (res != path_distances.end()) {
          dist_current = res->second.distance;
          links_current = res->second.links;
        }


        page_array_t* edges = connected_pages(current);

        for(size_t i=0; i < edges->length; i++) {
          shortest_path_t relaxed_path;
          page_t* end = edges->array[i];
          int weight = end->inlinks_size;
          relaxed_path.distance = dist_current + weight;
          relaxed_path.links = links_current + 1;
          size_t dist_end = INT_MAX;
          size_t links_to_end = 0;
          //cout << "('" << current->title << "','" << end->title << "'," << relaxed << ")" << endl; 

          PagePathHash::const_iterator it = path_distances.find(end);
          if (it != path_distances.end()) {
            dist_end = it->second.distance;
            links_to_end = it->second.links;
          }

          if (relaxed_path.distance < dist_end) {
            previous[end] = current;
            path_distances[end] = relaxed_path;
            dist_end = relaxed_path.distance;
            links_to_end = relaxed_path.links;
          }

          if (links_to_end > 5)
            continue;

          shortest_path_t* new_path = (shortest_path_t*) calloc(1,sizeof(shortest_path_t));
          new_path->distance = dist_end;
          new_path->links = links_to_end;
          new_path->end = end;
          //cout << "Enqueued: ('" << end->title << "'," << path->distance << ")" << endl;
          queue.push(new_path);
          cout << "QueueSize:" << queue.size() << endl;
        }

        free(edges->array);
        free(edges);
      }

      while(queue.size() > 0) {
        shortest_path_t* path = queue.top();
        queue.pop();
        free(path);
      }

      // Now trace back through previous.
      list<page_t*> path;
      page_t* cur = second;
      path.push_front(cur);
      for(;;) {
        PagePageHash::const_iterator res = previous.find(cur);
        if (res != previous.end()) {
          cur = res->second;
          path.push_front(cur);
          if (cur == first)
            break;
        } else {
          cout << "No Chain Found" << endl;
          break;
        }
      }
      for(list<page_t*>::const_iterator ii = path.begin(); ii != path.end(); ii++) {
        cout << (*ii)->title << endl;
      }
    }
  }
  bench_finish("Page Chain");
}

/* Singleton Initialization */
WikipediaGraph* WikipediaGraph::instance_ = NULL;
WikipediaGraph* WikipediaGraph::instance() {
  if (!instance_) {
    instance_ = new WikipediaGraph();
  }
  return instance_;
}

/* Static Implementation */

static void inline add_to_inlinks(page_t* page,page_t* inlink)
{
  if (page->inlinks) {
    page->inlinks = (page_t**) realloc(page->inlinks,sizeof(page_t*) * (page->inlinks_size + 1));
    page->inlinks[page->inlinks_size] = inlink;
    page->inlinks_size++;
  } else {
    page->inlinks = (page_t**) malloc(sizeof(page_t*));
    page->inlinks[0] = inlink;
    page->inlinks_size = 1;
  }
}

static void add_to_outlinks(page_t* page,page_t* outlink)
{
  if (page->outlinks) {
    page->outlinks = (page_t**) realloc(page->outlinks,sizeof(page_t*) * (page->outlinks_size + 1));
    page->outlinks[page->outlinks_size] = outlink;
    page->outlinks_size++;
  } else {
    page->outlinks = (page_t**) malloc(sizeof(page_t*));
    page->outlinks[0] = outlink;
    page->outlinks_size = 1;
  }
}

/* Append the inlinks and outlinks of a page into the edges array.  It is NULL terminated */
static inline page_array_t* connected_pages(page_t* page)
{
  PageSet edge_end_nodes;
  page_array_t* edges = (page_array_t*) malloc(sizeof(page_array_t));
  edges->array = (page_t**) malloc(sizeof(page_t) * (page->inlinks_size + page->outlinks_size));
  edges->length = 0;
  for(int i=0; i < page->inlinks_size; i++) {
    edges->array[edges->length] = page->inlinks[i];
    edges->length++;
  }
  for(int i=0; i < page->outlinks_size; i++) {
    edges->array[edges->length] = page->outlinks[i];
    edges->length++;
  }
  return edges;
}
