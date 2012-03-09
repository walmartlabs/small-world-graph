#!/bin/sh
export PATH=.:$PATH
echo 'DROP DATABASE IF EXISTS rel_benchmark' | mysql -u root
echo 'CREATE DATABASE rel_benchmark' | mysql -u root
mysql -u root rel_benchmark < ~/benchmarks/relationship_server/external_sites_medium
if [ -e /tmp/sphinx_searchd.pid ] 
  then
    kill -9 `cat /tmp/sphinx_searchd.pid`
fi
indexer -c benchmark_sphinx.conf --all 
#searchd -c benchmark_sphinx.conf > /dev/null 2>&1
searchd -c benchmark_sphinx.conf
#test_relationship_holder
#kill -9 `cat /tmp/test_sphinx_searchd.pid`
#echo 'DROP DATABASE IF EXISTS rel_test' | mysql -u root
