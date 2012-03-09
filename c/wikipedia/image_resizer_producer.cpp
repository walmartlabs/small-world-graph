#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>
#include <signal.h>

#include <getopt.h>
#include "ImageDownloadQueue.h"
#include <google/dense_hash_set>
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "s3_helper.h"
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using google::dense_hash_set;

typedef dense_hash_set<const char*,PaulHsiehHash,eqstr> ImageNameSet;

/* Globals */
static volatile sig_atomic_t gracefully_quit = 0;

int main(int argc, char **argv) {
  // Ignore SIGPIPES
  signal(SIGPIPE,SIG_IGN);
  char* host = (char*) "localhost";
  int port = 9091;
  int longest_side = 0;
  int c;
  int index;

  ImageNameSet processed_images;
  processed_images.set_empty_key(NULL);
  shared_ptr<S3Helper> sh = S3Helper::instance();

  while ((c = getopt(argc, argv, "h:p:s:")) != -1) {
    switch (c) {
      case 'h':
        host = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 's':
        longest_side = atoi(optarg);
        break;
    }
  }

  if (port == 0) {
    cout << "Invalid port" << endl;
    exit(-1);
  }

  if (longest_side == 0) {
    cout << "Invalid longest side" << endl;
    exit(-1);
  }
  
  index = optind;
  char* source_bucket = argv[index];
  if (!source_bucket) {
    cout << "Please specify a source bucket" << endl;
    exit(-1);
  }

  stringstream dest_bucket_strm;
  dest_bucket_strm << "climages" << longest_side;
  string dest_bucket = dest_bucket_strm.str();

  shared_ptr<TTransport> socket(new TSocket(host,port));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  ImageDownloadQueueClient client(protocol);

  FileList* dest_files = sh->ls(dest_bucket.c_str());
  for(FileList::const_iterator ii = dest_files->begin(); ii != dest_files->end(); ii++) {
    processed_images.insert(*ii);
  }
  free(dest_files);

  cout << processed_images.size() << " already in destination bucket." << endl;

  FileList* source_files = sh->ls(source_bucket);
  cout << source_files->size() << " images in source bucket." << endl;
  int counter = 0;
  vector<ImageWorkUnit> units;
  for(FileList::const_iterator ii = source_files->begin(); ii != source_files->end(); ii++) {
    ImageNameSet::const_iterator res = processed_images.find(*ii);
    if (res == processed_images.end()) {
      ImageWorkUnit unit;
      unit.bucket = source_bucket;
      unit.link = *ii;
      unit.desired_size = longest_side;
      units.push_back(unit);
      counter++;
      if (counter && counter % 10000 == 0) {
inner_retry:
        try {
          transport->open();
          client.enqueue(units);
          transport->close();
        } catch (TException &tx) {
          printf("ERROR: %s\n", tx.what());
          printf("Sleeping for 10s before retry\n");
          sleep(10);
          goto inner_retry;
        }
        units.clear();
      }
    }
  }
outer_retry:
  try {
    transport->open();
    client.enqueue(units);
    transport->close();
  } catch (TException &tx) {
    printf("ERROR: %s\n", tx.what());
    printf("Sleeping for 10s before retry\n");
    goto outer_retry;
  }
  cout << counter << " images enqueued." << endl;
  free(source_files);
}
