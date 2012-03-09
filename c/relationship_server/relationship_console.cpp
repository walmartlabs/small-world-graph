#include <iostream>
#include "cruxlux_config.h"
#include "site_holder.h"
#include "relationship_holder.h"

using namespace std;

/* Globals */
shared_ptr<SiteHolder> __sh;
shared_ptr<RelationshipHolder> __rh;

static void do_close_sites() 
{
  char input[1024];
  cout << "Type in a site name: ";
  cin.getline(input,1024);
  if (cin.eof()) {
    cout << endl;
    return;
  }

  uint32_t external_site_id = __sh->url_to_id(input);
  if (external_site_id > 0) {
    cout << "Found Site: " << external_site_id << endl;
    CloseSiteVector sites;
    __rh->sites_close_to(sites,external_site_id);
    for(CloseSiteVector::const_iterator ii = sites.begin(), end = sites.end(); ii != end; ii++) {
      int id = *ii;
      const char* url = __sh->id_to_url(id);
      if (url) {
        cout << url << endl;
      }
    }
  }
}


int
main(int argc, char **argv)
{
  shared_ptr<CruxluxConfig> cc = CruxluxConfig::instance();
  cc->filename("/etc/cruxlux_config");

  __sh = SiteHolder::instance();
  __sh->initialize();

  __rh = RelationshipHolder::instance();
  __rh->load("/mnt/relationships/relationships");

  char input[256];
  for(;;) {
    cout << "What operation do you want to do\n0: Close Sites\n" << endl;
    cout << "Type in a number: ";
    cin.getline(input,256);
    if (cin.eof()) {
      cout << endl;
      break;
    }
    int choice = atoi(input);
    switch(choice) {
      case 0:
        do_close_sites();
        break;
      case 1:
        //do_distances();
        break;
      case 2:
        //do_save_neighbors();
        break;
      case 3:
        //do_load_neighbors();
        break;
    }

  }
	return 0;
}
