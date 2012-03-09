#ifndef __WIKI_SEQUENTIAL_SCANNER__
#define __WIKI_SEQUENTIAL_SCANNER__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
  NO_TOKEN,
  END_OF_FILE,
  REDIRECT,
  LINK_BEGIN,
  LINK_END,
  EXTERNAL_LINK_BEGIN,
  EXTERNAL_LINK_END,
  TEMPLATE_BEGIN,
  TEMPLATE_END,
  PRINTABLE,
  ALNUM,
  DEFAULT,
  SEPARATOR,
  CRLF,
  COLON,
  SPACE,
  IMAGE_LINK_BEGIN,
  HASH,
  EQUALS,
  TABLE_BEGIN,
  TABLE_END,
  NOWIKI_BEGIN,
  NOWIKI_END,
  REF_BEGIN,
  REF_END,
  REF,
  MATH_BEGIN,
  MATH_END,
  TAG_BEGIN,
  TAG_END,
  PRE_BEGIN,
  PRE_END,
  SOURCE_BEGIN,
  SOURCE_END,
  VARIABLE_BEGIN,
  VARIABLE_END,
  HEADING,
  LIST_ELEMENT,
  LANGUAGE_LINK_BEGIN,
  CATEGORY_LINK_BEGIN,
  BR_LINE_BREAK,
  SMALL_BEGIN,
  SMALL_END,
  BIG_BEGIN,
  BIG_END,
  CENTER_BEGIN,
  CENTER_END,
  BOLD_ITALIC,
  BOLD,
  ITALIC,
  DOUBLE_TEMPLATE_BEGIN,
  DOUBLE_TEMPLATE_END,
  GALLERY_BEGIN,
  GALLERY_END,
  DOUBLE_SEPARATOR,
  TABLE_ROW_BEGIN,
  EXCLAMATION,
  DOUBLE_EXCLAMATION,
  TABLE_CAPTION_BEGIN,
  LAST_TOKEN // Don't change this
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
