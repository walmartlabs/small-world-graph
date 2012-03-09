#include "wikipedia_image_impl.h"
#include "md5.h"
#include "string_utils.h"
#include <iostream>
#include <sstream>

using namespace std;

#define MD5_DIGEST_LENGTH 32
static char* md5(const char* data, int len)
{
  unsigned char digest[16];
  MD5_CTX context;
  MD5Init(&context);
  MD5Update(&context,(unsigned char*) data,len);
  MD5Final(digest,&context);
  char* final_digest = (char*) malloc(MD5_DIGEST_LENGTH + 1);
  snprintf(final_digest,MD5_DIGEST_LENGTH + 1,
    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],
    digest[6],digest[7],digest[8],digest[9],digest[10],digest[11],
    digest[12],digest[13],digest[14],digest[15]);
  return final_digest;
}

WikipediaImageImpl::WikipediaImageImpl(const string& title)
{
  title_ = title;
  string link_title = replace_all(title," ","_");
  char* md5_of_title = md5(link_title.c_str(),link_title.size());
  stringstream linkstrm;
  linkstrm << "/" << md5_of_title[0] << "/" << md5_of_title[0] << md5_of_title[1] << "/" << link_title;
  link_ = linkstrm.str();
  free(md5_of_title);
}

WikipediaImage* WikipediaImageManagerImpl::new_image(const string& title)
{
  ImageMap::const_iterator res = images_.find(title);
  if (res != images_.end()) {
    return res->second;
  } else {
    WikipediaImageImpl* image = new WikipediaImageImpl(title);
    images_[title] = image;
    return image;
  }
}

size_t
WikipediaImageManagerImpl::size()
{
  return images_.size();
}

WikipediaImage::Manager* WikipediaImage::Manager::instance_ = NULL;
WikipediaImage::Manager* WikipediaImage::Manager::instance() {
  if (!instance_) {
    //__blacklist = new WordList(WordList::BLACK);
    instance_ = new WikipediaImageManagerImpl();
  }
  return instance_;
}
