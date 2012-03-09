#ifndef __WIKI_PREPROCESSOR_H__
#define __WIKI_PREPROCESSOR_H__

/* 
 * This should help one remove comments and potentially other things that
 * aren't wiki text that make parsing a nightmare.
 */
char* wiki_preprocess(const char* content);

#endif
