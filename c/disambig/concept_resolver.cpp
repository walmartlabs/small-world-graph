#include "concept_resolver.h"
#include <pillowtalk.h>
#include <tcutil.h>
#include <tchdb.h>
#include "benchmark.h"

typedef struct {
  int canonical_id;
  int score;
  int second_best_score;
  int overlap;
  int second_best_overlap;
  int smallest_neighbor_distance;
} score_t;

typedef dense_hash_set<char*, PaulHsiehHash, eqstr> StringSet;
typedef dense_hash_set<char*,char*,PaulHsiehHash, eqstr> StringMap;
typedef dense_hash_map<char*,score_t, PaulHsiehHash, eqstr> StringToScoreTup;

//static bool exists_in(const str_array_t* array, const char* needle);


ConceptResolver::ConceptResolver(const char* alias_tch, const char* neighbors_tch)
{
  canonical_id_mapping_.set_empty_key(NULL);

  id_canonical_mapping_allocated_ = N_CANONICALS_ALLOCATION_JUMP;
  id_canonical_mapping_ = (char **)malloc(sizeof(char*)*id_canonical_mapping_allocated_);
  if (!id_canonical_mapping_) {
    fprintf(stderr, "Failed malloc 1 in constructor\n");
    exit(1);
  }
  load_aliases(alias_tch);
  
  //printf("Mapping size: %d\n%s\n%d\n", (int)canonical_id_mapping_.size(), id_canonical_mapping_[300], canonical_id_mapping_.find("South Africa")->second);

  neighbors_ = (canonical_quality_ary_t*)malloc(canonical_id_mapping_.size()*sizeof(canonical_quality_ary_t));
  load_neighbors(neighbors_tch);

  //canonical_id_to_cands = (candidate_quality_ary_t*)malloc((canonical_id_mapping_.size()+1)*sizeof(canonical_quality_ary_t));
  //canonical_ids_seen = (int*)malloc(canonical_id_mapping_.size()*sizeof(int));
}

ConceptResolver::~ConceptResolver()
{
  for(StringToCanonicalQualityArray::iterator ii = aliases_.begin(); ii != aliases_.end(); ii++) {
    canonical_quality_ary_t set = ii->second;
    if (set.canonicals)
      free(set.canonicals);
  }
  aliases_.clear();

  for (size_t i = 0; i < canonical_id_mapping_.size(); i++) {
    if (neighbors_[i].canonicals)
      free(neighbors_[i].canonicals);
  }
  free(neighbors_);

  free(id_canonical_mapping_);
  //free(canonical_id_to_cands);
  //free(canonical_ids_seen);

  delete pool_;
}

void 
ConceptResolver::resolve(const vector<char*>& input,vector<resolved_concept_t>& output)
{
  //bench_start("resolve");
  //IntToCandidateQualityArray canonical_id_to_cands;
  int n_canonical_ids_seen = 0;
  StringToScoreTup bests;
  int* canonical_ids_seen = (int*)malloc(canonical_id_mapping_.size()*sizeof(int));

  //canonical_id_to_cands.set_empty_key(-1);
  //candidate_quality_ary_t* canonical_id_to_cands = (candidate_quality_ary_t*)malloc((canonical_id_mapping_.size()+1)*sizeof(canonical_quality_ary_t));
  //memset(canonical_id_to_cands, 0, canonical_id_mapping_.size()*sizeof(candidate_quality_ary_t));
  candidate_quality_ary_t* canonical_id_to_cands = (candidate_quality_ary_t*)calloc(canonical_id_mapping_.size()+1, sizeof(canonical_quality_ary_t));
  bests.set_empty_key(NULL);

  for(vector<char*>::const_iterator ii = input.begin(); ii != input.end(); ii++) {
    char* text_rep = *ii;
    //cout << "Finding Canonicals for " << text_rep << endl;
    StringToCanonicalQualityArray::const_iterator res = aliases_.find(text_rep);
    if (res != aliases_.end()) {
      canonical_quality_ary_t cqa = res->second;
      //str_array_t canonicals = res->second;
      for(int i=0; i < cqa.len; i++) {
        canonical_quality_t cqt = cqa.canonicals[i];
        int canonical_id = cqt.canonical_id;
        /*
        if (canonical_id < 0) {
          fprintf(stderr, "bad\n");
          exit(1);
        }
        */
        //cout << "\t" << canonical_id << " : " << cqt.quality << endl;
        //IntToCandidateQualityArray::iterator existing = canonical_id_to_cands.find(canonical_id);
        //if (existing != canonical_id_to_cands.end()) {
        if (canonical_id_to_cands[canonical_id].candidates) {
          //candidate_quality_ary_t* ary = &existing->second;
          candidate_quality_ary_t* ary = &(canonical_id_to_cands[canonical_id]);
          ary->candidates= (candidate_quality_t*) realloc(ary->candidates, sizeof(candidate_quality_t) * (ary->len + 1));
          ary->candidates[ary->len].candidate = text_rep;
          ary->candidates[ary->len].quality= cqt.quality;
          ary->len++;
        } else {
          candidate_quality_ary_t ary;
          ary.candidates= (candidate_quality_t*) malloc(sizeof(candidate_quality_t));
          ary.len = 1;
          ary.candidates[0].candidate = text_rep;
          ary.candidates[0].quality = cqt.quality;
          canonical_id_to_cands[canonical_id] = ary;
          canonical_ids_seen[n_canonical_ids_seen++] = canonical_id;
        }
      }
    }
  }

  //for(IntToCandidateQualityArray::const_iterator ii = canonical_id_to_cands.begin(); ii != canonical_id_to_cands.end(); ii++) {
  for (int ii = 0; ii < n_canonical_ids_seen; ii++) {
    //int canon_id = ii->first;
    int canon_id = canonical_ids_seen[ii];
    //const candidate_quality_ary_t* candidates = &ii->second;
    const candidate_quality_ary_t* candidates = &(canonical_id_to_cands[canon_id]);

    //cout << "Searching for Neighbors for " << canon_id << endl;
    canonical_quality_ary_t neighbors = neighbors_[canon_id];
    int overlap_count = 0;
    int smallest_neighbor_distance = 9999999;
    int distance_score = 0; //candidates->candidates[0].quality;
    for(int i=0; i < neighbors.len; i++) {
      canonical_quality_t neigh = neighbors.canonicals[i];
      //cout << "\t" << neigh.canonical_id << ":" << neigh.quality << endl;
      //IntToCandidateQualityArray::const_iterator res2 = canonical_id_to_cands.find(neigh.canonical_id);
      //if (res2 !=  canonical_id_to_cands.end()) {
      if (neigh.canonical_id >= 0) {
        if (memcmp(&(canonical_id_to_cands[neigh.canonical_id]), &(canonical_id_to_cands[canonical_id_mapping_.size()]), sizeof(candidate_quality_ary_t)) != 0) {
          //const candidate_quality_ary_t* neighbor_cands = &res2->second;
          const candidate_quality_ary_t* neighbor_cands = &(canonical_id_to_cands[neigh.canonical_id]);
          bool same_origin = false;
          for(int i=0; i < neighbor_cands->len; i++) {
            for(int j=0; j < candidates->len; j++) {
              // We can use == here, because of the string pool.
              if (neighbor_cands->candidates[i].candidate == candidates->candidates[j].candidate) {
                same_origin = true;
                goto same_origin_loop_done;
              }
            }
          }
same_origin_loop_done:
          if (!same_origin) {
            //cout << "\t\t" <<  "Found an overlap for " << neigh.canonical_id << endl;
            distance_score += neigh.quality;
            overlap_count++;
            if (neigh.quality < smallest_neighbor_distance) 
              smallest_neighbor_distance = neigh.quality;
          } else {
            //cout << "\t\t" <<  "Same Origin Candidate Found for potential overlap for " << neigh.canonical_id << endl;
          }
        }
      }
    }

    int final_score = ((1 + overlap_count) * 100) - distance_score;
      
    for(int i=0; i < candidates->len; i++) {
      char* candidate = candidates->candidates[i].candidate;
      int score_with_alias = final_score - candidates->candidates[i].quality;
      //cout << candidate << " : " << id_canonical_mapping_[canon_id] << " : (" << score_with_alias << ", " << overlap_count << ", " << distance_score << ", " << candidates->candidates[i].quality << ")" << endl;
      StringToScoreTup::const_iterator cur_best = bests.find(candidate);
      if (cur_best != bests.end()) {
        if (cur_best->second.score < score_with_alias) {
          score_t new_score;
          new_score.second_best_score = cur_best->second.score;
          new_score.second_best_overlap = cur_best->second.overlap;
          new_score.canonical_id = canon_id;
          new_score.score = score_with_alias;
          new_score.smallest_neighbor_distance = smallest_neighbor_distance;
          new_score.overlap = overlap_count;
          bests[candidate] = new_score;
        } else if (cur_best->second.second_best_score < score_with_alias) {
          score_t new_score = cur_best->second;
          new_score.second_best_score = score_with_alias;
          new_score.second_best_overlap = overlap_count;
          bests[candidate] = new_score;
        }
      } else {
        score_t new_score;
        new_score.canonical_id = canon_id;
        new_score.score = score_with_alias;
        new_score.second_best_score = 0;
        new_score.second_best_overlap = 0;
        new_score.smallest_neighbor_distance = smallest_neighbor_distance;
        new_score.overlap = overlap_count;
        bests[candidate] = new_score;
      }
    }
  }

  for(StringToScoreTup::const_iterator ii = bests.begin(); ii != bests.end(); ii++) {
    score_t score = ii->second;
    int score_diff = score.score - score.second_best_score;
    //cout << ii->first << ": score-" << score.score << ", 2score-" << score.second_best_score << ", overlap-" << score.overlap << ", 2overlap" << score.second_best_overlap << endl;
    if ((score_diff > 75 || (score.second_best_overlap == 0 && score.overlap > 0 && score_diff > 50) || (score.overlap >= score.second_best_overlap && score.score > 190 && score.second_best_overlap < 3)) && (score.score > 150 || score.smallest_neighbor_distance < 50 || (score.second_best_overlap == 0 && score_diff >= 50 && neighbors_[score.canonical_id].len == NEIGHBOR_LIMIT))) { // magic numbers here for thresholding
      resolved_concept_t concept;
      concept.text_rep = ii->first;
      concept.canonical = id_canonical_mapping_[score.canonical_id];
      concept.score = score.score;
      output.push_back(concept);
    }
  }

  //for(IntToCandidateQualityArray::iterator ii = canonical_id_to_cands.begin(); ii != canonical_id_to_cands.end(); ii++) {
  for (int i = 0; i < n_canonical_ids_seen; i++) {
    int canon_id = canonical_ids_seen[i];
    //candidate_quality_ary_t* array = &ii->second;
    free(canonical_id_to_cands[canon_id].candidates);
  }

  //bench_finish("resolve");
  free(canonical_id_to_cands);
  free(canonical_ids_seen);
}


/* Protected Implementation */

int ConceptResolver::retrieve_canonical_id(char *str, bool add) {
  StringToInt::const_iterator res = canonical_id_mapping_.find(str);
  if (add) {
    if (res == canonical_id_mapping_.end()) {
      int new_id = canonical_id_mapping_.size();
      id_canonical_mapping_[new_id] = str;
      canonical_id_mapping_[str] = new_id;
      if (canonical_id_mapping_.size() == id_canonical_mapping_allocated_) {
        id_canonical_mapping_allocated_ += N_CANONICALS_ALLOCATION_JUMP;
        id_canonical_mapping_ = (char **)realloc(id_canonical_mapping_, sizeof(char *)*(id_canonical_mapping_allocated_));
        if (!id_canonical_mapping_) {
          fprintf(stderr, "Failed realloc of id_canonical_mapping_ to %d\n", (int)id_canonical_mapping_allocated_);
          exit(1);
        }
      }
      return new_id;
    }
    else
      return res->second;
  } else
    return (res == canonical_id_mapping_.end() ? -1 : res->second);
}


/*
 *  Iterate through the aliases in the map and parse the json and make vectors
 */
void 
ConceptResolver::load_aliases(const char* aliases_tch)
{
  int ecode;
  TCHDB* hdb = tchdbnew();
  if(!tchdbtune(hdb,-1,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if(!tchdbopen(hdb, aliases_tch, HDBOREADER)){
    ecode = tchdbecode(hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  uint64_t num_records = tchdbrnum(hdb);

  pool_ = new StringPool(num_records);

  aliases_.set_empty_key(NULL);
  aliases_.resize(num_records);

  if (!tchdbiterinit(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "iterator error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  int key_len;
  int val_len;
  char* key;
  uint64_t count = 1;

  fprintf(stderr,"\rLoading Aliases: %9d / %d", (int) count, (int) num_records);
  fflush(stderr);

  while((key = (char*) tchdbiternext(hdb,&key_len))) {
    char* value = (char*) tchdbget(hdb,key,key_len,&val_len);

    if (!strcmp(key,"")) {
      free(key);
      free(value);
      continue;
    }

    char* pooled_key = pool_->retrieve(key);

    if (value) {
      bool in_score = false;
      int len = strlen(value);
      canonical_quality_ary_t ary;
      ary.canonicals = NULL;
      ary.len = 0;
      char* cur = value;
      canonical_quality_t* cur_canon = NULL;
      for(int i=0; i < len; i++) {
        if (value[i] == '|') {
          value[i] = '\0';
          if (in_score) {
            cur_canon->quality = atoi(cur);
            cur = value + i + 1;
            in_score = false;
          } else {
            ary.canonicals = (canonical_quality_t*) realloc(ary.canonicals, sizeof(canonical_quality_t) * (ary.len + 1));
            char *pooled_cur = pool_->retrieve(cur);
            ary.canonicals[ary.len].canonical_id = retrieve_canonical_id(pooled_cur, true);
            cur_canon = &ary.canonicals[ary.len];
            ary.len++;
            cur = value + i + 1;
            in_score = true;
          }
        }
      }
      if(cur_canon)
        cur_canon->quality = atoi(cur);
      /*
      printf("Saving %s\n", pooled_key);
      for(int i=0; i < ary.len;i++) {
        printf("\t%s:%d\n",ary.canonicals[i].canonical,ary.canonicals[i].quality);
      }
      */
      aliases_[pooled_key] = ary;
      free(key);
      free(value);
    }

    if (count % 1000 == 0) {
      fprintf(stderr,"\rLoading Aliases: %9d / %d", (int) count, (int) num_records);
      fflush(stderr);
    }
    count++;
  }
  fprintf(stderr,"\rLoading Aliases: %9d / %d\n", (int) count, (int) num_records);
  fflush(stderr);

  if (!tchdbclose(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
  }

  tchdbdel(hdb);
}

/*
 *  Iterate through the neigbhor keys in the map and parse the json and make vectors
 */
void 
ConceptResolver::load_neighbors(const char* neighbors_tch)
{
  int ecode;
  TCHDB* hdb = tchdbnew();
  if(!tchdbtune(hdb,-1,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if(!tchdbopen(hdb, neighbors_tch, HDBOREADER)){
    ecode = tchdbecode(hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  uint64_t num_records = tchdbrnum(hdb);

  if (!tchdbiterinit(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "iterator error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  int key_len;
  int val_len;
  char* key;
  uint64_t count = 1;

  fprintf(stderr,"\rLoading Neighbors: %9d / %d", (int) count, (int) num_records);
  fflush(stderr);

  while((key = (char*) tchdbiternext(hdb,&key_len))) {
    char* value = (char*) tchdbget(hdb,key,key_len,&val_len);

    if (!strcmp(key,"")) {
      free(key);
      free(value);
      continue;
    }

    char* pooled_key = pool_->retrieve(key);

    if (pooled_key) {
      int key_id = retrieve_canonical_id(pooled_key, false);
      if (key_id < 0) {
        free(key);
        free(value);
        continue;
      }
      bool in_score = false;
      int neighbors_count = 0;
      int len = strlen(value);
      canonical_quality_ary_t ary;
      ary.canonicals = NULL;
      ary.len = 0;
      char* cur = value;
      canonical_quality_t* cur_canon = NULL;

      for(int i=0; i < len; i++) {
        if (value[i] == '|') {
          value[i] = '\0';
          if (in_score) {
            cur_canon->quality = atoi(cur);
            cur = value + i + 1;
            in_score = false;
            if (neighbors_count == NEIGHBOR_LIMIT)
              goto cutoff;

          } else {
            ary.canonicals = (canonical_quality_t*) realloc(ary.canonicals, sizeof(canonical_quality_t) * (ary.len + 1));
            ary.canonicals[ary.len].canonical_id = retrieve_canonical_id(pool_->retrieve(cur), false);
            // note that this id can be -1 if neighbors.tch has mentions of things that aren't in aliases (this seems to happen with lower casing). It's up to user to check that not -1
            cur_canon = &ary.canonicals[ary.len];
            ary.len++;
            cur = value + i + 1;
            in_score = true;
            neighbors_count++;
          }
        }
      }
      if(cur_canon)
        cur_canon->quality = atoi(cur);

cutoff:
      neighbors_[key_id] = ary;
    }
    count++;
    free(key);
    free(value);
    
    if (count % 1000 == 0) {
      fprintf(stderr,"\rLoading Neighbors: %9d / %d", (int) count, (int) num_records);
      fflush(stderr);
    }
  }
  fprintf(stderr,"\rLoading Neighbors: %9d / %d\n", (int) count, (int) num_records);
  fflush(stderr);

  if (!tchdbclose(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
  }

  tchdbdel(hdb);
}

/*
static bool exists_in(const str_array_t* array,const char* needle) {
  for(int i=0; i < array->len; i++) {
    if (array->ary[i] == needle) {
      return true;
    }
  }
  return false;
}
*/
