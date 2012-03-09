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
#include "relationship_holder.h"


int
main(int argc, char **argv)
{
  shared_ptr<RelationshipHolder> rh = RelationshipHolder::instance();
  if (argc != 3) {
    cout << "Please specify a filename to load relationships from and cluster config file to get mysql data" << endl;
    exit(0);
  }
  rh->cluster_config_file(argv[2]);
  rh->load(argv[1]);
  cout << rh->statistics();
  rh->full_sorted_output();
  return 0;
}
