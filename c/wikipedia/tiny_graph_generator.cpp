#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "wikipedia_types.h"

using namespace std;

int main(int argc,char** argv)
{
  if (argc < 2) {
    cout << "Specify a tiny index root" << endl;
    exit(-1);
  }

  string index_root = argv[1];

  ofstream page_index((index_root + ".pages").c_str());
  if(page_index.is_open()) {
    page_index << "1|A|0" << endl;
    page_index << "2|B|0" << endl;
    page_index << "3|C|0" << endl;
    page_index << "4|D|0" << endl;
    page_index << "5|E|0" << endl;
    page_index << "6|F|0" << endl;
    page_index << "8|G|0" << endl;
    page_index << "9|H|0" << endl;
    page_index << "10|I|0" << endl;
    page_index << "11|J|0" << endl;
    page_index << "12|K|0" << endl;
    page_index << "13|X|0" << endl;
    page_index << "14|Y|0" << endl;
  }

  ofstream sparse_graph((index_root + ".sparse_graph").c_str());
  serialized_relationship_t rel;
  if(sparse_graph.is_open()) {
    // A
    rel.first_id = 1; rel.second_id = 2; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));
    rel.first_id = 1; rel.second_id = 3; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));
    rel.first_id = 1; rel.second_id = 13; rel.distance = 2; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));
    rel.first_id = 1; rel.second_id = 14; rel.distance = 2; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // B
    rel.first_id = 2; rel.second_id = 11; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));
    rel.first_id = 2; rel.second_id = 12; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // C
    rel.first_id = 3; rel.second_id = 4; rel.distance = 4; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // D
    rel.first_id = 4; rel.second_id = 5; rel.distance = 2; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // E
    rel.first_id = 5; rel.second_id = 4; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));
    rel.first_id = 5; rel.second_id = 6; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // I
    rel.first_id = 10; rel.second_id = 9; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // K
    rel.first_id = 12; rel.second_id = 10; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // X
    rel.first_id = 13; rel.second_id = 6; rel.distance = 3; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));

    // Y
    rel.first_id = 14; rel.second_id = 4; rel.distance = 1; sparse_graph.write((char*) &rel,sizeof(serialized_relationship_t));
  }
}
