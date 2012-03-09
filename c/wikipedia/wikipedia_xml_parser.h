#ifndef __WIKIPEDIA_XML_PARSER__
#define __WIKIPEDIA_XML_PARSER__

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * The title and text that are passed to the fn_ptr are the fn_ptr's to deal
 * with.  They will not be freed. The traveler variable is just data you want to go with the callback
 */
void wikipedia_xml_parse(const char* filename,void(* fn_ptr)(char* title,char* text, void* traveler),void* traveler);

#ifdef __cplusplus
}
#endif

#endif
