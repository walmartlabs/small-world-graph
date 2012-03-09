#include "wiki_infobox_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* key;
static char* value;

%%{ 
  machine infobox;
  write data;
  action start_key {
    key = (char*) fpc;
    printf("Starting Key:'%s'\n",key);
    //fnext;
  }
  action end_key {
    //fpc = '\0';
    printf("End Key:'%s'\n",key);
    //fnext;
  }

  action start_value {
    value = (char*) fpc;
    printf("Starting Value:'%s'\n",value);
  }

  action end_value {
    printf("End Value:'%s'\n",value);
  }

  action finish {
    printf("Finish:'%s'\n",fpc);
    fbreak;
  }
}%%

char** retrieve_images_from_infobox(const char* p, const char* pe)
{
  int cs;
  %%{
    key = ([a-zA-Z_]+ >start_key %end_key);
    value = ([^ ][^\n]+ >start_value %end_value) ;

    key_value_pair = '|' space* key ' '* '=' ' '* value;

    main := '{{' [^|]+ key_value_pair* '}}'
    0 @finish;

    write init;
    write exec noend;
  }%%
  return NULL;
}

%%{ 
  machine image_matcher;
  write data;
  action start_image {
    p_image_start = (char*) fpc;
  }
  action end_image {
    int image_length = fpc-p_image_start;
    new_image = (char*) malloc(image_length + 1);
    memcpy(new_image,p_image_start,fpc - p_image_start);
    new_image[image_length] = '\0';
    fbreak;
  }
}%%

char* image_match(const char* p, const char* pe)
{
  int cs;
  char* new_image = NULL;
  char* p_image_start = NULL;
  const char* eof = pe;
  %%{
    image_name = ([^ \[\{] print+ '.' ('jpg'i|'jpeg'i|'svg'i|'gif'i|'png'i)) >start_image %end_image;
    main := space* image_name any*;

    write init;
    write exec;
  }%%

  return new_image;
}

char* sanitize_title(const char* p, const char* pe)
{
  char* new_title = NULL;
  char* p_title_start = NULL;
  char* p_title_end = NULL;
  int len = pe - p;
  int i=0;
  for(i=0; i < len; i++) {
    if (!(p[i] == ' ' || p[i] == '\r' || p[i] == '\n' || p[i] == '\t') || p[i] == '\v' || p[i] == '\f') {
      p_title_start = (char*) p + i;
      break;
    }
  }
  if (p_title_start) {
    for(i=len - 1; i >= 0; i--) {
      if (!(p[i] == ' ' || p[i] == '\r' || p[i] == '\n' || p[i] == '\t') || p[i] == '\v' || p[i] == '\f') {
        p_title_end = (char*) p + i + 1;
        break;
      }
    }
  }

  if (p_title_start && p_title_end) {
    int new_title_len = p_title_end - p_title_start;
    new_title = (char*) malloc(new_title_len + 1);
    memcpy(new_title,p_title_start,new_title_len);
    new_title[new_title_len] = '\0';

    // now go through and do the gsub
    for(i=0; i < new_title_len; i++) {
      if(new_title[i] == '\r' || new_title[i] == '\t' || new_title[i] == '_' || new_title[i] == '\n')
        new_title[i] = ' ';
    }
  } 
  return new_title;
}
