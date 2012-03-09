#include <sys/types.h>
/* Required by event.h. */
#include <sys/time.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include <iostream>
#include <string>
#include <map>
#include <mysql/mysql.h>
#include "relationship_holder.h"
#include "benchmark.h"


int
main(int argc, char **argv)
{
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  rh->cluster_config_file("cluster_benchmark.yml");
  rh->build();
  rh->persist("/tmp/benchmark_relationships");
  return 0;
}
