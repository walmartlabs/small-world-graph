#ifndef __WIKIPEDIA_PAGE__
#define __WIKIPEDIA_PAGE__

#include <string>
#include <vector>
#include "wikipedia_image.h"
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <stdint.h>

using namespace std;
using namespace boost;

class WikipediaPage;
struct WikipediaRawLink;
struct WikipediaEdgeInfo;

typedef vector<WikipediaRawLink> WikipediaRawLinkList;
typedef vector<WikipediaEdgeInfo> WikipediaEdgeInfoList;
typedef vector<WikipediaPage*> WikipediaPageList;
typedef vector<WikipediaImage*> WikipediaImageList;

struct WikipediaRawLink {
  public:
    string name;
    uint8_t count;
    bool in_text;
}; 

struct WikipediaEdgeInfo {
  WikipediaPage* begin;
  WikipediaPage* end;
  uint8_t count;
  bool in_text;

  friend ostream& operator<<(ostream& os, const WikipediaEdgeInfo& edge);
} __attribute__((__packed__));


class WikipediaPage 
{
  public:
    uint32_t id() const { return id_; }
    virtual string title() const = 0;
    bool ambiguous() const { return ambiguous_; }
    bool entity() const { return entity_; }

    /* This returns NULL if there is no representative image for this page */
    virtual WikipediaImage* representative_image() const = 0;

    virtual WikipediaPage* canonical() const = 0;

    virtual WikipediaRawLinkList* raw_outlinks() const = 0;
    virtual WikipediaEdgeInfoList* outlinks() const = 0;
    virtual WikipediaEdgeInfoList* inlinks() const = 0; 

    virtual void print_sorted_edges() const = 0;
    virtual void print_sorted_edge_statistics() const = 0;
    virtual void print_neighbors(size_t num_neighbors) const = 0;
    virtual void print_bidi_edges() const = 0;

    virtual WikipediaImageList* images() const = 0;

    virtual ~WikipediaPage() {}

    class Manager;

  protected:
    WikipediaPage() {}
    uint32_t id_;
    bool ambiguous_;
    bool entity_;
} __attribute__((__packed__));

class WikipediaPage::Manager
{
  public:
    static const uint32_t MAX_DISTANCE = 255;
    static WikipediaPage::Manager* instance(); 

    /* Don't try to free the result of this function! */
    //virtual WikipediaPage* new_alias(const string& alias_title, const string& real_title) = 0;
    virtual WikipediaPage* new_page(const string& title, const string& content) = 0;
    virtual WikipediaPage* page(const string& title) = 0;

    /* Pass in an articles.xml Wikipedia dumpfile */
    virtual void load(const string& xmlfilename) = 0;

    /* This loads wikipedia pages from the index rather trying to parse from wikipedia.xml */
    virtual void load_from_index(const string& index_root) = 0;

    /* Load up pages from a tokyo cabinet outlinks file */
    virtual void load_from_tokyo_cabinet(const string& tc_file) = 0;

    virtual void to_index(const string& index_root) = 0;
    virtual void to_bidi_graph(const string& index_root) = 0;
    virtual void to_sparse_graph(const string& index_root,uint32_t num_edges) = 0;
    virtual void to_distances(const string& index_root,uint32_t num_neighbors) = 0;
    virtual void to_priority_topic_list(const string& index_root) = 0;

    /*
     * This output method makes a tokyo cabinet dictionary of type
     * Key: A:B
     * Value: distance of the edge
     */
    virtual void to_sparse_graph_tokyo_cabinet_dictionary(const string& output,uint64_t mmap_size) = 0;

    /*
     * Returns the distance between two pages.  It returns 255 in the event of no distance.
     */
    virtual uint32_t distance_between(const string& page1, const string& page2) = 0;
    virtual void to_link_counts(const string& index_root) = 0;

    /* We can save the state of the graph to a dump file */
    virtual void core_dump(const string& dump_file) = 0;

    /* Output a subgraph index that just has the outlinks,inlinks and the
     * outlinks of the inlinks of each of the tuples the list of the tuples
     */
    virtual void to_subgraph(const string& index_root,vector<string>& tuples) = 0;

    virtual size_t size() = 0;

    virtual void clear() = 0;

    virtual ~Manager() {}

    class Iterator;
    virtual shared_ptr<Iterator> iterator() = 0;

  protected:
    static WikipediaPage::Manager* instance_;
    Manager() {}
};

class WikipediaPage::Manager::Iterator
{
  public:
    virtual WikipediaPage* next() = 0;
    virtual ~Iterator() {}

  protected:
    Iterator() {}
};


#endif
