#include "wiki_sequential_scanner.h"

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

    '<nowiki>'i
    {
        EMIT(NOWIKI_BEGIN);
        fbreak;
    };

    '</nowiki>'i
    {
        EMIT(NOWIKI_END);
        fbreak;
    };

    '<math>'i
    {
        EMIT(MATH_BEGIN);
        fbreak;
    };

    '</math>'i
    {
        EMIT(MATH_END);
        fbreak;
    };

    '<pre>'i
    {
        EMIT(PRE_BEGIN);
        fbreak;
    };

    '</pre>'i
    {
        EMIT(PRE_END);
        fbreak;
    };

    '<ref>'i
    {
        EMIT(REF_BEGIN);
        fbreak;
    };

    '<ref'i [^/>]+ '>'
    {
        EMIT(REF_BEGIN);
        fbreak;
    };

    '<ref'i [^/>]+ '/>'
    {
        EMIT(REF);
        fbreak;
    };


    '</ref>'i
    {
        EMIT(REF_END);
        fbreak;
    };

    
    '<source>'i
    {
        EMIT(SOURCE_BEGIN);
        fbreak;
    };

    '<source'i [^/>]+ '>'
    {
        EMIT(SOURCE_BEGIN);
        fbreak;
    };

    '</source>'i
    {
        EMIT(SOURCE_END);
        fbreak;
    };


    "="{2,5}
    {
      EMIT(HEADING);
      fbreak;
    };

    "*"{1,5}
    {
      EMIT(LIST_ELEMENT);
      fbreak;
    };

    '[[' [\t ]* ('Image:'i | 'File:'i)
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

    '[' ('http://' | 'ftp://' | 'https://')
    {
      EMIT(EXTERNAL_LINK_BEGIN);
      fbreak;
    };

    '[[' [\t ]* ( "aa" | "ab" | "ace" | "af" | "ak" | "als" | "am" | "an" | "ang" | "ar" | "arc" | "arz" | "as" | "ast" | "av" | "aru" | "az" | "ba" | "bar" | "bat-smg" | "bcl" | "be" | "be-x-old" | "bg" | "bh" | "bi" | "bm" | "bn" | "bo" | "bpy" | "br" | "bs" | "bug" | "bxr" | "ca" | "cbk-zam" | "cdo" | "ce" | "ceb" | "ch" | "cho" | "chr" | "chy" | "ckb" | "closed-zh-tw" | "co" | "cr" | "crh" | "cs" | "csb" | "cu" | "cv" | "cy" | "cz" | "da" | "de" | "diq" | "da" | "dk" | "dsb" | "dv" | "dz" | "ee" | "el" | "eml" | "en" | "eo" | "epo" | "es" | "et" | "eu" | "ext" | "fa" | "ff" | "fi" | "fiu-vro" | "fj" | "fo" | "fr" | "frp" | "fur" | "fy" | "ga" | "gan" | "gd" | "gl" | "glk" | "gn" | "got" | "gu" | "gv" | "ha" | "hak" | "haw" | "he" | "hi" | "hif" | "ho" | "hr" | "hsb" | "ht" | "hu" | "hy" | "hz" | "ia" | "id" | "ie" | "ig" | "ii" | "ik" | "ilo" | "io" | "is" | "it" | "iu" | "ja" | "jbo" | "jp" | "jv" | "ka" | "kaa" | "kab" | "kg" | "ki" | "kj" | "kk" | "kl" | "km" | "kn" | "ko" | "kr" | "ks" | "ksh" | "ku" | "kv" | "kw" | "ky" | "la" | "lad" | "lb" | "lbe" | "lg" | "li" | "lij" | "lmo" | "ln" | "lo" | "lt" | "lv" | "map-bms" | "mdf" | "mg" | "mh" | "mhr" | "mi" | "minnan" | "mk" | "ml" | "mn" | "mo" | "mr" | "ms" | "mt" | "mus" | "mwl" | "my" | "myv" | "mzn" | "na" | "nah" | "nan" | "nap" | "nb" | "nds" | "nds-nl" | "ne" | "new" | "ng" | "nl" | "nn" | "no" | "nomcom" | "nov" | "nrm" | "nv" | "bizaad" | "ny" | "oc" | "om" | "or" | "os" | "pa" | "pag" | "pam" | "pap" | "pdc" | "pi" | "pih" | "pl" | "pms" | "pnb" | "pnt" | "ps" | "pt" | "qu" | "rm" | "rmy" | "rn" | "ro" | "roa-rup" | "roa-tara" | "ru" | "rw" | "sa" | "sah" | "sc" | "scn" | "sco" | "sd" | "se" | "sg" | "sh" | "si" | "simple" | "sk" | "sl" | "sm" | "sn" | "so" | "sq" | "sr" | "srn" | "ss" | "st" | "stq" | "su" | "sv" | "sw" | "szl" | "ta" | "te" | "tet" | "tg" | "th" | "ti" | "tk" | "tl" | "tn" | "to" | "tokipona" | "tp" | "tpi" | "tr" | "ts" | "tt" | "tum" | "tw" | "ty" | "udm" | "ug" | "uk" | "ur" | "uz" | "ve" | "vec" | "vi" | "vls" | "vo" | "wa" | "war" | "wo" | "wuu" | "xal" | "xh" | "yi" | "yo" | "za" | "zea" | "zh" | "zh-cfr" | "zh-classical" | "zh-min-nan" | "zh-yue" | "zu" ) [\t ]* ":" [\t ]*
    {
      EMIT(LANGUAGE_LINK_BEGIN);
      fbreak;
    };

    '[[' [\t ]* 'category'i [\t ]* ':' [\t ]*
    {
      EMIT(CATEGORY_LINK_BEGIN);
      fbreak;
    };

    ']'
    {
      EMIT(EXTERNAL_LINK_END);
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

    [ ]
    {
      EMIT(SPACE);
      fbreak;
    };

    '|'? '}}'
    {
      EMIT(TEMPLATE_END);
      fbreak;
    };

    '{{{'
    {
      EMIT(VARIABLE_BEGIN);
      fbreak;
    };

    '|'? '}}}'
    {
      EMIT(VARIABLE_END);
      fbreak;
    };

    '{{{{'
    {
      EMIT(DOUBLE_TEMPLATE_BEGIN);
      fbreak;
    };


    '}}}}'
    {
      EMIT(DOUBLE_TEMPLATE_END);
      fbreak;
    };

    '{|'
    {
      EMIT(TABLE_BEGIN);
      fbreak;
    };

    '|}'
    {
      EMIT(TABLE_END);
      fbreak;
    };

    ':'
    {
      EMIT(COLON);
      fbreak;
    };

    '#'{1,3}
    {
      EMIT(LIST_ELEMENT);
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

    '<br'i [\t ]* '/>'
    {
        EMIT(BR_LINE_BREAK);
        fbreak;
    };

    '<center>'i
    {
        EMIT(CENTER_BEGIN);
        fbreak;
    };

    '</center>'i
    {
        EMIT(CENTER_END);
        fbreak;
    };

    "''"
    {
      EMIT(ITALIC);
      fbreak;
    };

    "'''"
    {
      EMIT(BOLD);
      fbreak;
    };

    "'''''"
    {
      EMIT(BOLD_ITALIC);
      fbreak;
    };

    '<small>'i
    {
      EMIT(SMALL_BEGIN);
      fbreak;
    };

    '</small>'i
    {
      EMIT(SMALL_END);
      fbreak;
    };

    '<big>'i
    {
      EMIT(BIG_BEGIN);
      fbreak;
    };

    '</big>'i
    {
      EMIT(BIG_END);
      fbreak;
    };

    '<gallery>'i
    {
      EMIT(GALLERY_BEGIN);
      fbreak;
    };

    '</gallery>'i
    {
      EMIT(GALLERY_END);
      fbreak;
    };

    '|+'
    {
      EMIT(TABLE_CAPTION_BEGIN);
      fbreak;
    };

    '||'
    {
      EMIT(DOUBLE_SEPARATOR);
      fbreak;
    };

    '|-'
    {
      EMIT(TABLE_ROW_BEGIN);
      fbreak;
    };

    '!'
    {
      EMIT(EXCLAMATION);
      fbreak;
    };

    '!!'
    {
      EMIT(DOUBLE_EXCLAMATION);
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
    case HASH:
      return "HASH";
    case EQUALS:
      return "EQUALS";
    case TABLE_BEGIN:
      return "TABLE_BEGIN";
    case TABLE_END:
      return "TABLE_END";
    case NOWIKI_BEGIN:
      return "NOWIKI_BEGIN";
    case NOWIKI_END:
      return "NOWIKI_END";
    case LIST_ELEMENT:
      return "LIST_ELEMENT";
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
