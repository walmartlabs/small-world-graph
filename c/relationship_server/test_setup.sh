#!/bin/sh
export PATH=.:$PATH
echo 'DROP DATABASE IF EXISTS rel_test' | mysql -u root
echo 'CREATE DATABASE rel_test' | mysql -u root
mysql -u root rel_test < ~/benchmarks/relationship_server/external_sites_small
if [ -e /tmp/test_sphinx_searchd.pid ] 
  then
    kill -9 `cat /tmp/test_sphinx_searchd.pid`
fi
indexer -c test_sphinx.conf --all > /dev/null 2>&1
searchd -c test_sphinx.conf > /dev/null 2>&1
#test_relationship_holder
#kill -9 `cat /tmp/test_sphinx_searchd.pid`
#echo 'DROP DATABASE IF EXISTS rel_test' | mysql -u root
