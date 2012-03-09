#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "equality.h"
#include "sixty_four_bit_hash.h"
#include <google/sparse_hash_map>

using namespace std;
using google::sparse_hash_map;

typedef sparse_hash_map<u_int64_t,unsigned char,SixtyFourBitHash, eqlong> RelationshipHash;

typedef struct relationship_t {
  uint32_t first_id;
  uint32_t second_id;
  unsigned char distance;
} relationship_t;

int main(int argc,char** argv)
{
  RelationshipHash rels;
  relationship_t rel;
  if (argc < 2) {
    cerr << "Please specify a binary relationship file" << endl;
    exit(-1);
  }

  ifstream input(argv[1],ios::out | ios::binary);
  if (!input.is_open()) {
    cerr << "Cannot open outfile" << endl;
    exit(-1);
  }

  while(!input.eof()) {
    input.read((char*) &rel,sizeof(relationship_t));
    //cout << rel.first_id << "::" << rel.second_id << "::" << (int) rel.distance << endl;
    rels[composite_key(rel.first_id,rel.second_id)] = rel.distance;
  }
  cout << rels.size() << " Relationships Loaded" << endl;
  string line;
  cin >> line;
  return 0;
}
