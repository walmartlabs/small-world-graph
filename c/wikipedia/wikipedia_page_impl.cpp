#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <sstream>
#include <fstream>
#include <errno.h>
#include "string_utils.h"
#include <assert.h>
#include <tchdb.h>
#include <pillowtalk.h>

#include <expat.h>
#include "wikipedia_page_impl.h"
#include "wiki_scanner.h"
#include "wiki_parser.h"
#include <sys/queue.h>
#include <google/dense_hash_set>
#include <google/dense_hash_map>
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"
#include <algorithm>
#include <vector>
#include <list>
#include "wikipedia_types.h"
#include "wikipedia_xml_parser.h"
#include "wiki_title_validator.h"
#include "benchmark.h"

using google::dense_hash_set;
using google::dense_hash_map;

using namespace std;

/* Typedefs */

typedef dense_hash_map<WikipediaPageImpl*,int,ThomasWangHashPtr,eqptr> PageIntHash;
typedef dense_hash_set<WikipediaPageImpl*,ThomasWangHashPtr,eqptr> DensePageSet;
typedef dense_hash_set<WikipediaEdgeInfoImpl*,ThomasWangHashPtr,eqptr> DenseEdgeSet;

/* Prototypes */
static void save_xml_progress_to_page(char* title,char* content, void* traveler);
static bool compare_distances(page_relationship_t& first, page_relationship_t& second);
static inline uint32_t distance_calculation(uint32_t inlinks_to_page1, uint32_t inlinks_to_page2, uint32_t common_count, bool bidirectional);

/* Globals */
static size_t __pages_imported = 0;
static WikipediaPageManagerImpl* __page_manager_instance;

/* Implementation */
ostream& operator<<(ostream& os, const WikipediaEdgeInfo& edge)
{
  os << "('" << edge.begin->title() << "','" << edge.end->title() << "'," << edge.count << "," << edge.in_text << ")";
  return os;
}

WikipediaPageImpl::WikipediaPageImpl(uint32_t id,const char* title)
{
  id_ = id;
  title_ = (char*) title;
  ambiguous_ = false;

  raw_outlinks_.links = NULL;
  raw_outlinks_.len = 0;

  outlinks_ = NULL;
  outlinks_len_ = 0;

  inlinks_ = NULL;
  inlinks_len_ = 0;

  canonical_ = NULL;
  images_= NULL;
  images_len_ = 0;

  entity_ = true;

  int nwords = 0;
  string w;
  istringstream iss(title_, istringstream::in);

  while( iss >> w ) {
    if (w.at(0) < 65 || w.at(0) > 90) {
      if ((w.at(0) >= 97 && w.at(0) <= 122) && (w.compare("of") && w.compare("in") && w.compare("the") && w.compare("with") && w.compare("for") && w.compare("or") && w.compare("and"))) { //none of those words
        entity_ = false;
        nwords = -1;
        break;
      }
    }
    else if (w.compare("List") == 0) {
      entity_ = false;
      nwords = -1;
      break;
    }
    nwords++;
  }
  if (entity_ && nwords > 5)
    entity_ = false;
  else if (!entity_ && nwords > 0 && nwords < 3) 
    entity_ = true;

}

string
WikipediaPageImpl::title() const
{
  return string(title_);
}

WikipediaPage* 
WikipediaPageImpl::canonical() const
{
  if (canonical_)
    return canonical_;
  else
    return (WikipediaPageImpl*) this;
}

void 
WikipediaPageImpl::add_to_raw_outlinks(const char* name, size_t count, bool in_text)
{
  raw_outlinks_.links = (link_t**) realloc(raw_outlinks_.links,sizeof(link_t*) * (raw_outlinks_.len + 1));
  link_t* new_link = (link_t*) malloc(sizeof(link_t));
  new_link->name = (char*) name;
  new_link->count = count;
  new_link->in_text = in_text;
  raw_outlinks_.links[raw_outlinks_.len] = new_link;
  raw_outlinks_.len++;
}

void 
WikipediaPageImpl::add_to_outlinks(WikipediaEdgeInfoImpl* edge)
{
  outlinks_ = (WikipediaEdgeInfoImpl**) realloc(outlinks_,sizeof(WikipediaEdgeInfoImpl*) * (outlinks_len_+ 1));
  outlinks_[outlinks_len_] = edge;
  outlinks_len_++;
}

void 
WikipediaPageImpl::add_to_inlinks(WikipediaEdgeInfoImpl* edge)
{
  inlinks_ = (WikipediaEdgeInfoImpl**) realloc(inlinks_,sizeof(WikipediaEdgeInfoImpl*) * (inlinks_len_+ 1));
  inlinks_[inlinks_len_] = edge;
  inlinks_len_++;
}

void 
WikipediaPageImpl::add_to_images(WikipediaImage* image)
{
  images_ = (WikipediaImage**) realloc(images_,sizeof(WikipediaImage*) * (images_len_ + 1));
  images_[images_len_] = image;
  images_len_++;
}

WikipediaRawLinkList* 
WikipediaPageImpl::raw_outlinks() const
{
  WikipediaRawLinkList* list = new WikipediaRawLinkList();
  for(size_t i=0; i < raw_outlinks_.len; i++) {
    WikipediaRawLink link;
    link.name = raw_outlinks_.links[i]->name;
    link.in_text = raw_outlinks_.links[i]->in_text;
    link.count = raw_outlinks_.links[i]->count;
    list->push_back(link);
  }
  return list;
}

WikipediaEdgeInfoList*
WikipediaPageImpl::outlinks() const
{
  WikipediaEdgeInfoList* list = new WikipediaEdgeInfoList();
  for(size_t i=0; i < outlinks_len_; i++) {
    WikipediaEdgeInfo edge;
    edge.begin = outlinks_[i]->begin;
    edge.end = outlinks_[i]->end;
    edge.count = outlinks_[i]->count;
    edge.in_text = outlinks_[i]->in_text;
    list->push_back(edge);
  }
  return list;
}

size_t
WikipediaPageImpl::len_inlinks() const
{
  return inlinks_len_;
}

WikipediaEdgeInfoList*
WikipediaPageImpl::inlinks() const
{
  WikipediaEdgeInfoList* list = new WikipediaEdgeInfoList();
  for(size_t i=0; i < inlinks_len_; i++) {
    WikipediaEdgeInfo edge;
    edge.begin = inlinks_[i]->begin;
    edge.end = inlinks_[i]->end;
    edge.count = inlinks_[i]->count;
    edge.in_text = inlinks_[i]->in_text;
    list->push_back(edge);
  }
  return list;
}

WikipediaImageList* 
WikipediaPageImpl::images() const
{
  WikipediaImageList* list = new WikipediaImageList();
  for(size_t i=0; i < images_len_; i++) {
    list->push_back(images_[i]);
  }
  return list;
}

WikipediaImage* 
WikipediaPageImpl::representative_image() const
{
  if (images_)
    return images_[0];
  else
    return NULL;
}

/* Calculate the neighbors
 *
 * let b mean outlink pages from this
 * let c mean inlink pages from this
 * let d mean outlinks from the inlink page
 */
void 
WikipediaPageImpl::neighbors(PageRelationshipList& results) const
{
  page_relationship_t rel;
  PageRelationshipList relations;
  PageIntHash common_count;
  DensePageSet symmetry_existence;
  common_count.set_empty_key(NULL);
  symmetry_existence.set_empty_key(NULL);

  for(size_t i = 0; i < outlinks_len_; i++) {
    WikipediaPageImpl* page_b = (WikipediaPageImpl *)outlinks_[i]->end;
    if (!page_b->ambiguous_) {
      PageIntHash::const_iterator res = common_count.find(page_b);
      common_count[page_b] = 1;
      symmetry_existence.insert(page_b);
    }
  }

  DensePageSet::const_iterator sym_end = symmetry_existence.end();
  for (size_t i = 0; i < inlinks_len_; i++) { // inlinks to Circle
    WikipediaPageImpl* page_c = (WikipediaPageImpl*) inlinks_[i]->begin;
    if (!page_c->ambiguous_) {
      DensePageSet::const_iterator sym = symmetry_existence.find(page_c);
      if (sym != sym_end) {
        int boost_common_count = (int) max(4.0,(page_c->inlinks_len_/4.0)+0.5);
        common_count[page_c] = boost_common_count;
        //cout << "Setting Common Count to " << boost_common_count << " for " << page_c->title_ << endl;
      }
      for (size_t j = 0; j < page_c->outlinks_len_; j++) { // outlinks of inlink to Circle
        WikipediaEdgeInfoImpl* c_d = page_c->outlinks_[j];
        WikipediaPageImpl* page_d = (WikipediaPageImpl*) c_d->end;
        if (page_d == this) 
          continue;
        PageIntHash::const_iterator res = common_count.find(page_d);
        if (res != common_count.end()) {
          int old_count = res->second;
          common_count[page_d] = old_count + c_d->count;
        } else {
          common_count[page_d] = c_d->count;
        }
      }
    }
  }

  // Here we only add pages that are both entities and non ambiguous as potential neighbors
  for(PageIntHash::const_iterator jj = common_count.begin(); jj != common_count.end(); jj++) {
    rel.first = (WikipediaPageImpl*) this;
    WikipediaPageImpl* neighbor = jj->first;
    if (neighbor->entity_ && !neighbor->ambiguous_) {
      rel.second = neighbor;
      double inlink_quotient = min(1.0,((double) (jj->second) / (inlinks_len_ + 1)));
      rel.distance = (unsigned char)(256 * (1.0 - inlink_quotient));
      //cout << "(Title,Common Count,Quotient) -> ('" << neighbor->title_ << "'," << (int) jj->second << "," << (double) inlink_quotient << ")" << endl;
      relations.push_back(rel);
    }
  }

  if (results.size() > relations.size())
    results.resize(relations.size());
  partial_sort_copy(relations.begin(),relations.end(),results.begin(),results.end(),compare_distances);
}

/* 
 * Sort the edges of a page with reference to bidirectionals and inlink count
 * 1. First find all the bidirectional links and give those a link weight of 1
 * 2. Then find all the non-bidirectional links and give them a link weight of their # of inlinks
 *  - This is done to prioritize edges to lesser known topics.
 */ 
void 
WikipediaPageImpl::sorted_edges(PageRelationshipList& results) const
{
  PageRelationshipList edges;

  // This set is only connected edges to page
  DensePageSet connected_edges;
  DensePageSet symmetry_existence;
  DensePageSet bidirectional;
  PageIntHash common_count;
  DensePageSet::const_iterator res;

  bidirectional.set_empty_key(NULL);
  common_count.set_empty_key(NULL);
  symmetry_existence.set_empty_key(NULL);
  connected_edges.set_empty_key(NULL);

  for(size_t i = 0; i < outlinks_len_; i++) {
    WikipediaPageImpl* end = (WikipediaPageImpl*) outlinks_[i]->end;
    connected_edges.insert(end);
    symmetry_existence.insert(end);
    common_count[end] = 1;
  }

  for (size_t i = 0; i < inlinks_len_; i++) { // inlinks to Circle
    WikipediaPageImpl* inlink_page = (WikipediaPageImpl*) inlinks_[i]->begin;
    connected_edges.insert(inlink_page);
    common_count[inlink_page] = 1;
  }

  DensePageSet::const_iterator sym_end = symmetry_existence.end();
  DensePageSet::const_iterator connected_end = connected_edges.end();
  DensePageSet::const_iterator connected;
  //DensePageSet::const_iterator bidirectional_exist;
  for (size_t i = 0; i < inlinks_len_; i++) { // inlinks to Circle
    WikipediaPageImpl* inlink_page = (WikipediaPageImpl*) inlinks_[i]->begin;
    DensePageSet::const_iterator sym = symmetry_existence.find(inlink_page);
    if (sym != sym_end) {
      bidirectional.insert(inlink_page);
    } 
    for (size_t j = 0; j < inlink_page->outlinks_len_; j++) { // outlinks of inlink to Circle
      WikipediaPageImpl* page_b = (WikipediaPageImpl*) inlink_page->outlinks_[j]->end;
      connected = connected_edges.find(page_b);
      if (connected != connected_end) {
        PageIntHash::const_iterator res = common_count.find(page_b);
        if (res != common_count.end()) {
          common_count[page_b]++;
        } else {
          common_count[page_b] = 1;
        }
      }
    }
  }

  DensePageSet::const_iterator bidirectional_end = bidirectional.end();
  for(PageIntHash::const_iterator jj = common_count.begin(); jj != common_count.end(); jj++) {
    bool bidi_found = false;
    WikipediaPageImpl* end = jj->first;

    DensePageSet::const_iterator bidirectional_exist = bidirectional.find(end);
    if (bidirectional_exist != bidirectional_end)
      bidi_found = true;

    page_relationship_t rel;
    rel.first = (WikipediaPageImpl*) this;
    rel.second = end;
    int count = jj->second;
    rel.distance = distance_calculation(inlinks_len_,end->inlinks_len_,count,bidi_found);
    rel.common_count = count;
    rel.bidi = bidi_found;
    edges.push_back(rel);
  }


  results.resize(edges.size());
  partial_sort_copy(edges.begin(),edges.end(),results.begin(),results.end(),compare_distances);
}

void 
WikipediaPageImpl::bidi_sorted_edges(PageRelationshipList& results) const
{
  PageRelationshipList edges;
  DensePageSet bidirectional;

  bidirectional.set_empty_key(NULL);

  for(size_t i = 0; i < inlinks_len_; i++) {
    bidirectional.insert((WikipediaPageImpl*) inlinks_[i]->begin);
  }

  for(size_t i = 0; i < outlinks_len_; i++) {
    WikipediaEdgeInfoImpl* edge = outlinks_[i];
    WikipediaPageImpl* end = (WikipediaPageImpl*) edge->end;
    if (end->ambiguous_ || !edge->in_text || !end->entity_)
      continue;

    DensePageSet::const_iterator is_bidi = bidirectional.find(end);
    page_relationship_t rel;
    rel.first = (WikipediaPageImpl*) this;
    rel.second = end;
    if (is_bidi != bidirectional.end()) {
      rel.distance = 1;
    } else {
      if (edge->count > 2) {
        rel.distance = 2;
      } else if (edge-> count > 1) {
        rel.distance = 3;
      } else {
        rel.distance = 4;
      }
    }
    edges.push_back(rel);
  }

  results.resize(edges.size());
  partial_sort_copy(edges.begin(),edges.end(),results.begin(),results.end(),compare_distances);
}


void 
WikipediaPageImpl::print_sorted_edges() const
{
  PageRelationshipList results;
  sorted_edges(results);
  printf("%u Sorted Edges\n", (unsigned int) results.size());
  for(PageRelationshipList::const_iterator ii = results.begin(); ii != results.end(); ii++) {
    page_relationship_t rel = *ii;
    cout << "('" << rel.second->title_ << "'," << (int) rel.distance << ")" << endl;
  }
}

void
WikipediaPageImpl::print_sorted_edge_statistics() const
{
  PageRelationshipList results;
  sorted_edges(results);
  for(PageRelationshipList::const_iterator ii = results.begin(); ii != results.end(); ii++) {
    page_relationship_t rel = *ii;
    WikipediaPageImpl* end_point = rel.second;
    cout << title_ << "\t" 
         << inlinks_len_ << "\t"
         << outlinks_len_ << "\t"
         << end_point->title_ << "\t" 
         << end_point->inlinks_len_ << "\t" 
         << end_point->outlinks_len_ << "\t" 
         << rel.common_count << "\t"
         << ((rel.bidi) ? "T" : "F") << "\t"
         << (int) rel.distance
         << endl;
  }
}

void 
WikipediaPageImpl::print_neighbors(size_t num_neighbors) const
{
  PageRelationshipList results(num_neighbors);
  neighbors(results);
  for(PageRelationshipList::const_iterator ii = results.begin(); ii != results.end(); ii++) {
    page_relationship_t rel = *ii;
    cout << "('" << rel.second->title_ << "'," << (int) rel.distance << ")" << endl;
  }
}

void 
WikipediaPageImpl::print_bidi_edges() const
{
  PageRelationshipList results;
  bidi_sorted_edges(results);
  for(PageRelationshipList::const_iterator ii = results.begin(); ii != results.end(); ii++) {
    page_relationship_t rel = *ii;
    cout << "('" << rel.second->title_ << "'," << (int) rel.distance << ")" << endl;
  }
}


/* Manager Public Implementation */
WikipediaPageManagerImpl::WikipediaPageManagerImpl() { 
  auto_increment_id_ = 1; 
  pages_.set_empty_key(NULL);
  title_page_map_.set_empty_key(NULL);
  titles_.set_empty_key(NULL);
}

WikipediaPage* WikipediaPageManagerImpl::new_page(const string& title, const string& content)
{
  return perf_new_page(title.c_str(),content.c_str());
}

WikipediaPage* 
WikipediaPageManagerImpl::page(const string& title)
{
  TitlePageMap::const_iterator res = title_page_map_.find((char*)title.c_str());
  if (res != title_page_map_.end())
    return (WikipediaPageImpl* ) res->second;
  else 
    return NULL;
}

size_t 
WikipediaPageManagerImpl::size()
{
  return pages_.size();
}

void 
WikipediaPageManagerImpl::clear()
{
  for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
    WikipediaPageImpl* page = *ii;
    delete page;
  }
  pages_.clear();

  for(TitleSet::const_iterator ii = titles_.begin(); ii != titles_.end(); ii++) {
    char* title = (char*) *ii;
    free(title);
  }

  titles_.clear();
  title_page_map_.clear();
}

void 
WikipediaPageManagerImpl::to_index(const string& index_root)
{
  cout << "\nWriting Image Index to " << index_root + ".images" << endl;
  ofstream image_index((index_root + ".images").c_str());
  if (image_index.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      WikipediaImage* image = page->representative_image();
      if (image) {
        image_index << page->title_ << "|" << image->link() << endl;
      }
    }
    image_index.close();
  } else {
    cout << "Couldn't Open Image Index File" << endl;
  }

  cout << "Writing Page Index to " << index_root + ".pages" << endl;
  ofstream page_index((index_root + ".pages").c_str());
  if (page_index.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      page_index << page->id_ << "|" << page->title_ << "|" << (int) page->ambiguous_ << endl;
    }
    page_index.close();
  } else {
    cout << "Couldn't Open Page Index File" << endl;
  }

  cout << "Writing Redirect Index to " << index_root + ".redirects" << endl;
  ofstream redirect_index((index_root + ".redirects").c_str());
  if (redirect_index.is_open()) {
    for(TitlePageMap::const_iterator ii = title_page_map_.begin(); ii != title_page_map_.end(); ii++) {
      const char* title_pointer = ii->first;
      WikipediaPageImpl* page = ii->second;
      if (strcmp(title_pointer,page->title_))
        redirect_index << title_pointer << ">" << page->id_ << endl;
    }
    redirect_index.close();
  } else {
    cout << "Couldn't Open Redirect Index File" << endl;
  }

  cout << "Writing Outlinks Index to " << index_root + ".outlinks" << endl;
  ofstream outlinks_index((index_root + ".outlinks").c_str());
  if (outlinks_index.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      DensePageSet dups;
      dups.set_empty_key(NULL);
      for(size_t i=0; i < page->outlinks_len_; i++) {
        WikipediaEdgeInfoImpl* edge = page->outlinks_[i];
        WikipediaPageImpl* outlink_page = (WikipediaPageImpl*) edge->end;
        DensePageSet::const_iterator res = dups.find(outlink_page);
        if (res == dups.end()) {
          serialized_outlink_t link;
          link.first_id = page->id_;
          link.second_id = outlink_page->id_;
          link.in_text = edge->in_text;
          link.count = edge->count;
          outlinks_index.write((char*) &link,sizeof(serialized_outlink_t));
          dups.insert(outlink_page);
        }
      }
    }
    outlinks_index.close();
  } else {
    cout << "Couldn't Open Outlinks Index File" << endl;
  }
}

void 
WikipediaPageManagerImpl::to_sparse_graph(const string& index_root,uint32_t num_edges)
{
  bench_start("to_sparse_graph");
  cout << "Writing Graph Edges Index to " << index_root + ".sparse_graph" << endl;
  int count = 0;
  int total = pages_.size();

  ofstream graph_edges_index((index_root + ".sparse_graph").c_str());
  if (graph_edges_index.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      PageRelationshipList results;
      page->sorted_edges(results);
      //cout << page->title_ << " -> " << results.size() << " edges" << endl;
      for(PageRelationshipList::const_iterator jj = results.begin(); jj != results.end(); jj++) {
        serialized_relationship_t rel;
        if (jj->first) {
          rel.first_id = jj->first->id_;
          rel.second_id = jj->second->id_;
          rel.distance = jj->distance;
          graph_edges_index.write((char*) &rel,sizeof(serialized_relationship_t));
        }
      }
      count++;
      if (count % 100 == 0) {
        fprintf(stdout,"\r%9d / %d",(int) count,(int) total);
        fflush(stdout);
      }
    }
    fprintf(stdout,"\r%9d / %d",(int) count,(int) total);
    fflush(stdout);
    graph_edges_index.close();
    cout << endl;
  } else {
    cout << "Couldn't Open Graph Edges Index File" << endl;
  }
  bench_finish("to_sparse_graph");
}

void

WikipediaPageManagerImpl::to_bidi_graph(const string& index_root)
{
  bench_start("to_bidi_graph");
  cout << "Writing Bidi Graph Index to " << index_root + ".bidi" << endl;
  int count = 0;
  int total = pages_.size();

  ofstream bidi((index_root + ".bidi").c_str());
  if (bidi.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      PageRelationshipList results;
      page->bidi_sorted_edges(results);
      //cout << page->title_ << " -> " << results.size() << " edges" << endl;
      for(PageRelationshipList::const_iterator jj = results.begin(); jj != results.end(); jj++) {
        serialized_relationship_t rel;
        if (jj->first) {
          rel.first_id = jj->first->id_;
          rel.second_id = jj->second->id_;
          rel.distance = jj->distance;
          bidi.write((char*) &rel,sizeof(serialized_relationship_t));
        }
      }
      count++;
      if (count % 100 == 0) {
        fprintf(stdout,"\r%9d / %d",(int) count,(int) total);
        fflush(stdout);
      }
    }
    fprintf(stdout,"\r%9d / %d",(int) count,(int) total);
    fflush(stdout);
    bidi.close();
    cout << endl;
  } else {
    cout << "Couldn't Open Bidi Index File" << endl;
  }
  bench_finish("to_bidi");
}

void 
WikipediaPageManagerImpl::to_subgraph(const string& index_root,vector<string>& tuples)
{
  DensePageSet unique_pages;
  DenseEdgeSet unique_edges;
  DensePageSet::const_iterator uniq;

  unique_pages.set_empty_key(NULL);
  unique_edges.set_empty_key(NULL);

  ofstream pages_index((index_root + ".pages").c_str());
  ofstream outlinks_index((index_root + ".outlinks").c_str());
  for(vector<string>::const_iterator ii = tuples.begin(); ii != tuples.end(); ii++) {
    const char* title = (*ii).c_str();
    TitlePageMap::const_iterator finder = title_page_map_.find(title);
    if (finder == title_page_map_.end()) 
      continue;
    WikipediaPageImpl* page = finder->second;
    if (page) {
      unique_pages.insert(page);

      // First grab all the outlinks and their inlinks
      for(size_t i=0; i < page->outlinks_len_; i++) {
        WikipediaEdgeInfoImpl* edge = page->outlinks_[i];
        WikipediaPageImpl* end = (WikipediaPageImpl*) edge->end;

        cout << "Adding Outlink: " << (*edge) << endl;
        unique_pages.insert(end);
        unique_edges.insert(edge);

        for(size_t j=0; j < end->inlinks_len_; j++) {
          cout << "Adding Inlink to Outlink: " << *(end->inlinks_[j]) << endl;
          unique_pages.insert((WikipediaPageImpl*) end->inlinks_[j]->begin);
          unique_edges.insert(end->inlinks_[j]);
        }
      }

      for(size_t i=0; i < page->inlinks_len_; i++) {
        WikipediaEdgeInfoImpl* inlink= page->inlinks_[i];
        WikipediaPageImpl* start = (WikipediaPageImpl*) inlink->begin;
        cout << "Adding Inlink: " << (*inlink) << " " << i << "/" << page->inlinks_len_ << endl;
        unique_pages.insert(start);
        unique_edges.insert(inlink);

        for(size_t j=0; j < start->outlinks_len_; j++) {
          WikipediaEdgeInfoImpl* outlink= start->outlinks_[j];
          WikipediaPageImpl* end = (WikipediaPageImpl*) outlink->end;
          unique_pages.insert(end);
          unique_edges.insert(outlink);
          //cout << "Adding Outlink from Inlink: " << (*outlink) << endl;

          for(size_t k=0; k < end->inlinks_len_; k++) {
            //cout << "Adding Inlink to Outlink from Inlink: " << *(end->inlinks_[k]) << endl;
            unique_pages.insert((WikipediaPageImpl*) end->inlinks_[k]->begin);
            unique_edges.insert(end->inlinks_[k]);
          }
        }

        for(size_t j=0; j < start->inlinks_len_; j++) {
          WikipediaEdgeInfoImpl* i_inlink =  start->inlinks_[j];
          WikipediaPageImpl* i_start = (WikipediaPageImpl*) i_inlink->begin;
          //cout << "Adding Inlink to Inlink: " << (*i_inlink) << endl;

          unique_pages.insert(i_start);
          unique_edges.insert(i_inlink);
        }
      }
    }
  }


  for(DensePageSet::const_iterator jj = unique_pages.begin(); jj != unique_pages.end(); jj++) {
    WikipediaPageImpl* page = *jj;
    pages_index << page->id_ << "|" << page->title_ << "|" << (int) page->ambiguous_ << endl;
  }

  for(DenseEdgeSet::const_iterator jj = unique_edges.begin(); jj != unique_edges.end(); jj++) {
    WikipediaEdgeInfoImpl* edge = *jj;
    serialized_outlink_t link;
    link.first_id = ((WikipediaPageImpl*)edge->begin)->id_;
    link.second_id = ((WikipediaPageImpl*)edge->end)->id_;
    link.in_text = edge->in_text;
    link.count = edge->count;
    outlinks_index.write((char*) &link,sizeof(serialized_outlink_t));
  }
  pages_index.close();
  outlinks_index.close();
}

bool WikipediaPageManagerImpl::compare_inlink_count(WikipediaPageImpl* a, WikipediaPageImpl* b) 
{
  return a->inlinks_len_ > b->inlinks_len_;
}

void WikipediaPageManagerImpl::to_priority_topic_list(const string& index_root)
{
  bench_start("To Priority Topic List");
  list<WikipediaPageImpl*> sorted_pages;


  for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
    WikipediaPageImpl* page = *ii;
    if (!page->ambiguous_ && page->entity_) 
      sorted_pages.push_back(page);
  }

  sorted_pages.sort(WikipediaPageManagerImpl::compare_inlink_count);
  ofstream topic_list((index_root + ".topics").c_str());
  if (topic_list.is_open()) {
    for(list<WikipediaPageImpl*>::const_iterator ii = sorted_pages.begin(); ii != sorted_pages.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      topic_list << page->title_ << endl;
    }
  }
  bench_finish("To Priority Topic List");
}


void 
WikipediaPageManagerImpl::to_distances(const string& index_root,uint32_t num_neighbors)
{
  int count = 0;
  int total = pages_.size();
  cout << "Writing Distance Index to " << index_root + ".distances" << endl;
  ofstream distance_index((index_root + ".distances").c_str());
  if (distance_index.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      PageRelationshipList results(num_neighbors);
      page->neighbors(results);
      for(PageRelationshipList::const_iterator jj = results.begin(); jj != results.end(); jj++) {
        serialized_relationship_t rel;
        if (jj->first) {
          rel.first_id = jj->first->id_;
          rel.second_id = jj->second->id_;
          rel.distance = jj->distance;
          distance_index.write((char*) &rel,sizeof(serialized_relationship_t));
        }
      }
      count++;
      if (count % 100 == 0) {
        fprintf(stdout,"\r%9d / %d",count,total);
        fflush(stdout);
      }
    }
    fprintf(stdout,"\r%9d / %d",count,total);
    fflush(stdout);
    distance_index.close(); 
  } else {
    cout << "Couldn't Open Distance Index File" << endl;
  }
}

void
WikipediaPageManagerImpl::to_link_counts(const string& index_root)
{
  cout << "Writing Link Counts to " << index_root + ".link_counts" << endl;
  ofstream link_counts((index_root + ".link_counts").c_str());
  for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
    WikipediaPageImpl* page = *ii;
    link_counts << page->title_ << "|" << page->inlinks_len_ << endl;
  }
}


void 
WikipediaPageManagerImpl::load(const string& xmlfilename)
{
  wikipedia_xml_parse(xmlfilename.c_str(),save_xml_progress_to_page, NULL);
  fprintf(stdout,"\rPages Imported: %9llu",(unsigned long long)__pages_imported);
  fflush(stdout);
  resolve_redirects();
  resolve_outlinks();
}

void 
WikipediaPageManagerImpl::load_from_index(const string& index_root)
{
  bench_start("Load");
  FILE* file = NULL;
  char buffer[4096];
  string line;

  bench_start("Load Pages");
  title_page_map_.resize(10000000); // optimization
  pages_.resize(2800000);
  titles_.resize(10000000);

  WikipediaPageImpl** id_page_table = NULL;
  size_t biggest_id_so_far = 0;
  string index_filename = string(index_root) + ".pages";
  int page_count = 0;
  if ((file = fopen((char*)index_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,file)) {
      char* first = NULL;
      char* second = NULL;
      char* third = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '|') {
          buffer[i] = '\0';
          first = buffer;
          second = buffer + i + 1;
          for(int j=i; j < length; j++) {
            if (buffer[j] == '|') {
              buffer[j] = '\0';
              third = buffer + j + 1;
              buffer[length-1] = '\0';
              break;
            }
          }

          if (!first || !second || !third)
            break;

          //printf("%s|%s|%s\n",first,second,third);
          int id = atoi(first);
          const char* pooled_key = pool_title(second);
          WikipediaPageImpl* page = new WikipediaPageImpl(id,pooled_key);
          page->ambiguous_ = atoi(third);
          pages_.insert(page);
          title_page_map_[pooled_key] = page;
          page_count++;
          if (id > (int) biggest_id_so_far) {
            id_page_table = (WikipediaPageImpl**) realloc(id_page_table,sizeof(WikipediaPageImpl*) * (id + 1));
            for(int k = biggest_id_so_far + 1; k < id + 1; k++) {
              id_page_table[k] = NULL;
            }
            biggest_id_so_far = id;
          }
          id_page_table[id] = page;
          __pages_imported++;
          if (__pages_imported % 100 == 0) {
            fprintf(stdout,"\rPages Imported: %9llu",(unsigned long long) __pages_imported);
            fflush(stdout);
          }
          break;
        }
      }
    }
    fclose(file);
  } else {
    cout << "Page File Open Error" << endl;
  }

  fprintf(stdout,"\rPages Imported: %9llu\n",(unsigned long long) __pages_imported);
  fflush(stdout);
  bench_finish("Load Pages");

  bench_start("Load Redirects");
  string redirects_filename = string(index_root) + ".redirects";
  FILE* redirects_file = NULL;
  if ((redirects_file = fopen((char*)redirects_filename.c_str(),"r"))) {
    while (fgets(buffer,4096,redirects_file)) {
      char* first = NULL;
      char* second = NULL;
      int length = strlen(buffer);

      for(int i = 0; i < length; i++) {
        if (buffer[i] == '>') {
          buffer[i] = '\0';
          first = buffer;
          buffer[length-1] = '\0';
          second = buffer + i + 1;
          if (!first || !second)
            break;

          int id = atoi(second);
          WikipediaPageImpl* page = id_page_table[id];
          if (page) {
            const char* pooled_key = pool_title(first);
            title_page_map_[pooled_key] = page;
          }
          break;
        }
      }
    }
    fclose(redirects_file);
  } else {
    cout << "Redirects File Open Error" << endl;
  }
  bench_finish("Load Redirects");

#define BIG_BUF_SIZE 10000000
  bench_start("Load Outlinks");
  string outlink_filename = string(index_root) + ".outlinks";
  FILE* outlink_file = NULL;
  if ((outlink_file = fopen((char*)outlink_filename.c_str(),"rb"))) {
    serialized_outlink_t* outlink;
    char* large_buf = (char*) calloc(BIG_BUF_SIZE,sizeof(serialized_outlink_t));
    while(!feof(outlink_file)) {
      WikipediaPageImpl* first = NULL, *second = NULL;
      int elems_read = fread(large_buf,sizeof(serialized_outlink_t),BIG_BUF_SIZE,outlink_file);
      for(int i=0; i < elems_read; i++) {
        outlink = (serialized_outlink_t*) ((char*)large_buf + (i * sizeof(serialized_outlink_t)));
        first = id_page_table[outlink->first_id];
        second = id_page_table[outlink->second_id];
        WikipediaEdgeInfoImpl* edge = new WikipediaEdgeInfoImpl();
        edge->begin = first;
        edge->end = second;
        if (outlink->count > 255u)
          edge->count = 255u;
        else
          edge->count = outlink->count;
        edge->in_text = outlink->in_text;
        if (first && second) {
          first->add_to_outlinks(edge);
          second->add_to_inlinks(edge);
        }
      }
    }

    free(large_buf);
    fclose(outlink_file);
  } else {
    cout << "Outlink File Open Error" << endl;
  }
  bench_finish("Load Outlinks");
  free(id_page_table);

  bench_finish("Load");
}

WikipediaPageImpl* 
WikipediaPageManagerImpl::build_or_retrieve_page(const char* title)
{
  TitlePageMap::const_iterator res;
  res = title_page_map_.find(title);
  if (res != title_page_map_.end()) {
    return res->second;
  }
  const char* pooled_title = pool_title(title);
  WikipediaPageImpl* page = new WikipediaPageImpl(auto_increment_id_++,pooled_title);
  pages_.insert(page);
  title_page_map_[page->title_] = page;
  titles_.insert(page->title_);

  page->canonical_ = page;
  return page;
}

uint32_t 
WikipediaPageManagerImpl::distance_between(const string& page1_title, const string& page2_title)
{
  WikipediaPageImpl* page1 = NULL, *page2= NULL;
  TitlePageMap::const_iterator res;
  res = title_page_map_.find(page1_title.c_str());
  if (res != title_page_map_.end()) {
    page1 = res->second;
  }

  res = title_page_map_.find(page2_title.c_str());
  if (res != title_page_map_.end()) {
    page2 = res->second;
  }

  if (!page1 || !page2) {
    return MAX_DISTANCE;
  }

  PageRelationshipList edges;

  // This set is only connected edges to page
  DensePageSet connected_edges;
  DensePageSet symmetry_existence;
  DensePageSet bidirectional;
  uint32_t common_count = 0;

  bool page1_to_page2_found = false;
  bool page2_to_page1_found = false;
  bool bidi_found = false;

  // Search for a bidirectional link here
  for(size_t i = 0; i < page1->outlinks_len_; i++) {
    WikipediaPageImpl* end = (WikipediaPageImpl*) page1->outlinks_[i]->end;
    if (end == page2) {
      page1_to_page2_found = true;
      break;
    }
  }

  for(size_t i = 0; i < page2->outlinks_len_; i++) {
    WikipediaPageImpl* end = (WikipediaPageImpl*) page2->outlinks_[i]->end;
    if (end == page1) {
      page2_to_page1_found = true;
      common_count++;
      break;
    }
  }

  if (page1_to_page2_found && page2_to_page1_found)
    bidi_found = true;

  for(size_t i= 0; i < page1->outlinks_len_; i++) {
    WikipediaPageImpl* end = (WikipediaPageImpl*) page1->outlinks_[i]->end;
    if (end == page2) {
      common_count++;
    }
  }

  for(size_t i = 0; i < page1->inlinks_len_; i++) {
    WikipediaPageImpl* end = (WikipediaPageImpl*) page1->inlinks_[i]->begin;
    for(size_t j = 0; j < end->outlinks_len_; j++) {
      WikipediaPageImpl* outlink_end = (WikipediaPageImpl*) end->outlinks_[j]->end;
      if (outlink_end == page2) {
        common_count++;
      }
    }
  }

  uint32_t distance = distance_calculation(page1->inlinks_len_,page2->inlinks_len_,common_count,bidi_found);
  printf("Edge between %s and %s\n", page1->title_, page2->title_);
  printf("%s Inlinks:%u\n", page1->title_, (unsigned int) page1->inlinks_len_);
  printf("%s Inlinks:%u\n", page2->title_, (unsigned int) page2->inlinks_len_);
  printf("Bidirectional:%s\n", (bidi_found ? "true" : "false"));
  printf("Common Count:%d\n", common_count);
  printf("Computed Distance:%lu\n", (unsigned long) distance);
  return distance;
}

void 
WikipediaPageManagerImpl::to_sparse_graph_tokyo_cabinet_dictionary(const string& output,uint64_t mmap_size)
{
  TCHDB* tchdb;
  int ecode;
  int count = 0;
  int total = pages_.size();

  tchdb = tchdbnew();

  if(!tchdbtune(tchdb,total * 10L,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(tchdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  printf("Giving %llu memory to mmap\n", (unsigned long long) mmap_size);
  if (!tchdbsetxmsiz(tchdb,mmap_size)) {
    ecode = tchdbecode(tchdb);
    fprintf(stderr, "mmap error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if (!tchdbopen(tchdb,output.c_str(), HDBOWRITER|HDBOCREAT|HDBOTRUNC)) {
    ecode = tchdbecode(tchdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  char key_buf[8096];
  char value_buf[8096];
  for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
    //TitlePageMap::const_iterator res = title_page_map_.find("Laozi");
    //WikipediaPageImpl* page = res->second;
    WikipediaPageImpl* page = *ii;
    PageRelationshipList results;
    page->sorted_edges(results);
    //cout << page->title_ << " -> " << results.size() << " edges" << endl;
    for(PageRelationshipList::const_iterator jj = results.begin(); jj != results.end(); jj++) {
      if (jj->first->title_) {
        snprintf(key_buf,8096,"%s|%s",jj->first->title_,jj->second->title_);
        snprintf(value_buf,8096,"%u",(unsigned int) jj->distance);
        if (!tchdbput(tchdb,key_buf,strlen(key_buf),value_buf,strlen(value_buf))) {
          ecode = tchdbecode(tchdb);
          fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
        }
      }
    }
    count++;
    if (count % 100 == 0) {
      fprintf(stdout,"\r%9d / %d",(int) count,(int) total);
      fflush(stdout);
    }
  }
  fprintf(stdout,"\r%9d / %d",(int) count,(int) total);
  fflush(stdout);
  cout << endl;
}

void 
WikipediaPageManagerImpl::load_from_tokyo_cabinet(const string& tc_file)
{
  bench_start("Load");
  title_page_map_.resize(10000000); // optimization
  pages_.resize(2800000);
  titles_.resize(10000000);

  TCHDB* tchdb = tchdbnew();

  if(!tchdbopen(tchdb,tc_file.c_str(),HDBOREADER | HDBONOLCK)) {
    fprintf(stderr,"Unable to find tokyocabinet file\n");
    exit(-1);
  }

  if(!tchdbiterinit(tchdb)) {
    fprintf(stderr,"Unable to init interator\n");
    exit(-1);
  }

  uint64_t size = tchdbrnum(tchdb);
  uint64_t count = 0;
  TCXSTR *key = tcxstrnew();
  TCXSTR *val = tcxstrnew();
  while(tchdbiternext3(tchdb,key,val)) {
    WikipediaPageImpl* page = build_or_retrieve_page((const char*) tcxstrptr(key));
    char* value_str = strdup((const char*)tcxstrptr(val));
    char* outlink_title = value_str;
    int len = strlen(value_str);
    for(int i=0; i < len + 1; i++) {
      if (value_str[i] == '|' || value_str[i] == '\0') {
        value_str[i] = '\0';
        WikipediaPageImpl* outlink_page = build_or_retrieve_page(outlink_title);
        WikipediaEdgeInfoImpl* edge = new WikipediaEdgeInfoImpl();
        edge->begin = page;
        edge->end = outlink_page;
        edge->in_text = true;
        edge->count = 1;
        page->add_to_outlinks(edge);
        outlink_page->add_to_inlinks(edge);
        outlink_title = value_str + i + 1;
      }
    }

    free(value_str);

    count++;
    if (count % 1000 == 0) {
      fprintf(stderr,"\r%9llu / %llu",(unsigned long long)count,(unsigned long long)size);
      fflush(stderr);
    }
  }
  tcxstrdel(key);
  tcxstrdel(val);
  fprintf(stderr,"\r%9llu / %llu\n",(unsigned long long) count ,(unsigned long long) size);

  if (!tchdbclose(tchdb)) {
    fprintf(stderr,"Iterator Closing Error\n");
  }
  tchdbdel(tchdb);
  bench_finish("Load");
}

shared_ptr<WikipediaPage::Manager::Iterator>
WikipediaPageManagerImpl::iterator()
{
  return shared_ptr<WikipediaPage::Manager::Iterator>(new WikipediaPageIteratorImpl((WikipediaPageManagerImpl*)instance_));
}

/* Manager Protected Implementation */

/* Iterate through all the pages and find which of the string outlinks
 * actually point to a real page.  Then update the inlinks to each page
 * accordingly.
 */
void
WikipediaPageManagerImpl::resolve_outlinks()
{
  cout << "\nResolving Outlinks" << endl;
  int resolved_pages = 0;
  for(PageSet::const_iterator ii = pages_.begin(), end = pages_.end(); ii != end; ii++) {
    WikipediaPageImpl* page = *ii;

    TitleEdgeMap title_edges;
    title_edges.set_empty_key(NULL);

    TitleSet dups;
    dups.set_empty_key(NULL);
    // Insert the title of the current page because we don't want self referentials
    dups.insert(page->title_);
    for(size_t i=0; i < page->raw_outlinks_.len; i++) {
      link_t* outlink = page->raw_outlinks_.links[i];
      const char* outlink_title = outlink->name;
      TitlePageMap::const_iterator res = title_page_map_.find((char*)outlink_title);
      if (res != title_page_map_.end()) {
        WikipediaPageImpl* resolved_page = res->second;
        // We found a page in the existing map, so this is a real outlink
        TitleSet::const_iterator dup_it = dups.find(resolved_page->title_);
        if (dup_it == dups.end()) {
          WikipediaPageImpl* outlink_page = res->second;
          WikipediaEdgeInfoImpl* edge = new WikipediaEdgeInfoImpl();
          edge->begin = page;
          edge->end = outlink_page;
          if (outlink->count > 255) 
            edge->count = 255;
          else
            edge->count = outlink->count;
          edge->in_text = outlink->in_text;
          page->add_to_outlinks(edge);
          outlink_page->add_to_inlinks(edge);
          dups.insert(resolved_page->title_);
          title_edges[resolved_page->title_] = edge;
        } else {
          TitleEdgeMap::const_iterator existing_edge_it = title_edges.find(resolved_page->title_);
          if (existing_edge_it != title_edges.end()) {
            WikipediaEdgeInfoImpl* edge = existing_edge_it->second;
            edge->count += outlink->count;
            if (outlink->in_text)
              edge->in_text = true;
          }
        }
      }
      free(page->raw_outlinks_.links[i]);
    }
    free(page->raw_outlinks_.links);
    page->raw_outlinks_.links = NULL;
    page->raw_outlinks_.len = 0;
    resolved_pages++;
    if (resolved_pages % 100 == 0) {
      fprintf(stdout,"\rOutlinks Resolved: %9d",resolved_pages);
      fflush(stdout);
    }
  }
  fprintf(stdout,"\rOutlinks Resolved: %9d",resolved_pages);
  fflush(stdout);
}

/* Iterate through all the redirect mappings.
 * For each redirect mapping (a->b)
 *   Continue looking up the potential canonical (b) until you can't find
 *   anything in the redirect map, leaving you with the real canonical page (B)
 *
 *   Once you have B, then you want to merge the inlinks of a into the inlinks of B
 */
void 
WikipediaPageManagerImpl::resolve_redirects()
{
  cout << "\nResolving Redirects" << endl;
  int redirect_count = 0;
  for(RedirectMap::const_iterator ii = redirects_.begin(),end = redirects_.end(); ii != end; ii++) {
    const char* alias_title = ii->first;
    char* canonical_title = ii->second;
    int loop_prevention_count=0;
    for(;;) {
      RedirectMap::const_iterator res = redirects_.find(canonical_title);
      if (res == end)
        break;
      else
        canonical_title = res->second;
      loop_prevention_count++;
      if (loop_prevention_count == 10) {
        break;
      }
    }
    if (loop_prevention_count == 10)
      continue;

    TitlePageMap::const_iterator canon_it = title_page_map_.find(canonical_title);
    if (canon_it != title_page_map_.end()) {
      WikipediaPageImpl* canonical = canon_it->second;
      title_page_map_[alias_title] = canonical;
      redirect_count++;
    }

    if (redirect_count % 100 == 0) {
      fprintf(stdout,"\rRedirects Resolved: %9d",redirect_count);
      fflush(stdout);
    }
  }
  fprintf(stdout,"\rRedirects Resolved: %9d",redirect_count);
  fflush(stdout);
}

void 
WikipediaPageManagerImpl::new_alias(const char* alias_title, const char* real_title)
{
  const char* pooled_alias = NULL;
  const char* pooled_real = NULL;
  const char* pooled_lowercase_alias = NULL;


  pooled_alias = pool_title(alias_title);
  pooled_real = pool_title(real_title);

  redirects_[(char*) pooled_alias] = (char*) pooled_real;
  if(isupper(pooled_alias[0]) && islower(pooled_alias[1])) {
    char* lowercased_alias_title = strdup(alias_title);
    downcase(lowercased_alias_title);
    pooled_lowercase_alias = pool_title(lowercased_alias_title);
    redirects_[(char*) pooled_lowercase_alias] = (char*) pooled_real;
    free(lowercased_alias_title);
  }
}

inline
const char* 
WikipediaPageManagerImpl::pool_title(const char* title)
{
  TitleSet::const_iterator res = titles_.find(title);
  const char* pooled_title = NULL;
  if (res == titles_.end()) {
    pooled_title = strdup(title);
    titles_.insert(pooled_title);
    return pooled_title;
  } else {
    pooled_title = *res;
    return pooled_title;
  }
}

WikipediaPageImpl* 
WikipediaPageManagerImpl::perf_new_page(const char* title, const char* content)
{
  if (!title || !content)
    return NULL;

  wiki_parse_tree_t* tree = wiki_parse(content);
  if (tree) {
    if (tree->redirect) {
      new_alias(title,tree->redirect);
      free_parse_tree(tree);
      //assert(alias != NULL);
      return NULL;
    } else {
      WikipediaPageImpl* page = NULL;
      const char* pooled_title = pool_title(title);
      page = new WikipediaPageImpl(auto_increment_id_,pooled_title);
      title_page_map_[page->title_] = page;
      //cout << "Inserting " << pooled_title << " into pages" << endl;
      pages_.insert(page);
      titles_.insert(page->title_);

      // Here we add the default downcase redirect to mirror the behavior of wikipedia
      if (isupper(page->title_[0]) && islower(page->title_[1])) {
        char* lowercased_title = strdup(page->title_);
        downcase(lowercased_title);
        if (strcmp(page->title_,lowercased_title)) {
          titles_.insert(lowercased_title);
          title_page_map_[lowercased_title] = page;
        } else {
          free(lowercased_title);
        }
      }

      auto_increment_id_++;
      page->canonical_ = page;
      //cout << "Building " << title << endl;
      for(size_t i = 0; i < tree->outlinks.len; i++) {
        link_t* link = tree->outlinks.links[i];
        const char* pooled_title = pool_title(link->name);
        page->add_to_raw_outlinks(pooled_title,link->count,link->in_text);
      }
      for(size_t i = 0; i < tree->image_links.len; i++) {
        WikipediaImage* image = WikipediaImage::Manager::instance()->new_image(tree->image_links.links[i]->name);
        page->add_to_images(image);
      }

      if (tree->ambiguous) {
        page->ambiguous_ = true;
      }

      free_parse_tree(tree);

      __pages_imported++;
      if (__pages_imported % 100 == 0) {
        fprintf(stdout,"\rPages Imported: %9llu",(unsigned long long)__pages_imported);
        fflush(stdout);
      }

      return page;
    }
  } else {
    return NULL;
  }
}

static bool compare_page_ids(const WikipediaPageImpl* a, const WikipediaPageImpl* b)
{
  return a->id() < b->id();
}

void 
WikipediaPageManagerImpl::core_dump(const string& dump_file)
{
  cout << "\nWriting Core Dump to " << dump_file << endl;
  ofstream core(dump_file.c_str());
  list<WikipediaPageImpl*> sorted_pages;
  if (core.is_open()) {
    for(PageSet::const_iterator ii = pages_.begin(); ii != pages_.end(); ii++) {
      WikipediaPageImpl* page = *ii;
      sorted_pages.push_back(page);
    }

    sorted_pages.sort(compare_page_ids);

    for (list<WikipediaPageImpl*>::const_iterator ii = sorted_pages.begin(), end = sorted_pages.end(); 
         ii != end; ii++) {
      WikipediaPageImpl* page = *ii;
      core << "(" << page->id_ << ",'" << page->title_ << "'," << page->outlinks_len_ << "," << page->inlinks_len_ << ")" << endl;
    }

    core.close();
  } else {
    cout << "Couldn't Open Core Dump" << endl;
  }
}


WikipediaPage::Manager* WikipediaPage::Manager::instance_ = NULL;
WikipediaPage::Manager* WikipediaPage::Manager::instance() {
  if (!instance_) {
    //__blacklist = new WordList(WordList::BLACK);
    __page_manager_instance = new WikipediaPageManagerImpl();
    instance_ = __page_manager_instance;
  }
  return instance_;
}

WikipediaPageIteratorImpl::WikipediaPageIteratorImpl(WikipediaPageManagerImpl* man)
{
  it_ = man->pages_.begin();
  end_ = man->pages_.end();
}

WikipediaPage*
WikipediaPageIteratorImpl::next()
{
  if (it_ == end_) {
    return NULL;
  } else {
    WikipediaPage* cur = *it_;
    it_++;
    return cur;
  }
}

//-----------------------------------------------------------------
//-- S t a t i c    P r i v a t e    I m p l e m e n t a t i o n --
//-----------------------------------------------------------------
//
static void
save_xml_progress_to_page(char* title, char* text, void* traveler)
{
  if (title && useful_title(title,title + strlen(title)) && text) {
    __page_manager_instance->perf_new_page(title,text);
  }
  free(title);
  free(text);
}

static bool compare_distances(page_relationship_t& first, page_relationship_t& second)
{
  if (first.distance == second.distance)
    return ((first.second)->len_inlinks() < (second.second)->len_inlinks());
  return (first.distance < second.distance);
}

static inline uint32_t distance_calculation(uint32_t inlinks_to_page1, uint32_t inlinks_to_page2, uint32_t common_count, bool bidirectional)
{
  uint32_t denom;
  uint32_t offset;
  double biggest_inlinks = 293424;
  double exponent = 5.0f;
  double biggest_exponent = 12.0;
  double smallest_exponent = 1.0;
  if (bidirectional) {
    if (inlinks_to_page1 <= inlinks_to_page2) {
      denom = inlinks_to_page1 + 1;
    } else {
      denom = (inlinks_to_page1 + inlinks_to_page2) / 2 + 1;
    }
    offset = 10;
    exponent = 15.0f;
  } else {
    denom = (int)(0.97f * (inlinks_to_page1 + 1) + 0.03f * (inlinks_to_page2));
    offset = 15;
  }
  exponent = max((double)smallest_exponent,min((double)biggest_exponent,(double)log(biggest_inlinks/(float)denom)));
  //fprintf(stderr,"%d::%d::%d::%d::%d::%f\n", inlinks_to_page1, inlinks_to_page2, denom, common_count, (int) bidirectional, exponent);
  return ((int) (50.0 * pow(1.0f - ((double)common_count / denom), exponent))) + offset;
}
