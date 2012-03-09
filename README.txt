OPEN RELATIONSHIP GRAPH
=======================

Open Relationship Graph is a JSON graph that describes how concepts relate to one another. There is code to generate it from Wikipedia and such open sources, and to utilize (finding paths between concepts). Unlike Neo4J, it is not a graph database, and unlike Twitter's recently open sourced Cassovary graph processing library, it cannot currently handle graphs of billions of edges. However, future directions may include expanding its size capabilities. 


CODE
----
The top level code is divided into 3 directories by language.

meguro contains Javascript files for use with https://github.com/jubos/meguro , a simple map/reduce framework by jubos that uses Spidermonkey, for outputting JSON-format graph representations from a Wikipedia parse tree (see below) or other inputs. 

The c subdirectory contains, what else, all the C++ code. Within it, hash_fns is a utility directory for hash functions. wikipedia contains code for constructing a JSON parse tree that can be loaded into a graph, which can be tested with path finding by relationship_server and relationship_client. The relationship server uses bidirectional Dijkstra's algorithm (http://en.wikipedia.org/wiki/Dijkstra's_algorithm). disambig contains experiments with disambiguation of concepts using the graph.

ruby contains utility Ruby scripts, including download_crunchbase_into_tc.rb (does what it says, into a Tokyo Cabinet) and merge_graphs.rb, which can be used to combine two JSON graphs (for example, one from Wikipedia and one from Crunchbase).

A good starting point and base use would be to construct a parse tree using c/wikipedia, convert it to a graph with meguro/wikipedia, load the graph with relationship_server, and interact using relationship_client.


LICENSE
-------
@WalmartLabs is releasing this under an MIT License, a copy of which is also included at the top level of the source tree.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


CONTRIBUTORS
------------
Guha Jayachandran, @guha
Curtis Spencer, @jubos
