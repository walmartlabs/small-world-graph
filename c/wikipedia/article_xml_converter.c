#line 1 "article_xml_converter.rl"
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <expat.h>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace boost;

/* Typedefs */

typedef struct {
  char* cdata;
  size_t cdata_len;
  int in_title;
  char* title;
  int in_text;
  char* text;
} xml_progress_t;

static void XMLCALL load_start(void *user_data, const char *name, const char **attr);
static void XMLCALL load_end(void *user_data, const char *name);
static void XMLCALL character_data_handler(void* user_data, const XML_Char *s, int len);
static void parse_outlinks(xml_progress_t*);

/* Globlals */
static const regex redirect_rx("^#REDIRECT\\s+\\[\\[(.*?)\\]\\]",boost::regex::perl);
static ofstream output("/tmp/titles");

int main(int argc,char** argv)
{
  if (argc < 2) {
    cerr << "Please specify an articles.xml" << endl;
    exit(-1);
  }

  char buf[BUFSIZ];
  FILE* xml_file = NULL;
  xml_progress_t buffer = {0};
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, &buffer);
  XML_SetElementHandler(parser, load_start, load_end);
  XML_SetCharacterDataHandler(parser,character_data_handler);
  int done = 0;

  xml_file = fopen(argv[1],"r");
  if (xml_file) {
    do {
      size_t len = fread(buf, 1, sizeof(buf),xml_file);
      done = len < sizeof(buf);
      if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
        fprintf(stderr, "%s at line %lu\n", XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
      }
    } while (!done);
  } else {
    printf("Error opening file: %s\n",strerror(errno));
    fprintf(stderr,"Cannot find file\n");
  }
  XML_ParserFree(parser);
}

/* Static Private Implementation */

static void XMLCALL
load_start(void *user_data, const char *name, const char **attr)
{
  xml_progress_t* prog = (xml_progress_t*) user_data;
  if (!strcmp(name,"title")) {
    prog->in_title = 1;
  } else if (!strcmp(name,"text")) {
    prog->in_text = 1;
  }
}

static void XMLCALL
load_end(void *user_data, const char *name)
{
  xml_progress_t* prog = (xml_progress_t*) user_data;
  if (!strcmp(name,"title")) {
    prog->title = prog->cdata;
    prog->in_title = 0;
  } else if (!strcmp(name,"text")) {
    prog->text = prog->cdata;
    prog->in_text = 0;
  } else if (!strcmp(name,"page")) {
    parse_outlinks(prog);
    free(prog->title);
    free(prog->text);
  }
  prog->cdata = NULL;
}

static void XMLCALL
character_data_handler(void* user_data, const XML_Char *s, int len)
{
  xml_progress_t* prog = (xml_progress_t*) user_data;
  if (len > 0) {
    if (prog->in_title || prog->in_text) {
      if (prog->cdata) {
        size_t new_cdata_len = prog->cdata_len + len;
        prog->cdata = (XML_Char*) realloc(prog->cdata,sizeof(XML_Char) * (new_cdata_len + 1));
        memcpy(prog->cdata + prog->cdata_len,s,len);
        prog->cdata[new_cdata_len] = '\0';
        prog->cdata_len = new_cdata_len;
      } else {
        prog->cdata_len = len;
        prog->cdata = (XML_Char*) malloc(sizeof(XML_Char) * (len + 1));
        memcpy(prog->cdata,s,len);
        prog->cdata[len] = '\0';
      }
    }
  }
}


#line 126 "article_xml_converter.c"
static const char _wikitext_actions[] = {
	0, 1, 0, 1, 1, 1, 2
};

static const char _wikitext_key_offsets[] = {
	0, 0, 1, 2, 3, 4, 5, 6, 
	7
};

static const char _wikitext_trans_keys[] = {
	69, 68, 73, 82, 69, 67, 84, 82, 
	0
};

static const char _wikitext_single_lengths[] = {
	0, 1, 1, 1, 1, 1, 1, 1, 
	1
};

static const char _wikitext_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const char _wikitext_index_offsets[] = {
	0, 0, 2, 4, 6, 8, 10, 12, 
	14
};

static const char _wikitext_trans_targs[] = {
	2, 0, 3, 0, 4, 0, 5, 0, 
	6, 0, 7, 0, 8, 0, 1, 0, 
	0
};

static const char _wikitext_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 5, 0, 0, 0, 
	0
};

static const char _wikitext_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	1
};

static const char _wikitext_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	3
};

static const int wikitext_start = 8;
static const int wikitext_first_final = 8;
static const int wikitext_error = 0;

static const int wikitext_en_main = 8;

#line 136 "article_xml_converter.rl"


/* Here we search for outlinks and output them */
static void parse_outlinks(xml_progress_t* prog)
{
  char* p = prog->text;
  char* pe = p + prog->cdata_len;
  char    *eof = pe;  // required for backtracking (longest match determination)

  
#line 195 "article_xml_converter.c"
	{
	cs = wikitext_start;
	ts = 0;
	te = 0;
	act = 0;
	}
#line 146 "article_xml_converter.rl"
  
#line 204 "article_xml_converter.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_acts = _wikitext_actions + _wikitext_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 1:
#line 1 "article_xml_converter.rl"
	{ts = p;}
	break;
#line 225 "article_xml_converter.c"
		}
	}

	_keys = _wikitext_trans_keys + _wikitext_key_offsets[cs];
	_trans = _wikitext_index_offsets[cs];

	_klen = _wikitext_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _wikitext_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	cs = _wikitext_trans_targs[_trans];

	if ( _wikitext_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _wikitext_actions + _wikitext_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 2:
#line 129 "article_xml_converter.rl"
	{te = p+1;{
      printf("Redirect Found\n");
    }}
	break;
#line 295 "article_xml_converter.c"
		}
	}

_again:
	_acts = _wikitext_actions + _wikitext_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 0:
#line 1 "article_xml_converter.rl"
	{ts = 0;}
	break;
#line 308 "article_xml_converter.c"
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}
#line 147 "article_xml_converter.rl"
}
