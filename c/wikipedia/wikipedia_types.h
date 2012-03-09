#ifndef __WIKIPEDIA_TYPES__
#define __WIKIPEDIA_TYPES__

#include <string>
#include <stdint.h>

using namespace std;

typedef struct relationship_t {
  string first;
  string second;
  unsigned char distance;
} relationship_t;

typedef struct {
  uint32_t first_id;
  uint32_t second_id;
  unsigned char distance;
} serialized_relationship_t;

typedef struct {
  uint32_t first_id;
  uint32_t second_id;
  uint32_t in_text;
  uint32_t count;
} serialized_outlink_t;

typedef struct {
  int first_id;
  int second_id;
  unsigned char distance;
} serialized_edge_t;

#endif
