#ifndef __WIKIPEDIA_IMAGE__
#define __WIKIPEDIA_IMAGE__

#include <string>

using namespace std;

class WikipediaImage
{
  public:
    string title() const { return title_; }
    string link() const { return link_; }

    class Manager;

  protected:
    string title_;
    string link_;
};

class WikipediaImage::Manager
{
  public:
    static WikipediaImage::Manager* instance(); 

    /* Don't try to free this one! */
    virtual WikipediaImage* new_image(const string& title) = 0;

    virtual size_t size() = 0;

    virtual ~Manager() {}
  protected:
    static WikipediaImage::Manager* instance_;
    Manager() {}
};

#endif
