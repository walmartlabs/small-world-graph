#ifndef __WIKIPEDIA_IMAGE_IMPL_
#define __WIKIPEDIA_IMAGE_IMPL_

#include "wikipedia_image.h"
#include <google/sparse_hash_map>
#include "paul_hsieh_hash.h"
#include "equality.h"

using google::sparse_hash_map;

typedef sparse_hash_map<string, WikipediaImage*, PaulHsiehSTLHash, eqstring> ImageMap;

class WikipediaImageImpl : public WikipediaImage 
{
  public:
    WikipediaImageImpl(const string& title);
};

class WikipediaImageManagerImpl : public WikipediaImage::Manager
{
  public:
    WikipediaImageManagerImpl() {}
    WikipediaImage* new_image(const string& title);
    size_t size();

  protected:
    ImageMap images_;
};

#endif
