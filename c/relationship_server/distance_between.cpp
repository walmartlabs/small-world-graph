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
    cout << "Please specify two url arguments" << endl;
    exit(1);
  }
  rh->cluster_config_file("/etc/cruxlux_config");
  rh->calculate_distance_between(argv[1],argv[2]);
  rh->full_sorted_output();
  return 0;
}
