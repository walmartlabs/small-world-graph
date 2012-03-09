#include <stdlib.h>
#include <string.h>
#include "wiki_preprocessor.h"
#include "wiki_comment_scanner.h"

#include "bsd_queue.h"

typedef struct text_block_t {
  const char* start;
  const char* end;
  bool need_to_free;
  TAILQ_ENTRY(text_block_t) entries;
} text_block_t;

typedef struct preproc_meta_t {
  TAILQ_HEAD(,text_block_t) text_list;
  text_block_t* cur_text;
  int comment_gate;
} preproc_meta_t;

/* Prototypes */
static preproc_meta_t* initialize_preprocessor(const char* content);
static void finalize_preprocessor(preproc_meta_t* meta);
static void parse_comment_begin(preproc_meta_t* meta,wiki_comment_token_t* token);
static void parse_comment_end(preproc_meta_t* meta,wiki_comment_token_t* token);
static void parse_sitename(preproc_meta_t* meta,wiki_comment_token_t* token);
static void parse_server(preproc_meta_t* meta,wiki_comment_token_t* token);
static void parse_pagename(preproc_meta_t* meta,wiki_comment_token_t* token);
static void append_text_block(preproc_meta_t* meta,wiki_comment_token_t* token,const char* content);
static char* concatenate_and_free_text_blocks(preproc_meta_t* meta);

char* wiki_preprocess(const char* content)
{
  preproc_meta_t* meta = initialize_preprocessor(content);

  wiki_comment_token_t token;
  char* p = (char*) content;
  char* pe = p + strlen(content);
  comment_scan(&token,NULL,p,pe);
  while(token.type != END_OF_FILE) {
    switch(token.type) {
      case COMMENT_BEGIN:
        parse_comment_begin(meta,&token);
        break;
      case COMMENT_END:
        parse_comment_end(meta,&token);
        break;
      case SITENAME:
        parse_sitename(meta,&token);
        break;
      case SERVER:
        parse_server(meta,&token);
        break;
      case PAGENAME:
        parse_pagename(meta,&token);
        break;
      default:
        break;
    }
    comment_scan(&token,&token,NULL,pe);
  }

  meta->cur_text->end = pe;
  TAILQ_INSERT_TAIL(&meta->text_list,meta->cur_text,entries);


  // Now concatenate all the text blocks into one string
  char* transformed = concatenate_and_free_text_blocks(meta);
  finalize_preprocessor(meta);
  return transformed;
}

/************************
 * Private Implementation
 ************************/

static preproc_meta_t* initialize_preprocessor(const char* text)
{
  preproc_meta_t* meta = (preproc_meta_t*) calloc(1,sizeof(preproc_meta_t));
	TAILQ_INIT(&meta->text_list);
  meta->cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
  meta->cur_text->start = text;
  return meta;
}

static void finalize_preprocessor(preproc_meta_t* meta)
{
  free(meta);
}

static void parse_comment_begin(preproc_meta_t* meta,wiki_comment_token_t* token)
{
  meta->comment_gate++;
  meta->cur_text->end = token->start;
  TAILQ_INSERT_TAIL(&meta->text_list,meta->cur_text,entries);
  meta->cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
}

static void parse_comment_end(preproc_meta_t* meta,wiki_comment_token_t* token)
{
  if (meta->comment_gate > 0) {
    meta->comment_gate--;
    meta->cur_text->start = token->stop;
  }
}

/*
 * Replace {{SITENAME}} with Wikipedia
 */
static void parse_sitename(preproc_meta_t* meta,wiki_comment_token_t* token)
{
  if (meta->comment_gate == 0) {
    append_text_block(meta,token,"Wikipedia");
  }
}

static void append_text_block(preproc_meta_t* meta,wiki_comment_token_t* token,const char* content)
{
  meta->cur_text->end = token->start;
  TAILQ_INSERT_TAIL(&meta->text_list,meta->cur_text,entries);
  meta->cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
  meta->cur_text->start = strdup(content);
  meta->cur_text->end = meta->cur_text->start + strlen(meta->cur_text->start);
  meta->cur_text->need_to_free = true;
  TAILQ_INSERT_TAIL(&meta->text_list,meta->cur_text,entries);
  meta->cur_text = (text_block_t*) calloc(1,sizeof(text_block_t));
  meta->cur_text->start = token->stop;
}

/*
 * Iterate through the text blocks and concatenate them into the final
 * preprocessed string.  Free the text blocks as you iterate to save on
 * iteration.  If a text block is marked as needing to free the string, free
 * the string accordingly.
 */
static char* concatenate_and_free_text_blocks(preproc_meta_t* meta)
{
  char* transformed = NULL;
  int len = 0;
  text_block_t* cur_text = NULL;
  TAILQ_FOREACH(cur_text,&meta->text_list,entries) {
    if (cur_text->start && cur_text->end) {
      len += cur_text->end - cur_text->start;
    }
  }

  transformed = (char*) malloc(len + 1);
  char* cur = transformed;

  // Now append the snippets into the final string
  while (!TAILQ_EMPTY(&meta->text_list)) {
    cur_text = TAILQ_FIRST(&meta->text_list);
    TAILQ_REMOVE(&meta->text_list,cur_text,entries);
    if (cur_text->start && cur_text->end) {
      int block_len = cur_text->end - cur_text->start;
      memcpy(cur,cur_text->start,block_len);
      cur += block_len;
    }
    if (cur_text->need_to_free)
      free((char*) cur_text->start);
    free(cur_text);
  }

  transformed[len] = '\0';
  return transformed;
}

static void parse_server(preproc_meta_t* meta,wiki_comment_token_t* token)
{
  if (meta->comment_gate == 0) {
    append_text_block(meta,token,"http://en.wikipedia.org");
  }
}

static void parse_pagename(preproc_meta_t* meta,wiki_comment_token_t* token)
{
  if (meta->comment_gate == 0) {
    append_text_block(meta,token,"PAGENAME");
  }
}
