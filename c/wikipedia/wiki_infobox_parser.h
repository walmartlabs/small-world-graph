#ifndef __WIKI_INFOBOX_PARSER__
#define __WIKI_INFOBOX_PARSER__

#ifdef __cplusplus
extern "C" {
#endif

char** retrieve_images_from_infobox(const char* start, const char* end);

/* If an image exists in the value part of a infobox key value, it will return
 * back that image in a new NULL terminated string */
char* image_match(const char* start, const char* end);

/* 
 * First we want to strip leading and trailing whitespace, then convert
 * underscores to spaces to normalize.  Returns NULL if it is a useless title
 */
char* sanitize_title(const char* title, const char* end);

#ifdef __cplusplus
}
#endif

#endif
