#!/bin/bash

outname="output/parse-tree"
dir='.'

cd meguro/wikipedia || exit 1
./wpt_graph_sequence.sh $1 || exit 1

cd ../ruby || exit 1
ruby merge_graphs.rb ../meguro/wikipedia/$outname.final_graph.tch $dir/data/graphs/crunchbase.graph2.tch $dir/data/graphs/mergedWithDists.tch || exit 1 

cd ../meguro/graph || exit 1
meguro -a 16G -x 1G -t 4 -j build_concept_dict.js -m $dir/data/concept_dict.map.tch -r $dir/data/concept_dict.tch $dir/data/graphs/mergedWithDists.tch || exit 1

