#ifndef __CONCEPT_RESOLVER_H__
#define __CONCEPT_RESOLVER_H__

#include <vector>
#include <google/dense_hash_set>
#include <google/dense_hash_map>
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "thomas_wang_hash.h"
#include "string_pool.h"

using google::dense_hash_map;
using google::dense_hash_set;
using namespace std;

#define THRESHOLD 1
#define NEIGHBOR_LIMIT 500
#define N_CANONICALS_ALLOCATION_JUMP 16384

typedef struct {
  int len;
  char** ary;
} str_array_t;

/*
 * For any given alias like Apple, we will have an array of Alias quality to
 * capture the fact that Apple Inc. is a better canonical than Apple I or Apple
 * Store.
 */
typedef struct {
  int canonical_id;
  int quality;
} canonical_quality_t;

typedef struct {
  int len;
  canonical_quality_t* canonicals;
} canonical_quality_ary_t;

typedef struct {
  char* candidate;
  int quality;
} candidate_quality_t;

typedef struct {
  int len;
  candidate_quality_t* candidates;
} candidate_quality_ary_t;

typedef struct {
  char* text_rep;
  char* canonical;
  int score;
} resolved_concept_t;

typedef vector<resolved_concept_t> ResolvedConceptList;

typedef dense_hash_map<char*,str_array_t, PaulHsiehHash, eqstr> StringToSetMap;
typedef dense_hash_map<char*, canonical_quality_ary_t, PaulHsiehHash, eqstr> StringToCanonicalQualityArray;
typedef dense_hash_map<int, candidate_quality_ary_t, ThomasWangHash, eqint> IntToCandidateQualityArray;
typedef dense_hash_map<char*, int, PaulHsiehHash, eqstr> StringToInt;

class ConceptResolver {
  public:
    ConceptResolver(const char* canonical_tch, const char* neighbors_tch);
    virtual ~ConceptResolver();
    void resolve(const vector<char*>& input,vector<resolved_concept_t>& output);
  protected:
    StringPool* pool_;
    StringToCanonicalQualityArray aliases_;
    canonical_quality_ary_t* neighbors_;
    StringToInt canonical_id_mapping_;
    char **id_canonical_mapping_;
    size_t id_canonical_mapping_allocated_;
    //candidate_quality_ary_t* canonical_id_to_cands_;
    //int* canonical_ids_seen_;

    void load_aliases(const char* aliases_path);
    void load_neighbors(const char* neighbors_path);
    int retrieve_canonical_id(char* canon_str, bool add);

};

#endif
