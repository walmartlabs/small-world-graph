#include "wikipedia_sparse_graph.h"
#include "wikipedia_types.h"
#include "benchmark.h"
#include <algorithm>
#include <vector>
#include <queue>
#include <boost/pending/relaxed_heap.hpp>
#include "radixheap.h"
#include <stdlib.h>
#include <valgrind/callgrind.h>

using namespace std;
using namespace boost;

#define NUMBER_OF_PATHS 10000000

int main(int argc,char** argv)
{
  bench_start("Randomizing Paths");
  srand(NULL);
  shortest_path_t** paths = (shortest_path_t**) malloc(sizeof(shortest_path_t*) * NUMBER_OF_PATHS);
  for(int i=0; i < NUMBER_OF_PATHS; i++) {
    shortest_path_t* path = new shortest_path_t();
    page_t* page = new page_t();
    page->id = i;
    path->end = page;
    path->distance = rand() % (255 * 6);
    paths[i] = path;
  }
  bench_finish("Randomizing Paths");

  /*
  bench_start("Test Priority Queue");
  priority_queue<shortest_path_t*,vector<shortest_path_t*>,shortest_path_compare_t> queue;

  for(int i=0; i < NUMBER_OF_PATHS; i++) {
    shortest_path_t* path = paths[i];
    queue.push(path);
  }
  while(!queue.empty()) {
    queue.pop();
  }
  bench_finish("Test Priority Queue");
  */

  /*
  bench_start("Test RadixHeap Queue");
  {
    RadixHeap heap(NUMBER_OF_PATHS);
    for(int i=0; i < NUMBER_OF_PATHS; i++) {
      shortest_path_t* path = paths[i];
      int key = path->distance;
      heap.insert(i,key);
    }
    while(heap.nItems() > 0) {
      heap.deleteMin();
    }
  }
  bench_finish("Test RadixHeap Queue");
  */

  bench_start("Test BucketQueue");
  CALLGRIND_START_INSTRUMENTATION;
  {
    BucketQueue bqueue(NUMBER_OF_PATHS);
    for(int i=0; i < NUMBER_OF_PATHS; i++) {
      shortest_path_t* path = paths[i];
      bqueue.insert(path->end,path->distance,1);
    }
    shortest_path_t* path;
    while((path = bqueue.delete_min())) {
    }
  }

  CALLGRIND_STOP_INSTRUMENTATION;
  bench_finish("Test BucketQueue");

  for(int i=0; i < NUMBER_OF_PATHS; i++) {
    free(paths[i]);
  }
  free(paths);

  sleep(10);
}
