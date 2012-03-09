#ifndef __WIKI_TOKEN__
#define __WIKI_TOKEN__

typedef enum {
  NO_TOKEN,
  END_OF_FILE,
  REDIRECT,
  LINK_BEGIN,
  LINK_END,
  PRINTABLE,
  ALNUM,
  DEFAULT,
  SEPARATOR
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

#endif
