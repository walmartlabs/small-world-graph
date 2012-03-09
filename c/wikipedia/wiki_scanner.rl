#include "wiki_scanner.h"

#include <iostream>

using namespace std;

#define EMIT(t)     do { out->type = t; out->stop = p + 1; out->column_stop += (out->stop - out->start); } while (0)
#define DISTANCE()  (p + 1 - ts)

%%{
  machine wikitext;

  main := |*

    '#redirect'i
    {
      EMIT(REDIRECT);
      fbreak;
    };

    '{{disambig}}'i
    {
      EMIT(DISAMBIGUATION);
      fbreak;
    };

    '{{surname|nocat}}'i
    {
      EMIT(DISAMBIGUATION);
      fbreak;
    };

    '{{given name}}'i
    {
      EMIT(DISAMBIGUATION);
      fbreak;
    };

    '[[Category:Given names]]'i
    {
      EMIT(DISAMBIGUATION);
      fbreak;
    };

    '<nowiki>'i
    {
        EMIT(IGNORE_BEGIN);
        fbreak;
    };

    '</nowiki>'i
    {
        EMIT(IGNORE_END);
        fbreak;
    };

    '<math>'i
    {
        EMIT(IGNORE_BEGIN);
        fbreak;
    };

    '</math>'i
    {
        EMIT(IGNORE_END);
        fbreak;
    };

    '<pre>'i
    {
        EMIT(IGNORE_BEGIN);
        fbreak;
    };

    '</pre>'i
    {
        EMIT(IGNORE_END);
        fbreak;
    };

    '<ref>'i
    {
        EMIT(IGNORE_BEGIN);
        fbreak;
    };

    '<ref'i [^/>]+ '>'
    {
        EMIT(IGNORE_BEGIN);
        fbreak;
    };

    '<ref'i [^/>]+ '/>'
    {
        EMIT(IGNORABLE);
        fbreak;
    };


    '</ref>'i
    {
        EMIT(IGNORE_END);
        fbreak;
    };

    "'"{2,5}
    {
      EMIT(IGNORABLE);
      fbreak;
    };

    "="{2,5}
    {
      EMIT(IGNORABLE);
      fbreak;
    };

    '*'
    {
      EMIT(IGNORABLE);
      fbreak;
    };

    '{{' space* 'current'i
    {
        EMIT(IGNORE_BEGIN);
        fbreak;
    };

    '<!--'
    {
        EMIT(COMMENT_BEGIN);
        fbreak;
    };

    '-->'
    {
        EMIT(COMMENT_END);
        fbreak;
    };


    '[[' space* 'Image:'
    {
      EMIT(IMAGE_LINK_BEGIN);
      fbreak;
    };

    '[['
    {
      EMIT(LINK_BEGIN);
      fbreak;
    };

    ']]'
    {
      EMIT(LINK_END);
      fbreak;
    };

    '|'
    {
      EMIT(SEPARATOR);
      fbreak;
    };

    '{{'
    {
      EMIT(TEMPLATE_BEGIN);
      fbreak;
    };

    '}}'
    {
      EMIT(TEMPLATE_END);
      fbreak;
    };

    '{|'
    {
      EMIT(IGNORE_BEGIN);
      fbreak;
    };

    '|}'
    {
      EMIT(IGNORE_END);
      fbreak;
    };

    ':'
    {
      EMIT(COLON);
      fbreak;
    };

    '#'
    {
      EMIT(HASH);
      fbreak;
    };

    '='
    {
      EMIT(EQUALS);
      fbreak;
    };
        
    ("\r"? "\n") | "\r"
    {
      EMIT(CRLF);
      fbreak;
    };

    
#    [a-zA-Z0-9]+
#    {
#      EMIT(ALNUM);
#      fbreak;
#    };
#
#    (0x24..0x25 | 0x2b | 0x2d | 0x2f | 0x40 | 0x5c | 0x5e..0x5f | 0x7e)+
#    {
#      EMIT(PRINTABLE);
#      fbreak;
#    };

  *|;

  write data;
}%%

const char* token_to_human_readable(wiki_token_t* tok)
{
  switch(tok->type) {
    case NO_TOKEN:
      return "NO_TOKEN";
    case END_OF_FILE:
      return "END_OF_FILE";
    case REDIRECT:
      return "REDIRECT";
    case LINK_BEGIN:
      return "LINK_BEGIN";
    case LINK_END:
      return "LINK_END";
    case TEMPLATE_BEGIN:
      return "TEMPLATE_BEGIN";
    case TEMPLATE_END:
      return "TEMPLATE_END";
    case PRINTABLE:
      return "PRINTABLE";
    case ALNUM:
      return "ALNUM";
    case DEFAULT:
      return "DEFAULT";
    case SEPARATOR:
      return "SEPARATOR";
    case CRLF:
      return "CRLF";
    case COLON:
      return "COLON";
    case IMAGE_LINK_BEGIN:
      return "IMAGE_LINK_BEGIN";
    case DISAMBIGUATION:
      return "DISAMBIGUATION";
    case HASH:
      return "HASH";
    case EQUALS:
      return "EQUALS";
    case IGNORE_BEGIN:
      return "IGNORE_BEGIN";
    case IGNORE_END:
      return "IGNORE_END";
    case IGNORABLE:
      return "IGNORABLE";
    case COMMENT_BEGIN:
      return "COMMENT_BEGIN";
    case COMMENT_END:
      return "COMMENT_END";
    case TABLE_BEGIN:
      return "TABLE_BEGIN";
    case TABLE_END:
      return "TABLE_END";
    default:
      return "UNKNOWN";
  }
}

void scan(wiki_token_t* out,wiki_token_t* last_token, char* p, char* pe)
{
  WikiTokenType last_type = NO_TOKEN;

  if (last_token) {
    last_type = last_token->type;
    if (last_type == NO_TOKEN)
      p = last_token->start + 1;
    else
      p = last_token->stop;
  }   

  out->type       = NO_TOKEN;
  out->code_point = 0;
  out->start      = p;

  if (p == pe) {
    out->stop = p;
    out->type = END_OF_FILE;
    return;
  }
  
  char    *eof = pe;  // required for backtracking (longest match determination)
  int     cs;         // current state (standard Ragel)
  char    *ts;        // token start (scanner)
  char    *te;        // token end (scanner)
  int     act;        // identity of last patterned matched (scanner)

  %% write init;
  %% write exec;
}
