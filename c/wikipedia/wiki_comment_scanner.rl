#include "wiki_comment_scanner.h"

#define EMIT(t)     do { out->type = t; out->stop = p + 1; out->column_stop += (out->stop - out->start); } while (0)
#define DISTANCE()  (p + 1 - ts)

%%{
  machine wikicomments;

main := |*

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

    '{{SERVER}}'
    {
        EMIT(SERVER);
        fbreak;
    };

    '{{SITENAME}}'
    {
        EMIT(SITENAME);
        fbreak;
    };

    '{{PAGENAME}}'
    {
        EMIT(PAGENAME);
        fbreak;
    };


  *|;

  write data;
}%%

void comment_scan(wiki_comment_token_t* out,wiki_comment_token_t* last_token, char* p, char* pe)
{
  WikiCommentTokenType last_type = NO_TOKEN;

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
