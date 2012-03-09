#include "s3_helper.h"
#include <iostream>
#include <fstream>
#include <signal.h>
#include <string.h>

using namespace std;

struct file_buffer {
  char* buffer;
  uint32_t size;
};

static file_buffer open_image_file(const char* filename)
{
  int length;
  char * buffer;

  ifstream is;
  is.open (filename, ios::binary );

  // get length of file:
  is.seekg (0, ios::end);
  length = is.tellg();
  is.seekg (0, ios::beg);

  // allocate memory:
  buffer = new char [length];

  // read data as a block:
  is.read (buffer,length);
  is.close();

  file_buffer fb;
  fb.buffer = buffer;
  fb.size = length;
  return fb;
}

static char* mime_type(const char* image_path) 
{
  char* extension = strrchr((char*)image_path,'.');
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

int main(int argc, char **argv) {
  char* filename = argv[1];
  char* bucket = argv[2];
  char* dest_path = argv[3];

  if (!filename || !bucket || !dest_path) {
    printf("Please specify a filename,a bucket, and a destination path\n");
    exit(-1);
  }

  file_buffer fb = open_image_file(filename);

  shared_ptr<S3Helper> __sh = S3Helper::instance();
  __sh->put(bucket,dest_path,fb.buffer,fb.size,mime_type(dest_path),true,true);
}
