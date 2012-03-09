#ifndef __WIKI_TITLE_VALIDATOR__
#define __WIKI_TITLE_VALIDATOR__

#ifdef __cplusplus
extern "C" {
#endif

/* Here we return true/false depending on whether a title is useful to us.
 * For exampl, we don't care about
 * - Templates
 * - Dates
 */
bool useful_title(const char* start, const char* end);

#ifdef __cplusplus
}
#endif

#endif
