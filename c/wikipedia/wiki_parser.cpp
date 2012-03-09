#include "wiki_scanner.h"
#include "wiki_parser.h"

#include <sys/queue.h>
#include <google/dense_hash_set>
#include <google/dense_hash_map>
#include "equality.h"
#include "paul_hsieh_hash.h"
#include "wiki_infobox_parser.h"
#include "string_utils.h"

using namespace std;
using google::dense_hash_set;
using google::dense_hash_map;

/* Typedefs */
typedef dense_hash_set<char*,PaulHsiehHash,eqstr> NameSet;
typedef dense_hash_map<char*,link_t*,PaulHsiehHash,eqstr> NameLinkMap;

typedef enum {
  IMAGE,
  INTERNAL,
  EXTERNAL,
  TEMPLATE,
  KEY_VALUE,
  IGNORE
} link_type_t;

struct link_entry_t {
  link_type_t type;
  char* data;
  int len;
  char* key;
  char* key_stop;
  char* value;
  char* alias;
  char* alias_stop;
  bool done;
  bool invalid;
  bool anchor;
  TAILQ_ENTRY(link_entry_t) entries;
};

struct text_block_t {
  char* begin;
  char* end;
  bool need_to_free;
  TAILQ_ENTRY(text_block_t) entries;
};


static link_t* append_to_link_array(link_array_t* array,char* name, bool in_text)
{
  array->links = (link_t**) realloc(array->links,(array->len + 1) * sizeof(link_t*));
  link_t* link = (link_t*) malloc(sizeof(link_t));
  link->name = name;
  link->in_text = in_text;
  link->count = 1;
  array->links[array->len] = link;
  array->len++;
  return link;
}


/* 
 * This makes a new string from a beginning and an end 
 */
static char* make_str_from_begin_and_end(char* begin, char* end)
{
  int len = end - begin;
  if (len > 0) {
    char* new_str= (char*) malloc(len + 1);
    memcpy(new_str,begin,len);
    new_str[len] = '\0';
    return new_str;
  } else {
    return NULL;
  }
}

/* This method takes in a canonical name and an alias for it and builds an rdfa link for it */
/*
static char* build_rdfa_link(char* canonical, char* alias)
{
  char buffer[4096];
  string escaped_canonical = escape_double_quotes(canonical);
  snprintf(buffer,4096,"<a href=\"http://www.cruxlux.com/rc/report?tuples=%s\" rel=\"%s\">%s</a>",urlencode(canonical).c_str(),escaped_canonical.c_str(),alias);
  return strdup(buffer);
}
*/

static char* build_traditional_wiki_link(char* canonical, char* alias)
{
  char buffer[4096];
  snprintf(buffer,4096,"[[%s|%s]]",canonical,alias);
  return strdup(buffer);
}

/* Implementation */
void free_parse_tree(wiki_parse_tree_t* tree)
{
  for(size_t i=0; i < tree->outlinks.len; i++) {
    free(tree->outlinks.links[i]->name);
    free(tree->outlinks.links[i]);
  }
  free(tree->outlinks.links);
  for(size_t i=0; i < tree->image_links.len; i++) {
    free(tree->image_links.links[i]->name);
    free(tree->image_links.links[i]);
  }

  free(tree->image_links.links);

  free(tree->redirect);
  free(tree);
}

void print_parse_tree(wiki_parse_tree_t* tree)
{
  if (tree->ambiguous) {
    printf("+ Ambiguous");
  }
  if (tree->redirect) {
    printf("+ Redirect: %s\n",tree->redirect);
  }
  printf("+ Outlinks: %u (name,count,in_text)\n",(unsigned int) tree->outlinks.len);
  for(size_t i=0; i < tree->outlinks.len; i++) {
    cout << "--- ('" << tree->outlinks.links[i]->name << "'," << tree->outlinks.links[i]->count << "," << tree->outlinks.links[i]->in_text << ")" << endl;
  }
  printf("+ Image Links: %u\n", (unsigned int) tree->image_links.len);
  for(size_t i=0; i < tree->image_links.len; i++) {
    printf("---'%s'\n",tree->image_links.links[i]->name);
  }
}

wiki_parse_tree_t* wiki_parse(const char* content)
{
  wiki_parse_tree_t* parse_tree = NULL;
  link_entry_t* cur_link;
  bool redirect = false;
  TAILQ_HEAD(,link_entry_t) link_stack;
	TAILQ_INIT(&link_stack);

  NameSet image_set;
  NameLinkMap outlink_set;
  NameSet::const_iterator finder;
  NameLinkMap::const_iterator link_finder;

  image_set.set_empty_key(NULL);
  outlink_set.set_empty_key(NULL);

  int content_length = strlen(content);

  if (content_length != 0) {
    parse_tree = (wiki_parse_tree_t*) calloc(1,sizeof(wiki_parse_tree_t));
    wiki_token_t token;
    char* p = (char*) content;
    char* pe = p + content_length;
    scan(&token,NULL,p,pe);
    int ignore_semaphore = 0;
    int comment_semaphore= 0;
    char* comment_start = NULL;
    while(token.type != END_OF_FILE) {
      if (token.type == COMMENT_BEGIN) {
        comment_start = token.start;
        comment_semaphore++;
      } else if (token.type == COMMENT_END) {
        comment_semaphore--;
        if (!TAILQ_EMPTY(&link_stack)) {
          cur_link = TAILQ_FIRST(&link_stack);
          if (cur_link->type == TEMPLATE && cur_link->value) {
            // see if a workable image is here
            if (comment_start) {
              char* value_image = image_match(cur_link->value,comment_start); 
              if (value_image) {
                finder = image_set.find(value_image);
                if (finder == image_set.end()) {
                  image_set.insert(value_image);
                  append_to_link_array(&(parse_tree->image_links),value_image,false);
                  //parse_tree->image_links.push_back(value_image);
                } else
                  free(value_image);
              }
              cur_link->value = token.stop;
            }
          }
        }
        comment_start = NULL;
      } else if(!comment_semaphore) {
        if (token.type == IGNORE_BEGIN) {
          cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
          cur_link->data = token.stop;
          cur_link->type = IGNORE;
          ignore_semaphore++;
          TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
        } else if (token.type == IGNORE_END) {
          if (!TAILQ_EMPTY(&link_stack)) {
            cur_link = TAILQ_FIRST(&link_stack);
            char* ignore_start = cur_link->data;
            if (cur_link->type == IGNORE) {
              ignore_semaphore--;
            }
            TAILQ_REMOVE(&link_stack,cur_link,entries);
            free(cur_link);

            if (!TAILQ_EMPTY(&link_stack)) {
              cur_link = TAILQ_FIRST(&link_stack);
              if (cur_link->type == TEMPLATE && cur_link->value) {
                // see if a workable image is here
                char* value_image = image_match(cur_link->value,ignore_start); 
                if (value_image) {
                  finder = image_set.find(value_image);
                  if (finder == image_set.end()) {
                    image_set.insert(value_image);
                    append_to_link_array(&(parse_tree->image_links),value_image,false);
                    //parse_tree->image_links.push_back(value_image);
                  } else
                    free(value_image);
                }
                cur_link->value = token.stop;
              }
            }
          }
        } else if (token.type == TEMPLATE_END) {
          if (!TAILQ_EMPTY(&link_stack)) {
            cur_link = TAILQ_FIRST(&link_stack);
            if (cur_link->type == IGNORE) {
              ignore_semaphore--;
            }
            TAILQ_REMOVE(&link_stack,cur_link,entries);
            free(cur_link);
          }
        } else if (!ignore_semaphore) {
          switch(token.type) {
            case REDIRECT:
              redirect = true;
              break;

            case TEMPLATE_BEGIN:
              // If we are starting a template inside the link part of a link it is
              // probably a bogus link
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (!cur_link->done)
                  cur_link->invalid = true;
              }
              // Now make a new 
              cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
              cur_link->data = token.stop;
              cur_link->type = TEMPLATE;
              TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
              break;

            case IMAGE_LINK_BEGIN:
              cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
              cur_link->data = token.stop;
              cur_link->type = IMAGE;
              TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
              break;

            case LINK_BEGIN:
              cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
              cur_link->data = token.stop;
              cur_link->type = INTERNAL;
              TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
              break;

            case LINK_END: 
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                TAILQ_REMOVE(&link_stack,cur_link,entries);

                if (!cur_link->invalid) {
                  if (!cur_link->done) {
                    cur_link->len = token.start - cur_link->data;
                  }
                  if (redirect) {
                    char* sanitized_title = sanitize_title(cur_link->data,cur_link->data + cur_link->len);
                    if (sanitized_title)
                      parse_tree->redirect = sanitized_title;
                    free(cur_link);
                    goto parse_done;
                  } else {
                    switch(cur_link->type) {
                      case INTERNAL:
                        {
                          char* sanitized_title = sanitize_title(cur_link->data,cur_link->data + cur_link->len);
                          if (sanitized_title) {
                            link_finder = outlink_set.find(sanitized_title);
                            if (link_finder == outlink_set.end()) {
                              // If we are a top level link in the text, then save to an extra set
                              bool in_text = TAILQ_EMPTY(&link_stack);
                              link_t* new_link = append_to_link_array(&(parse_tree->outlinks),sanitized_title,in_text);
                              outlink_set[sanitized_title] = new_link;
                            } else {
                              link_t* existing_link = link_finder->second;
                              existing_link->count++;
                              if (TAILQ_EMPTY(&link_stack)) {
                                existing_link->in_text = true;
                              }
                              free(sanitized_title);
                            }
                          } else
                            free(sanitized_title);
                        }
                        break;

                      case IMAGE:
                        {
                          char* value_image = image_match(cur_link->data,cur_link->data + cur_link->len); 
                          if (value_image) {
                            finder = image_set.find(value_image); 
                            if (finder == image_set.end()) { 
                              image_set.insert(value_image); 
                              append_to_link_array(&(parse_tree->image_links),value_image,false);
                            } else {
                              free(value_image);
                            }
                          }
                        }
                        break;

                      default:
                        break;
                    }
                  }
                }
                free(cur_link);
              }
              break;

            case SEPARATOR:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == TEMPLATE) {
                  if (cur_link->key && cur_link->value) {
                    char* value_image = image_match(cur_link->value,token.start); 
                    if (value_image) {
                      finder = image_set.find(value_image);
                      if (finder == image_set.end()) {
                        image_set.insert(value_image);
                        append_to_link_array(&(parse_tree->image_links),value_image,false);
                        //parse_tree->image_links.push_back(value_image);
                      } else
                        free(value_image);
                    }
                  }
                  cur_link->key = token.stop;
                  cur_link->value = NULL;
                } else {
                  if (!cur_link->done) {
                    cur_link->len = token.start - cur_link->data;
                    cur_link->done = true;
                  }
                }
              }
              break;

            case EQUALS:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == TEMPLATE) {
                  cur_link->key_stop = token.start;
                  cur_link->value = token.stop;
                }           
              }
              break;

            case COLON:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                cur_link->invalid = true;
              }
              break;

            case CRLF:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == INTERNAL && !cur_link->done) {
                  cur_link->invalid = true;
                }
              }

            case HASH:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                cur_link->anchor = true;
                if (!cur_link->done) {
                  cur_link->len = token.start - cur_link->data;
                  cur_link->done = true;
                }
              }
              break;

            case DISAMBIGUATION:
              parse_tree->ambiguous = true;
              break;

            default:
              break;
          }
        }
      }
      //print_parse_tree(parse_tree);
      scan(&token,&token,NULL,pe);
    }

parse_done:

    // This is there in case there is a bad parse.
    while (!TAILQ_EMPTY(&link_stack)) {
      cur_link = TAILQ_FIRST(&link_stack);
      TAILQ_REMOVE(&link_stack,cur_link,entries);
      free(cur_link);
    }
  }
  return parse_tree;
}

char* wikitext_to_plaintext(const char* content)
{
  link_entry_t* cur_link;
  text_block_t* cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
  cur_text->begin = (char*) content;

  bool redirect = false;

  TAILQ_HEAD(,link_entry_t) link_stack;
	TAILQ_INIT(&link_stack);

  TAILQ_HEAD(,text_block_t) text_list;
	TAILQ_INIT(&text_list);

  int content_length = strlen(content);
  char* p = (char*) content;
  char* pe = p + content_length;

  if (content_length != 0) {
    wiki_token_t token;
    scan(&token,NULL,p,pe);
    int ignore_semaphore = 0;
    int comment_semaphore = 0;
    while(token.type != END_OF_FILE) {
      if (token.type == COMMENT_BEGIN) {
        if (TAILQ_EMPTY(&link_stack)) {
          cur_text->end = token.start;
          TAILQ_INSERT_TAIL(&text_list,cur_text,entries);
          cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
        }
        comment_semaphore++;
      } else if (token.type == COMMENT_END) {
        if (TAILQ_EMPTY(&link_stack) && comment_semaphore > 0) {
          cur_text->begin = token.stop;
        }
        if (comment_semaphore > 0)
          comment_semaphore--;
      } else if(comment_semaphore <= 0) {
        if (token.type == IGNORE_BEGIN) {
          //cout << "Ignore Begin" << endl;
          if (TAILQ_EMPTY(&link_stack)) {
            cur_text->end = token.start;
            //cout << "1. Appending:'" << string(cur_text->begin, cur_text->end - cur_text->begin) << "'" << endl;
            TAILQ_INSERT_TAIL(&text_list,cur_text,entries);
            cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
          }
          cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
          cur_link->data = token.stop;
          cur_link->type = IGNORE;
          ignore_semaphore++;
          TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
        } else if (token.type == IGNORE_END) {
          //cout << "Ignore End" << endl;
          // Comment Ending
          if (!TAILQ_EMPTY(&link_stack)) {
            cur_link = TAILQ_FIRST(&link_stack);
            if (cur_link->type == IGNORE) {
              ignore_semaphore--;
            }
            TAILQ_REMOVE(&link_stack,cur_link,entries);
            free(cur_link);

            if (TAILQ_EMPTY(&link_stack)) {
              //cout << "2. Setting Begin to: " << string(token.stop,10) << endl;
              cur_text->begin = token.stop;
            } else {
              cur_link = TAILQ_FIRST(&link_stack);
              if (cur_link->type == TEMPLATE && cur_link->value) {
                cur_link->value = token.stop;
              }
            }
          } 
        } else if (!ignore_semaphore) {
          switch(token.type) {
            case REDIRECT:
              redirect = true;
              goto parse_done;
              break;

            case TEMPLATE_END:
              //cout << "Template End" << endl;
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == IGNORE) {
                  ignore_semaphore--;
                }
                TAILQ_REMOVE(&link_stack,cur_link,entries);
                free(cur_link);
                if (TAILQ_EMPTY(&link_stack)) {
                  //cout << "3. Setting Begin to: " << string(token.stop,10) << endl;
                  cur_text->begin = token.stop;
                }
              }
              break;


            case IGNORABLE:
              if (TAILQ_EMPTY(&link_stack)) {
                cur_text->end = token.start;
                //cout << "4. Appending:'" << string(cur_text->begin, cur_text->end - cur_text->begin) << "'" << endl;
                TAILQ_INSERT_TAIL(&text_list,cur_text,entries);
                cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
                cur_text->begin = token.stop;
              } 
              break;

            case TEMPLATE_BEGIN:
              //cout << "Template Begin" << endl;
              // If we are starting a template inside the link part of a link it is
              // probably a bogus link
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (!cur_link->done)
                  cur_link->invalid = true;
              } else {
                cur_text->end = token.start;
                //cout << "Appending:'" << string(cur_text->begin, cur_text->end - cur_text->begin) << "'" << endl;
                TAILQ_INSERT_TAIL(&text_list,cur_text,entries);
                cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
              }
              // Now make a new 
              cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
              cur_link->data = token.stop;
              cur_link->type = TEMPLATE;
              TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
              break;

            case IMAGE_LINK_BEGIN:
              if (TAILQ_EMPTY(&link_stack)) {
                cur_text->end = token.start;
                //cout << "5. Appending:'" << string(cur_text->begin, cur_text->end - cur_text->begin) << "'" << endl;
                TAILQ_INSERT_TAIL(&text_list,cur_text,entries);
                cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
              } 

              cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
              cur_link->data = token.stop;
              cur_link->type = IMAGE;
              TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
              break;

            case LINK_BEGIN:
              if (TAILQ_EMPTY(&link_stack)) {
                cur_text->end = token.start;
                //cout << "6. Appending:'" << string(cur_text->begin, cur_text->end - cur_text->begin) << "'" << endl;
                TAILQ_INSERT_TAIL(&text_list,cur_text,entries);
                cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
              } 
              cur_link = (link_entry_t*) calloc(1,sizeof(link_entry_t));
              cur_link->data = token.stop;
              cur_link->type = INTERNAL;
              TAILQ_INSERT_HEAD(&link_stack,cur_link,entries);
              break;

            case LINK_END: 
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                TAILQ_REMOVE(&link_stack,cur_link,entries);

                if (!cur_link->invalid) {
                  if (!cur_link->done) {
                    cur_link->len = token.start - cur_link->data;
                  }
                  switch(cur_link->type) {
                    case INTERNAL:
                      {
                        char* sanitized_title = sanitize_title(cur_link->data,cur_link->data + cur_link->len);
                        if (sanitized_title && TAILQ_EMPTY(&link_stack)) {
                          char* link_text = NULL;
                          if (cur_link->alias) {
                            link_text = make_str_from_begin_and_end(cur_link->alias,token.start);
                          } else {
                            link_text = strdup(sanitized_title);
                          }
                          if(link_text) {
                            text_block_t* text_block = (text_block_t*) calloc(1,sizeof(text_block_t));
                            char* wiki_link = build_traditional_wiki_link(sanitized_title,link_text);
                            text_block->begin = wiki_link;
                            text_block->end = wiki_link + strlen(wiki_link);
                            text_block->need_to_free = true;
                            TAILQ_INSERT_TAIL(&text_list,text_block,entries);
                          }
                          free(link_text);
                        }
                        free(sanitized_title);
                      }
                      break;
                    default:
                      break;
                  }
                }
                free(cur_link);
                if (TAILQ_EMPTY(&link_stack)) {
                  //cout << "7. Setting Begin to: " << string(token.stop,10) << endl;
                  cur_text->begin = token.stop;
                }
              }
              break;

            case SEPARATOR:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == TEMPLATE) {
                  if (cur_link->key && cur_link->value) {
                    char* value_image = image_match(cur_link->value,token.start); 
                    if (value_image) {
                      free(value_image);
                    }
                  }
                  cur_link->key = token.stop;
                  cur_link->value = NULL;
                } else {
                  cur_link->alias = token.stop;
                  if (!cur_link->done) {
                    cur_link->len = token.start - cur_link->data;
                    cur_link->done = true;
                  }
                }
              }
              break;

            case EQUALS:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == TEMPLATE) {
                  cur_link->key_stop = token.start;
                  cur_link->value = token.stop;
                }           
              }
              break;

            case COLON:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (!cur_link->done) {
                  cur_link->invalid = true;
                }
              }
              break;

            case CRLF:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                if (cur_link->type == INTERNAL && !cur_link->done) {
                  cur_link->invalid = true;
                }
              }

            case HASH:
              if (!TAILQ_EMPTY(&link_stack)) {
                cur_link = TAILQ_FIRST(&link_stack);
                cur_link->anchor = true;
                if (!cur_link->done) {
                  cur_link->len = token.start - cur_link->data;
                  cur_link->done = true;
                }
              }
              break;

            case DISAMBIGUATION:
              break;

            default:
              break;
          }
        }
      }
      scan(&token,&token,NULL,pe);
    }

parse_done:

    cur_text->end = pe;
    //cur_text->duplicated = make_str_from_begin_and_end(cur_text->begin,cur_text->end);
    TAILQ_INSERT_TAIL(&text_list,cur_text,entries);

    // This is there in case there is a bad parse.
    while (!TAILQ_EMPTY(&link_stack)) {
      cur_link = TAILQ_FIRST(&link_stack);
      TAILQ_REMOVE(&link_stack,cur_link,entries);
      free(cur_link);
    }
  }

  char* plaintext = NULL;
  int len = 0;
  TAILQ_FOREACH(cur_text,&text_list,entries) {
    if (cur_text->begin && cur_text->end) {
      len += cur_text->end - cur_text->begin;
      //cout << "Entry: '" << string(cur_text->begin, cur_text->end - cur_text->begin) << "'" << endl;
    }
  }

  plaintext = (char*) malloc(len + 1);
  char* cur = plaintext;
  // Now append the snippets into the final string
  while (!TAILQ_EMPTY(&text_list)) {
    cur_text = TAILQ_FIRST(&text_list);
    TAILQ_REMOVE(&text_list,cur_text,entries);
    if (cur_text->begin && cur_text->end) {
      int block_len = cur_text->end - cur_text->begin;
      memcpy(cur,cur_text->begin,block_len);
      cur += block_len;
    }
    if (cur_text->need_to_free) 
      free(cur_text->begin);
    free(cur_text);
  }

  plaintext[len] = '\0';
  if (redirect) {
    free(plaintext);
    return NULL;
  }
  return plaintext;
}
