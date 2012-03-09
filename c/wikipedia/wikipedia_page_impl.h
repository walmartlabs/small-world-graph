#ifndef __WIKIPEDIA_PAGE_IMPL_
#define __WIKIPEDIA_PAGE_IMPL_

#include "wikipedia_page.h"
#include <google/sparse_hash_map>
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include <google/sparse_hash_set>
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"
#include "equality.h"
#include "wiki_parser.h"

using google::sparse_hash_map;
using google::sparse_hash_set;
using google::dense_hash_map;
using google::dense_hash_set;

class WikipediaPageImpl;

typedef enum { WIKIPEDIA } edge_source_t;
typedef enum { GENERAL } edge_type_t;

typedef struct {
  edge_source_t source;
  edge_type_t type;
  char* snippet;
} edge_descriptor_t;

typedef struct {
  WikipediaPageImpl* first;
  WikipediaPageImpl* second;
  unsigned char distance;
  uint32_t common_count;
  bool bidi;
  edge_descriptor_t* descriptors;
} page_relationship_t;

typedef struct {
  WikipediaPageImpl* end;
  size_t count;
  bool in_text;
} one_way_edge_t;

struct WikipediaEdgeInfoImpl : public WikipediaEdgeInfo {
};

typedef vector<page_relationship_t> PageRelationshipList;

class WikipediaPageImpl : public WikipediaPage 
{
  public:
    WikipediaPageImpl(uint32_t id, const char* title);

    string title() const;

    WikipediaImage* representative_image() const;

    WikipediaPage* canonical() const;
    
    WikipediaRawLinkList* raw_outlinks() const;

    WikipediaEdgeInfoList* outlinks() const;
    WikipediaEdgeInfoList* inlinks() const; 
    size_t len_inlinks() const;

    WikipediaPageList* text_outlinks() const;

    WikipediaImageList* images() const;

    void neighbors(PageRelationshipList& results) const;

    void sorted_edges(PageRelationshipList& result) const;

    /* Simpler edges than the sorted edges that take into account outlinks and bidirectionals mainly */
    void bidi_sorted_edges(PageRelationshipList& result) const;

    void print_sorted_edges() const;
    void print_sorted_edge_statistics() const;
    void print_neighbors(size_t num_neighbors) const;
    void print_bidi_edges() const;

    bool operator<(const WikipediaPageImpl& page) { cout << "Here" << endl; return id_ < page.id_; }

    friend class WikipediaPageManagerImpl;

  protected:
    char* title_;

    link_array_t raw_outlinks_;

    WikipediaEdgeInfoImpl** outlinks_;
    size_t outlinks_len_;

    WikipediaEdgeInfoImpl** inlinks_;
    size_t inlinks_len_;

    WikipediaPageImpl** text_outlinks_;
    size_t text_outlinks_len_;

    WikipediaImage** images_;
    size_t images_len_;

    WikipediaPageImpl* canonical_;

    void add_to_raw_outlinks(const char* name, size_t count, bool in_text);

    void add_to_outlinks(WikipediaEdgeInfoImpl* edge);
    void add_to_inlinks(WikipediaEdgeInfoImpl* edge);

    void add_to_images(WikipediaImage* image);
};

typedef dense_hash_set<const char*,PaulHsiehHash, eqstr> TitleSet;
typedef dense_hash_set<WikipediaPageImpl*, ThomasWangHashPtr, eqptr> PageSet;
typedef dense_hash_map<const char*, WikipediaPageImpl*, PaulHsiehHash, eqstr> TitlePageMap;
typedef sparse_hash_map<char*,char*, ThomasWangHashPtr, eqptr> RedirectMap;
typedef dense_hash_map<const char*, WikipediaEdgeInfoImpl*, PaulHsiehHash, eqstr> TitleEdgeMap;

class WikipediaPageManagerImpl : public WikipediaPage::Manager
{
  public:
    WikipediaPageManagerImpl();
    WikipediaPage* new_page(const string& title, const string& content);
    WikipediaPage* page(const string& title);

    size_t size();

    void clear();

    shared_ptr<Iterator> iterator();

    void load(const string& xmlfilename);
    void load_from_index(const string& index_root);
    void load_from_tokyo_cabinet(const string& tc_file);

    void to_index(const string& index_root);
    void to_bidi_graph(const string& index_root);
    void to_sparse_graph(const string& index_root,uint32_t num_edges);
    void to_distances(const string& index_root,uint32_t num_neighbors);
    void to_subgraph(const string& index_root,vector<string>& tuples);
    void to_priority_topic_list(const string& index_root);
    void to_link_counts(const string& index_root);

    void to_sparse_graph_tokyo_cabinet_dictionary(const string& output,uint64_t mmap_size);

    uint32_t distance_between(const string& page1, const string& page2);

    void core_dump(const string& dump_file);

    WikipediaPageImpl* perf_new_page(const char* title, const char* content);

    static bool compare_inlink_count(WikipediaPageImpl* a, WikipediaPageImpl* b);

    friend class WikipediaPageIteratorImpl;

  protected:

    void new_alias(const char* alias_title, const char* real_title);

    const char* pool_title(const char* title);
    WikipediaPageImpl* build_or_retrieve_page(const char* title);

    void resolve_redirects();
    void resolve_outlinks();

    uint32_t auto_increment_id_;
    PageSet pages_;
    TitlePageMap title_page_map_;
    TitleSet titles_;
    RedirectMap redirects_;
};

class WikipediaPageIteratorImpl : public WikipediaPage::Manager::Iterator 
{
  public:
    WikipediaPageIteratorImpl(WikipediaPageManagerImpl*);
    WikipediaPage* next();

  protected:
    PageSet::const_iterator it_;
    PageSet::const_iterator end_;
};

#endif
