#ifndef __WIKI_PARSER__
#define __WIKI_PARSER__

/* - name
 *    name of the link ie. [[Cruxlux]] -> Cruxlux
 * - count
 *    number of times the link appears
 * - in_text
 *    if the link appears in the text of the page as opposed to in a template or infobox
 */
typedef struct {
  char* name;
  size_t count;
  bool in_text;
} link_t;

typedef struct {
  link_t** links;
  size_t len;
} link_array_t;

typedef struct {
  char* redirect;
  bool ambiguous;
  link_array_t outlinks;
  link_array_t image_links;
} wiki_parse_tree_t;

void free_parse_tree(wiki_parse_tree_t* tree);

void print_parse_tree(wiki_parse_tree_t* tree);

wiki_parse_tree_t* wiki_parse(const char* content);

/* This method will return back a rendered form of wikitext that is human
 * readable plain text */
char* wikitext_to_plaintext(const char* content);

#endif 
