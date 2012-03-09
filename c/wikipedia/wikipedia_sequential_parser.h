#ifndef __WIKI_SEQUENTIAL_PARSER__
#define __WIKI_SEQUENTIAL_PARSER__

#include "bsd_queue.h"

typedef enum {
  INFO_BOX = 0,
  TEXT = 1,
  INTERNAL_LINK = 2,
  EXTERNAL_LINK = 4,
  MATH_SECTION = 8,
  HEADING_SECTION = 16,
  TEMPLATE = 32,
  IMAGE = 64,
  REDIRECT_SECTION = 128,
  KEY_VALUE = 256,
  PRE = 512,
  REF_SECTION = 1024,
  SOURCE_SECTION = 2048,
  FORMATTING_SECTION = 4096,
  CATEGORY_LINK_SECTION = 8192,
  LANGUAGE_LINK_SECTION = 16384,
  GALLERY_SECTION = 32768,
  TABLE_SECTION = 65536,
  TABLE_CAPTION_SECTION = 131072,
  TABLE_ROW_SECTION,
  TABLE_CELL_SECTION
} wiki_section_type_t;

typedef struct wiki_section_t {
  wiki_section_type_t type;
  TAILQ_ENTRY(wiki_section_t) entries;
} wiki_section_t;

typedef TAILQ_HEAD(wiki_section_head,wiki_section_t) wiki_section_list_t;

typedef struct {
  wiki_section_t super;
  char* text;
} wiki_text_t;

typedef struct wiki_link_t {
  wiki_section_t super;
  char* alias;
  char* target;
} wiki_link_t;

typedef struct wiki_external_link_t {
  wiki_section_t super;
  char* target;
  wiki_section_list_t sections;
} wiki_external_link_t;

typedef struct wiki_category_link_t {
  wiki_section_t super;
  char* category;
} wiki_category_link_t;

typedef struct wiki_language_link_t {
  wiki_section_t super;
  char* language;
  char* target;
} wiki_language_link_t;

typedef struct wiki_ref_t {
  wiki_section_t super;
  wiki_section_list_t sections;
} wiki_ref_t;

typedef enum {
  BOLD_FORMAT,
  ITALIC_FORMAT,
  BOLD_ITALIC_FORMAT
} wiki_format_type;

typedef struct wiki_formatting_t {
  wiki_section_t super;
  wiki_format_type format;
  wiki_section_list_t sections;
} wiki_formatting_t;

typedef struct wiki_pre_t {
  wiki_section_t super;
  char* text;
} wiki_pre_t;

typedef struct wiki_math_t {
  wiki_section_t super;
  char* text;
} wiki_math_t;

typedef struct wiki_source_code_t {
  wiki_section_t super;
  char* text;
} wiki_source_t;

typedef struct wiki_heading_t {
  wiki_section_t super;
  wiki_section_list_t sections;
} wiki_heading_t;

typedef struct wiki_kv_pair_t {
  wiki_section_t super;
  char* key;
  wiki_section_list_t sections;
  TAILQ_ENTRY(wiki_kv_pair_t) entries;
} wiki_kv_pair_t;

typedef TAILQ_HEAD(wiki_key_value_list_head,wiki_kv_pair_t) wiki_key_value_list_t;

typedef struct {
  wiki_section_t super;
  char* title;
  size_t param_count;
  wiki_key_value_list_t kv_pairs;
} wiki_template_t;

typedef struct {
  wiki_section_t super;
  wiki_link_t* link;
} wiki_redirect_t;

typedef struct wiki_image_t {
  wiki_section_t super;
  char* href;
  wiki_key_value_list_t kv_pairs;
} wiki_image_t;

typedef struct wiki_gallery_t {
  wiki_section_t super;
  char* text;
} wiki_gallery_t;

typedef struct wiki_table_cell_t {
  wiki_section_t super;
  wiki_section_list_t content;
  //wiki_section_list_t style;
  char* style;
  bool header;
  TAILQ_ENTRY(wiki_table_cell_t) entries;
} wiki_table_cell_t;

typedef TAILQ_HEAD(wiki_table_cell_list_head,wiki_table_cell_t) wiki_table_cell_list_t;

typedef struct wiki_row_t {
  wiki_section_t super;
  wiki_table_cell_list_t cells;
  //wiki_section_list_t style;
  char* style;
  TAILQ_ENTRY(wiki_row_t) entries;
} wiki_row_t;

typedef TAILQ_HEAD(wiki_row_list_head, wiki_row_t) wiki_row_list_t;

typedef struct wiki_table_caption_t {
  wiki_section_t super;
  char* style;
  wiki_section_list_t sections;
} wiki_table_caption_t;

typedef struct wiki_table_t {
  wiki_section_t super;
  //wiki_section_list_t style;
  char* style;
  wiki_table_caption_t* caption;
  wiki_row_list_t rows;
} wiki_table_t;

typedef struct {
  wiki_section_list_t sections;
  bool error;
  char* error_str;
  char* error_loc;
} wiki_seq_tree_t;

wiki_seq_tree_t* wiki_seq_parse(const char* content, bool verbose = false);
void free_wiki_seq_tree(wiki_seq_tree_t* tree);

#endif 
