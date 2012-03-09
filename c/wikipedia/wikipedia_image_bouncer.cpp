#include <iostream>
#include <string>
#include <fstream>
#include <signal.h>

#include "benchmark.h"
#include "curl_helper.h"
#include "s3_helper.h"
#include "tuple_image_mapper.h"
#include "thread_safe_queue.h"
#include <google/dense_hash_set>
#include "equality.h"
#include "paul_hsieh_hash.h"

#include <boost/algorithm/string.hpp>
#include "boost/filesystem.hpp"   // includes all needed Boost.Filesystem declarations

using namespace boost;
using namespace boost::filesystem;   
using google::dense_hash_set;

typedef dense_hash_set<const char*,PaulHsiehHash,eqstr> ImageNameSet;
typedef dense_hash_set<const char*,PaulHsiehHash,eqstr> S3KeySet;

#define MAX_LONGEST_SIDE 640
#define BUCKET_NAME "cl_raw_wikipedia_images"
#define WIKI_UPLOAD "http://upload.wikimedia.org/wikipedia/en"
#define WIKI_COMMONS "http://upload.wikimedia.org/wikipedia/commons"
#define IMAGE_ROOT "/mnt/wikipedia_images/"

struct full_image_t {
  const char* name;
  const char* image_path;
  raw_curl_data_t* cd;

  char* mime_type() 
  {
    const char* extension = strrchr(image_path,'.');
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
    else if (!strncasecmp(extension,"svg",3))
      return (char*) "image/svg";
    else {
      return (char*) "binary/octet-stream";
    }
  }
};


/* Globals */
static uint32_t failure_count = 0;
static uint32_t success_count = 0;
static pthread_mutex_t failure_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t success_m = PTHREAD_MUTEX_INITIALIZER;

static S3KeySet __key_set;
static ImageNameSet __image_name_set;
static shared_ptr<CurlHelper> __ch;
static TupleImageMapper* __tim;
static shared_ptr<S3Helper> __sh;
static ThreadSafeQueue<full_image_t*> __download_q;
static ThreadSafeQueue<full_image_t*> __upload_q(1000);
static bool download_complete = false;
static bool upload_complete = false;

/* End Globals */

/*
static char* find_extension(const char* filename)
{
  char* extension = strrchr(filename,'.');
  if (!extension)
    extension = (char*) "jpg";
  else
    extension = extension + 1;
  return extension;
}
*/

static char* image_link_to_en_wiki_link(const char* wiki_link)
{
  stringstream thumb_link;
  //thumb_link << WIKI_UPLOAD << wiki_link << "/125px-" << strrchr(wiki_link,'/') + 1;
  thumb_link << WIKI_UPLOAD << wiki_link; 
  return strdup(thumb_link.str().c_str());
}

static char* image_link_to_commons_wiki_link(const char* wiki_link)
{
  stringstream thumb_link;
  //thumb_link << WIKI_COMMONS << wiki_link << "/125px-" << strrchr(wiki_link,'/') + 1;
  thumb_link << WIKI_COMMONS << wiki_link;
  return strdup(thumb_link.str().c_str());
}

/* Download all the keys from the CLC bucket and put them in the set so we can
 * ensure we don't redownload */
static void fill_key_set() 
{
  bench_start("Fill Key Set");
  FileList* fl = __sh->ls(BUCKET_NAME);
  for(FileList::const_iterator ii = fl->begin(); ii != fl->end(); ii++) {
    char* key = *ii;
    __key_set.insert(key);
  }
  delete fl;
  bench_finish("Fill Key Set");
}

static bool already_downloaded(const char* key)
{
  S3KeySet::const_iterator res = __key_set.find(key + 1);
  return res != __key_set.end();
}

static bool already_enqueued(const char* key)
{
  ImageNameSet::const_iterator res = __image_name_set.find(key + 1);
  return res != __image_name_set.end();
}

static void enqueue_image_to_download(const char* name,const char* url)
{
  if (!already_downloaded(url)) {
    if(!already_enqueued(url)) {
      full_image_t* fit = (full_image_t*) malloc(sizeof(full_image_t));
      fit->name = name;
      fit->image_path = url;
      //cout << "Enqueuing " << name << " :: " << url << " to download" << endl;
      __download_q.enqueue(fit);
      __image_name_set.insert(url + 1);
    } else {
      //cout << "Skipping " << url << endl;
    }
  } else {
    //cout << "Skipping " << url << endl;
  }
}

/* Use boost filesystem to write it in the proper location */
static void write_image_to_filesystem(const char* image_path, const char* data, int len)
{
  vector<string> buckets;
  split(buckets,image_path,is_any_of("/"));
  if (buckets.size() == 4) {
    //current_path("/mnt/wikipedia_images");
    string first_bucket = buckets[1];
    string second_bucket = buckets[2];
    path directory("/mnt/wikipedia_images/" + first_bucket + "/" + second_bucket);
    string filename = buckets[3];
    create_directories(directory);
    ofstream file((directory / filename).string().c_str(),ios_base::binary);
    file << string(data,len);
    file.close();
  }
}

/* Pull off Image's from the download queue to download them and put them on the resize queue*/
static void* download(void*) {
  signal(SIGPIPE,SIG_IGN);
  for(;;) {
    try {
      full_image_t* img = __download_q.dequeue();
      char* en_link = image_link_to_en_wiki_link(img->image_path);
      raw_curl_data_t* cd = __ch->fetch_raw(en_link);
      //char* extension = find_extension(image_path);
      if (cd && cd->len > 0 && (cd->response_code != 404 && cd->response_code != 500)) {
        img->cd = cd;
        write_image_to_filesystem(img->image_path,cd->data,cd->len);
        __upload_q.enqueue(img);
        pthread_mutex_lock(&success_m);
        success_count++;
        pthread_mutex_unlock(&success_m);
        //cout << "DONE: " << en_link << endl;
      } else {
        if (cd) {
          free(cd->data);
          free(cd->effective_url);
        }
        char* commons_link = image_link_to_commons_wiki_link(img->image_path);
        //printf("Fallback to Commons Link %s\n",commons_link);
        cd = __ch->fetch_raw(commons_link);
        if (cd && cd->len > 0 && (cd->response_code != 404 && cd->response_code != 500)) {
          img->cd = cd;
          write_image_to_filesystem(img->image_path,cd->data,cd->len);
          __upload_q.enqueue(img);
          pthread_mutex_lock(&success_m);
          success_count++;
          pthread_mutex_unlock(&success_m);
          //cout << "DONE: " << commons_link << endl;
        } else {
          if (cd) {
            free(cd->data);
            free(cd->effective_url);
          }
          pthread_mutex_lock(&failure_m);
          failure_count++;
          pthread_mutex_unlock(&failure_m);
          cout << "FAILED: " << img->name << " :: " << img->image_path << endl;
        }
        free(commons_link);
      }
      free(en_link);
    } catch (QueueEmptyException& qe) {
      return NULL;
    }
  }
}

static void* upload(void*)
{
  signal(SIGPIPE,SIG_IGN);
  for(;;) {
    try {
      full_image_t* image= __upload_q.dequeue();
      cout << "Start Uploading " << image->image_path << " to S3" << endl;
      __sh->put(BUCKET_NAME,image->image_path + 1,(char*) image->cd->data,image->cd->len,image->mime_type());
      //__sh->put(BUCKET_NAME,"/c/cb/Pedro.jpg",(char*) image->cd->data,image->cd->len,image->mime_type());
      cout << "Finish Uploading " << image->image_path << " to S3" << endl;
      free(image->cd->effective_url);
      free(image->cd->data);
      free(image->cd);
      free(image);
    } catch (QueueEmptyException& qe) {
      if (download_complete) {
        upload_complete = true;
        cout << "Upload Thread Finish" << endl;
        return NULL;
      }
      cout << "Upload Queue Empty: Sleep" << endl;
      sleep(5);
    }
  }
}

static void* status(void*) {
  cout << "Reporter Thread Start" << endl;
  while(!upload_complete) {
    cout << "Download Queue Size: " << __download_q.size() << endl;
    cout << "Upload Queue Size: " << __upload_q.size() << endl;
    cout << "Download Success Count: " << success_count << endl;
    cout << "Download Failure Count: " << failure_count << endl;
    sleep(5);
  }
  cout << "Reporter Thread Finish" << endl;
  return NULL;
}


#define NUM_DOWNLOAD_THREADS 3
int
main(int argc, char **argv)
{
  signal(SIGPIPE,SIG_IGN);
  pthread_t download_threads[NUM_DOWNLOAD_THREADS];
  pthread_t upload_thread;
  pthread_t reporter_thread;
  int error;

  __tim = TupleImageMapper::instance();
  __ch = CurlHelper::current_instance();
  __sh = S3Helper::instance();

  __tim->load("/mnt/wikipedia/image_tuples");

  __key_set.set_empty_key(NULL);
  __image_name_set.set_empty_key(NULL);

  fill_key_set();
  __tim->map(enqueue_image_to_download);
  //enqueue_image_to_download("John McCain","/9/93/John_McCain_official_portrait_with_alternative_background.jpg");

  cout << __key_set.size() << " images already downloaded to S3." << endl;
  cout << __image_name_set.size() << " images left to download." << endl;

  for(int i=0; i< NUM_DOWNLOAD_THREADS; i++) {
    error = pthread_create(&download_threads[i], NULL, /* default attributes please */ download,NULL); 
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
  }

  error = pthread_create(&upload_thread,NULL,upload,NULL);
  error = pthread_create(&reporter_thread,NULL,status,NULL);

  for(int i=0; i< NUM_DOWNLOAD_THREADS; i++) {
    error = pthread_join(download_threads[i], NULL);
  }

  download_complete = true;

  pthread_join(upload_thread,NULL);
  pthread_join(reporter_thread,NULL);
  return 0;
}
