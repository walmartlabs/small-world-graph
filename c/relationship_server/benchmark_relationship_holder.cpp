#include <iostream>
#include <string>
#include <vector>

#include "relationship_holder.h"
#include <sys/resource.h>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "benchmark.h"

using namespace std;
using namespace boost::unit_test;

/* 
 * This tests loading up 5 million records and running some operations on that
 */
BOOST_AUTO_TEST_CASE( large_test ) {
  rusage memory_info;
  int rc;
  ifstream in_file("/home/cspenc/relationships");
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  if (!in_file) {
    BOOST_FAIL("No File");
  }

  vector<int> site_ids;

  string line;

  bench_start("Store");
  while(!in_file.eof()) {
    int site_id;
    int other_id;
    float distance;
    in_file >> line;
    sscanf(line.c_str(),"%d,%d,%f",&site_id,&other_id,&distance);
    rh->store(site_id,other_id,distance);
    site_ids.push_back(site_id);
  }
  bench_finish("Store");

  rc = getrusage(RUSAGE_SELF,&memory_info);
  if (rc != 0) {
    BOOST_FAIL("Cannot get resource information");
  }
  cout << "Memory Usage: " << memory_info.ru_maxrss << endl;

  vector<int> sub_set;
  for(int i=0; i < 1000; i++) {
    sub_set.push_back(site_ids[i]);
  }

  bench_start("10000 distance calls");
  for(int i =0; i < 10000; i++) {
    rh->distances(sub_set);
  }
  bench_finish("10000 distance calls");

  bench_start("10000 sites_close_to calls");
  for(int i =0; i < 10; i++) {
    for(int j=0; j < 1000; j++) {
      rh->sites_close_to(sub_set[j]);
    }
  }
  bench_finish("10000 sites_close_to calls");

  cout << rh->statistics() << endl;

}
