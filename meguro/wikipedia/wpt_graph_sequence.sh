#!/bin/bash

#cmd_start="/home/curtis/local/bin/meguro -b -n 100000000 -a 8G -x 1G -t 4"
cmd_start="meguro -b -n 100000000 -a 8G -x 1G -t 4"
outdir="output"
input=$1
[ ! $input ] && echo "Usage" && exit 1

outname=$outdir/`basename $input .tch`

echo "Output will go to $outname.*"

echo "Initial map/reduce from raw Wikipedia parse tree"
$cmd_start -j graph_from_wpt_phase1.js -m $outname.map.tch -r $outname.red.tch $input || exit 1

if [ 1 -eq 1 ]; then
echo "Map on the reduced file above to get just redirects"
$cmd_start -j phase1_to_redirects.js -m $outname.redirects.tch $outname.red.tch || exit 1

echo "Map on redirects.tch to get a file of ultimate redirects (so all values are canonical), using redirects as dictionary"
$cmd_start -j redirects_to_ultimate_redirects.js -d $outname.redirects.tch -m $outname.ultimate_redirects.tch $outname.redirects.tch || exit 1

echo "Map on phase 1 output using ultimate_redirects as a dictionary to get a set of pipe-delimited outlinks for each key"
$cmd_start -j phase1_to_outlinks.js -d $outname.ultimate_redirects.tch -m $outname.outlinks.tch $outname.red.tch || exit 1

echo "$outname.outlinks.tch is ready to be used in the C distance calculator"
#../../c/wikipedia/bin/gcc-4.3.0/release/wikipedia_indexer -t tokyo -y tokyo -i $outname.outlinks.tch -s -e 50 -o $outname.distances.tch || exit 1
#X/home/curtis/code/smallworld/src/c/wikipedia/bin/gcc-4.4.4/debug/wikipedia_indexer -t tokyo -y tokyo -i $outname.outlinks.tch -s -e 50 -o $outname.distances.tch || exit 1

echo "Map/reduce on original map, using ultimate_redirects as dictionary, to get a Relationship Graph formatted tch with all edges canonically named"
$cmd_start -j phase1_to_canonicalized_graph_format.js -d $outname.ultimate_redirects.tch -m $outname.map2.tch -r $outname.red2.tch $outname.red.tch || exit 1

echo "Now can map on canon_graph, with distances as dictionary, to get final graph"
$cmd_start -j fill_in_distances_to_graph.js -d $outname.distances.tch -m $outname.final_graph.tch $outname.red2.tch || exit 1

else
echo "Map/reduce on original map, using ultimate_redirects as dictionary, to get a Relationship Graph formatted tch with all edges canonically named"
$cmd_start -j phase1_to_canonicalized_graph_format.js -d $outname.ultimate_redirects.tch -m $outname.map2.tch -r $outname.red2.tch $outname.red.tch || exit 1
fi

