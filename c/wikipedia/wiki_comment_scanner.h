#ifndef __WIKI_COMMENT_SCANNER__
#define __WIKI_COMMENT_SCANNER__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
  NO_TOKEN,
  END_OF_FILE,
  COMMENT_BEGIN,
  COMMENT_END,
  SERVER,
  SITENAME,
  PAGENAME
} WikiCommentTokenType;

typedef struct
{
    char        *start;
    char        *stop;
    size_t      line_start;
    size_t      line_stop;
    size_t      column_start;
    size_t      column_stop;
    uint32_t    code_point;
    WikiCommentTokenType         type;
} wiki_comment_token_t;

void comment_scan(wiki_comment_token_t*, wiki_comment_token_t*, char*, char*);

#endif
