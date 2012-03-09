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

#include "gd_helper.h"
#include "s3_helper.h"

#include "boost/filesystem.hpp"   // includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string.hpp>

#include "svg_helper.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace boost;
using namespace boost::filesystem;   

/* Globals */
static shared_ptr<GDImage::Manager> __mim;
static shared_ptr<S3Helper> __sh;
static volatile sig_atomic_t gracefully_quit = 0;

static char* find_extension(const char* filename)
{
  char* extension = strrchr(filename,'.');
  if (!extension)
    extension = (char*) "jpg";
  else
    extension = extension + 1;
  return extension;
}

static  char* mime_type(const char* image_path) 
{
  char* extension = strrchr(image_path,'.');
  if (!extension || strlen(extension) < 2)
    return (char*) "binary/octet-stream";
  else
    extension = extension + 1;
  if (!strncasecmp(extension,"jpg",3) || !strncasecmp(extension,"jpeg",4))
    return (char*) "image/jpeg";
  else if (!strncasecmp(extension,"png",3))
    return (char*) "image/png";
  else if (!strncasecmp(extension,"gif",3))
    return (char*) "image/gif";
  else if (!strncasecmp(extension,"svg",3)) // we actually return png because in this case we have converted it
    return (char*) "image/png";
  else {
    return (char*) "binary/octet-stream";
  }
}


/* Use boost filesystem to write it in the proper location */
static void write_image_to_filesystem(const char* image_path, const char* data, int len)
{
  vector<string> buckets;
  split(buckets,image_path,is_any_of("/"));
  if (buckets.size() == 3) {
    current_path("/tmp");
    string first_bucket = buckets[0];
    string second_bucket = buckets[1];
    path directory(first_bucket + "/" + second_bucket);
    string filename = buckets[2];
    create_directories(directory);
    ofstream file((directory / filename).string().c_str(),ios_base::binary);
    file << string(data,len);
    file.close();
  }
}

static void handle_sigint(int signal) {
  if (!gracefully_quit) {
    cout << "Sent Quit Signal" << endl;
    gracefully_quit = 1;
  } else {
    exit(0);
  }
}

int main(int argc, char **argv) {
  // Ignore SIGPIPES
  signal(SIGPIPE,SIG_IGN);
  signal(SIGINT,handle_sigint);
  char* host = (char*) "localhost";
  int port = 9091;
  int c;

  __sh = S3Helper::instance();
  __mim = GDImage::Manager::instance();

  while ((c = getopt(argc, argv, "h:p")) != -1) {
    switch (c) {
      case 'h':
        host = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        if (port == 0) {
          cout << "Invalid port" << endl;
          exit(-1);
        }
        break;
    }
  }
  shared_ptr<TTransport> socket(new TSocket(host,port));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  ImageDownloadQueueClient client(protocol);

  for(;;) {
    try {
      if (gracefully_quit == 1) {
        cout << "Gracefully Quitting" << endl;
        exit(0);
      }
      transport->open();
      vector<ImageWorkUnit> work_units;
      client.dequeue(work_units);
      transport->close();
      if(work_units.size() > 0) {
        cout << "DEQUEUED: " << work_units.size() << endl;
        for(vector<ImageWorkUnit>::const_iterator ii = work_units.begin(); ii != work_units.end(); ii++) {
          ImageWorkUnit unit = *ii;
          char* data = NULL;
          uint32_t data_len;
          printf("START DOWNLOAD: %s\n", unit.link.c_str());
          data_len = __sh->get(unit.bucket.c_str(),unit.link.c_str(),&data);
          printf("FINISH DOWNLOAD: %s\n", unit.link.c_str());
          if (data_len > 0) {
            stringstream dest_bucket_strm;
            dest_bucket_strm << "climages" << unit.desired_size;
            string dest_bucket = dest_bucket_strm.str();
            char* extension = find_extension(unit.link.c_str());
            if (!strcasecmp("svg",extension)) {
              cout << "START SVG: " << unit.link << endl;
              char* converted_svg = NULL;
              int converted_size = svg_convert(data,data_len,&converted_svg,unit.desired_size);
              if (converted_svg) {
                cout << "DONE SVG: " << unit.link << endl;
                //__sh->put(dest_bucket.c_str(),unit.link.c_str(),converted_svg,converted_size,mime_type(unit.link.c_str()),true,true);
              } else {
                cout << "ERROR SVG: " << unit.link << endl;
              }
              free(converted_svg);
            } else {
              try {
                cout << "START GD: " << unit.link << endl;
                shared_ptr<GDImage> mi = __mim->new_image(extension,data,data_len);
                mi->floor(unit.desired_size);
                gd_image_data_t* gd_data = mi->data();
                if (gd_data && gd_data->data) {
                  cout << "DONE GD: " << unit.link << endl;
                  //__sh->put(dest_bucket.c_str(),unit.link.c_str(),gd_data->data,gd_data->size,mime_type(unit.link.c_str()),true,true);
                } else {
                  cout << "ERROR GD:" << unit.link << endl;
                }
                free_gd_image_data(gd_data);
              } catch (gd_exception& gde) {
                cout << "ERROR GD:" << unit.link << endl;
              }
            }
          } else {
            cout << "ERROR FILE: " << unit.link << endl;
          }
          free(data);
        }
      } else {
        cout << "Queue is Empty" << endl;
        sleep(10);
      }
    } catch (TException &tx) {
      printf("ERROR: %s\n", tx.what());
      printf("Sleeping for 10s before retry\n");
      sleep(10);
    }
  }
}
