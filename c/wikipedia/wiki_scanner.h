#ifndef __WIKI_SCANNER__
#define __WIKI_SCANNER__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
  NO_TOKEN,
  END_OF_FILE,
  REDIRECT,
  LINK_BEGIN,
  LINK_END,
  TEMPLATE_BEGIN,
  TEMPLATE_END,
  PRINTABLE,
  ALNUM,
  DEFAULT,
  SEPARATOR,
  CRLF,
  COLON,
  IMAGE_LINK_BEGIN,
  DISAMBIGUATION,
  HASH,
  EQUALS,
  IGNORE_BEGIN,
  IGNORE_END,
  IGNORABLE,
  COMMENT_BEGIN,
  COMMENT_END,
  TABLE_BEGIN,
  TABLE_END
} WikiTokenType;

typedef struct
{
    char        *start;
    char        *stop;
    size_t      line_start;
    size_t      line_stop;
    size_t      column_start;
    size_t      column_stop;
    uint32_t    code_point;
    WikiTokenType         type;
} wiki_token_t;

/* Call this method to just print out useful information about the token */
const char* token_to_human_readable(wiki_token_t* tok);
void scan(wiki_token_t*, wiki_token_t*, char*, char*);

#endif
