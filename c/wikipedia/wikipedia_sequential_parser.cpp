#include <string.h>
#include "bsd_queue.h"
#include "wikipedia_sequential_parser.h"
#include "wiki_sequential_scanner.h"
#include "wiki_preprocessor.h"
#include <string>
#include <ctype.h>

using namespace std;

typedef enum {
  IMAGE_CTX,
  INTERNAL_LINK_CTX,
  EXTERNAL_LINK_CTX,
  TEMPLATE_CTX,
  PARAMETER_CTX,
  VALUE_CTX,
  EXTERNAL_LINK_VALUE_CTX,
  IGNORE_CTX,
  ROOT_CTX,
  NOWIKI_CTX,
  PRE_CTX,
  REF_CTX,
  MATH_CTX,
  HEADING_CTX,
  FORMATTING_CTX,
  CATEGORY_LINK_CTX,
  LANGUAGE_LINK_CTX,
  INTERNAL_LINK_ANCHOR_TEXT_CTX,
  GALLERY_CTX,
  TABLE_CTX,
  TABLE_ROW_CTX,
  TABLE_CAPTION_CTX,
  TABLE_CELL_CTX,
  SOURCE_CTX,
} ctx_type_t;

typedef struct ctx_t {
  ctx_type_t type;
  char* data;
  int len;
  char* text_start;
  char* text_stop;

  char* tok_start;
  char* tok_stop;

  char* key_start;
  char* key_stop;
  char* value_start;
  char* alias_start;
  char* alias_stop;
  char* target_start;
  char* target_stop;
  char* title_start;
  char* title_stop;

  // Renamed link is Wikiese for [[Hello|renamed]] style links
  bool renamed_link;
  bool ignorable;
  bool nonspace_encountered;

  bool newline_encountered;

  size_t param_count;

  // section will be freed upon garbage collection of a bad parse in orphaned
  // contexts
  wiki_section_t* section;
  wiki_section_list_t* section_list;
  wiki_key_value_list_t* kv_pairs;

  // This is used as a pointer that can be used to pass data but shouldn't be
  // freed if this context is garbage collected on a bad parse
  void* weak_ref;
  wiki_section_t* weak_section;

  // Fields necessary for parsing tables
  wiki_table_t* cur_table;
  wiki_row_t* cur_row;
  wiki_table_cell_t* cur_cell;

  TAILQ_ENTRY(ctx_t) entries;
} ctx_t;

typedef struct parser_meta_t {
  TAILQ_HEAD(,ctx_t) ctx_stack;
  ctx_t* ctx;
  char* raw_content;
  char* preprocessed_content;
  bool redirect;
  bool verbose;
  wiki_seq_tree_t* tree;
} parser_meta_t;

/* Prototypes */
static wiki_seq_tree_t* initialize_tree();
static parser_meta_t* initialize_parser(wiki_seq_tree_t* tree,const char* content, bool verbose);
static char* make_str_from_begin_and_end(char* begin, char* end);
static char* make_stripped_str_from_begin_and_end(char* begin, char* end);
static void append_text_section(parser_meta_t* meta, char* start, char* stop);
static void append_link_section(wiki_section_list_t* head,wiki_link_t* link);
static void append_redirect_section(wiki_section_list_t* head,wiki_link_t* destination);
static void append_template_section(parser_meta_t* meta ,wiki_template_t* tpl);
static void append_section(parser_meta_t* meta, wiki_section_t* sec);
static wiki_section_t* build_text_section();
static wiki_section_t* build_heading_section();
static wiki_section_t* build_template_section();
static wiki_section_t* build_image_section();
static wiki_section_t* build_key_value_section();
static wiki_section_t* build_pre_section();
static wiki_section_t* build_gallery_section();
static wiki_section_t* build_ref_section();
static wiki_section_t* build_math_section();
static wiki_section_t* build_source_section();
static wiki_section_t* build_external_link_section();
static wiki_section_t* build_category_link();
static wiki_section_t* build_language_link();
static wiki_section_t* build_table_section();
static wiki_section_t* build_table_caption();
static wiki_section_t* build_table_row();
static wiki_section_t* build_table_cell();
static void parse_heading(parser_meta_t* meta,wiki_token_t* tok);
static void parse_formatting(parser_meta_t* meta,wiki_token_t* tok);
static void parse_separator_in_internal_link(parser_meta_t* meta,wiki_token_t* tok);
static void parse_separator_in_parameter(parser_meta_t* meta, wiki_token_t* tok);
static void parse_separator_in_template(parser_meta_t* meta,wiki_token_t* tok);
static void parse_separator_in_value(parser_meta_t* meta,wiki_token_t* tok);
static void parse_separator_in_image(parser_meta_t* meta, wiki_token_t* tok);
static void parse_separator_in_table_caption(parser_meta_t* meta, wiki_token_t* tok);
static void parse_separator_in_table_row(parser_meta_t* meta, wiki_token_t* tok);
static void parse_separator_in_table_cell(parser_meta_t* meta, wiki_token_t* tok);
static void parse_link_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_link_end(parser_meta_t* meta, wiki_token_t* tok);
static void parse_image_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_end_of_file(parser_meta_t* meta, wiki_token_t* tok);
static void parse_template_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_template_end(parser_meta_t* meta, wiki_token_t* tok);
static void parse_template_end_in_value(parser_meta_t* meta, wiki_token_t* tok);
static void parse_template_end_in_parameter(parser_meta_t* meta, wiki_token_t* tok);
static void parse_template_end_in_template(parser_meta_t* meta, wiki_token_t* tok);
static void parse_separator(parser_meta_t* meta, wiki_token_t* tok);
static void parse_equals(parser_meta_t* meta, wiki_token_t* tok);
static void parse_equals_in_parameter(parser_meta_t* meta, wiki_token_t* tok);
static void parse_link_end_in_internal_link(parser_meta_t* meta,wiki_token_t* tok);
static void parse_link_end_in_internal_link_anchor_text(parser_meta_t* meta,wiki_token_t* tok);
static void parse_link_end_in_image(parser_meta_t* meta,wiki_token_t* tok);
static void parse_link_end_in_parameter(parser_meta_t* meta,wiki_token_t* tok);
static void parse_link_end_in_value(parser_meta_t* meta,wiki_token_t* tok);
static void parse_link_end_in_category(parser_meta_t* meta,wiki_token_t* tok);
static void parse_link_end_in_language_link(parser_meta_t* meta,wiki_token_t* tok);
static void parse_double_template_end(parser_meta_t* meta, wiki_token_t* tok);
static void parse_nowiki_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_nowiki_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_gallery_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_gallery_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_pre_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_pre_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_ref(parser_meta_t* meta, wiki_token_t* tok);
static void parse_ref_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_ref_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_math_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_math_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_source_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_source_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_external_link_begin(parser_meta_t* meta,wiki_token_t* tok);
static void parse_external_link_end(parser_meta_t* meta,wiki_token_t* tok);
static void parse_external_link_end_in_external_link(parser_meta_t* meta,wiki_token_t* tok);
static void parse_external_link_end_in_external_link_value(parser_meta_t* meta,wiki_token_t* tok);
static void parse_space(parser_meta_t* meta,wiki_token_t* tok);
static void parse_space_in_external_link(parser_meta_t* meta,wiki_token_t* tok);
static void parse_newline(parser_meta_t* meta, wiki_token_t* tok);
static void parse_newline_in_table(parser_meta_t* meta, wiki_token_t* tok);
static void parse_newline_in_table_row(parser_meta_t* meta, wiki_token_t* tok);
static void parse_newline_in_internal_link(parser_meta_t* meta, wiki_token_t* tok);
static void parse_category_link_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_language_link_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_end(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_end_in_table(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_end_in_table_row(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_end_in_table_cell(parser_meta_t* meta, wiki_token_t* tok);
static void parse_double_separator(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_caption_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_exclamation(parser_meta_t* meta, wiki_token_t* tok);
static void parse_double_exclamation(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_row_begin(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_row_begin_in_caption(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_row_begin_in_table(parser_meta_t* meta, wiki_token_t* tok);
static void parse_table_row_begin_in_table_cell(parser_meta_t* meta, wiki_token_t* tok);
static char* next_numbered_parameter_key(ctx_t* ctx);
static bool is_text_container_context(ctx_t* ctx);
static bool is_trim_whitespace_context(ctx_t* ctx);
static bool has_only_leading_whitespace_in_ctx(ctx_t* ctx, wiki_token_t* tok);
static char* retrieve_language_from_token(wiki_token_t* tok);
static char* context_name_to_string(ctx_type_t ctx_type);

static char* generate_href(ctx_t* ctx,wiki_token_t* tok);
static void cleanup_parser(parser_meta_t* meta);
static ctx_t* push_ctx_stack(parser_meta_t* meta,ctx_type_t type,wiki_section_t* sec, wiki_section_list_t* sec_list, wiki_token_t* tok);
static void pop_ctx_stack(parser_meta_t* meta);
static void set_error(parser_meta_t* meta,wiki_token_t* tok,const char* txt);

/* Free Prototypes */
static void free_section(wiki_section_t* sec);
static void free_text(wiki_text_t* txt);
static void free_link(wiki_link_t* link);
static void free_external_link(wiki_external_link_t* link);
static void free_image(wiki_image_t* img);
static void free_redirect(wiki_redirect_t* red);
static void free_template(wiki_template_t* tpl);
static void free_key_value_pair(wiki_kv_pair_t* kv);
static void free_pre(wiki_pre_t* sec);
static void free_gallery(wiki_gallery_t* sec);
static void free_ref(wiki_ref_t* sec);
static void free_math(wiki_math_t* math);
static void free_source(wiki_source_t* source);
static void free_heading(wiki_heading_t* heading);
static void free_formatting(wiki_formatting_t* formatting);
static void free_category_link(wiki_category_link_t* cat);
static void free_language_link(wiki_language_link_t* lang);
static void free_table(wiki_table_t* table);
static void free_table_caption(wiki_table_caption_t* cap);
static void free_table_row(wiki_row_t* row);
static void free_table_cell(wiki_table_cell_t* cell);

static inline int min(int x, int y);

void free_wiki_seq_tree(wiki_seq_tree_t* tree)
{
  if (tree) {
    wiki_section_t* sec = NULL;
    while((sec = TAILQ_FIRST(&tree->sections))) {
      TAILQ_REMOVE(&tree->sections,sec,entries);
      free_section(sec);
    }
  }
  if (tree->error_loc) 
    free(tree->error_loc);
  free(tree);
}

wiki_seq_tree_t* wiki_seq_parse(const char* content, bool verbose)
{
  int content_length = 0;
  wiki_seq_tree_t* tree = initialize_tree();

  if (!content) { 
    tree->error = true;
    tree->error_str = (char*) "NULL content";
    return tree;
  }

  char* stripped_content = wiki_preprocess(content);
  if (!stripped_content) {
    tree->error = true;
    tree->error_str = (char*) "NULL content after preprocessing";
    return tree;
  }

  content_length = strlen(stripped_content);

  if (content_length == 0) {
    tree->error = true;
    tree->error_str = (char*) "Empty content after preprocessing";
    return tree;
  }

  parser_meta_t* meta = initialize_parser(tree, stripped_content,verbose);

  wiki_token_t token;
  char* p = (char*) stripped_content;
  char* pe = p + content_length;

  //int ignore_gate = 0;

  scan(&token,NULL,p,pe);
  
  for(;;) {
    if (meta->ctx && meta->ctx->ignorable) {
      // These are the token types that can exit us from an ignorable context
      switch(token.type) {
        case END_OF_FILE:
          parse_end_of_file(meta,&token);
          goto parse_complete;
        case NOWIKI_END:
          parse_nowiki_end(meta,&token);
          break;
        case PRE_END:
          parse_pre_end(meta,&token);
          break;
        case MATH_END:
          parse_math_end(meta,&token);
          break;
        case SOURCE_END:
          parse_source_end(meta,&token);
          break;
        case GALLERY_END:
          parse_gallery_end(meta,&token);
          break;
        default:
          break;
      }
    } else {
      switch(token.type) {
        case END_OF_FILE:
          parse_end_of_file(meta,&token);
          goto parse_complete;
        case REDIRECT:
          meta->redirect = true;
          break;
        case LINK_BEGIN:
          parse_link_begin(meta,&token);
          break;
        case LINK_END:
          parse_link_end(meta,&token);
          break;
        case EXTERNAL_LINK_BEGIN:
          parse_external_link_begin(meta,&token);
          break;
        case EXTERNAL_LINK_END:
          parse_external_link_end(meta,&token);
          break;
        case TEMPLATE_BEGIN:
          parse_template_begin(meta,&token);
          break;
        case TEMPLATE_END:
          parse_template_end(meta,&token);
          break;
        case IMAGE_LINK_BEGIN:
          parse_image_begin(meta,&token);
          break;
        case SEPARATOR:
          parse_separator(meta,&token);
          break;
        case EQUALS:
          parse_equals(meta,&token);
          break;
        case NOWIKI_BEGIN:
          parse_nowiki_begin(meta,&token);
          break;
        case PRE_BEGIN:
          parse_pre_begin(meta,&token);
          break;
        case REF:
          parse_ref(meta,&token);
          break;
        case REF_BEGIN:
          parse_ref_begin(meta,&token);
          break;
        case REF_END:
          parse_ref_end(meta,&token);
          break;
        case MATH_BEGIN:
          parse_math_begin(meta,&token);
          break;
        case SOURCE_BEGIN:
          parse_source_begin(meta,&token);
          break;
        case SPACE:
          parse_space(meta,&token);
          break;
        case HEADING:
          parse_heading(meta,&token);
          break;
        case CATEGORY_LINK_BEGIN:
          parse_category_link_begin(meta,&token);
          break;

        case LANGUAGE_LINK_BEGIN:
          parse_language_link_begin(meta,&token);
          break;

        case CRLF:
          parse_newline(meta,&token);
          break;

        case BOLD:
        case BOLD_ITALIC:
        case ITALIC:
        case BIG_BEGIN:
        case BIG_END:
        case CENTER_BEGIN:
        case CENTER_END:
        case SMALL_BEGIN:
        case SMALL_END:
        case BR_LINE_BREAK:
          parse_formatting(meta,&token);
          break;

        case DOUBLE_TEMPLATE_END:
          parse_double_template_end(meta,&token);
          break;

        case GALLERY_BEGIN:
          parse_gallery_begin(meta,&token);
          break;

        case TABLE_BEGIN:
          parse_table_begin(meta,&token);
          break;

        case TABLE_END:
          parse_table_end(meta,&token);
          break;

        case DOUBLE_SEPARATOR:
          parse_double_separator(meta,&token);
          break;

        case TABLE_CAPTION_BEGIN:
          parse_table_caption_begin(meta,&token);
          break;

        case EXCLAMATION:
          parse_exclamation(meta,&token);
          break;

        case DOUBLE_EXCLAMATION:
          parse_double_exclamation(meta,&token);
          break;

        case TABLE_ROW_BEGIN:
          parse_table_row_begin(meta,&token);
          break;

        default:
          break;
      }
    }

    if (meta->tree->error)
      goto parse_complete;

    scan(&token,&token,NULL,pe);
  }

  parse_complete:
  cleanup_parser(meta);

  free(stripped_content);

  return tree;
}


/**************************/
/* Private Implementation */
/**************************/
static char* make_str_from_begin_and_end(char* begin, char* end)
{
  int len = end - begin;
  if (len > 0) {
    char* new_str= (char*) malloc(len + 1);
    memcpy(new_str,begin,len);
    new_str[len] = '\0';
    return new_str;
  } else {
    return NULL;
  }
}

static inline void append_link_section(wiki_section_list_t* head,wiki_link_t* link)
{
  ((wiki_section_t*) link)->type = INTERNAL_LINK;
  TAILQ_INSERT_TAIL(head,(wiki_section_t*) link, entries);
}

/*
 * If we encounter the separator then we know the string before it is the
 * target of the link and the string behind it will be the alias part of the
 * link.
 */
static void parse_separator_in_internal_link(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* ctx = meta->ctx;
    wiki_link_t* link = (wiki_link_t*) ctx->section;
    if (!link->target) {
      link->target = make_str_from_begin_and_end(ctx->target_start,tok->start);
      ctx->alias_start = tok->stop;
      ctx->renamed_link = true;
    } 
    push_ctx_stack(meta,INTERNAL_LINK_ANCHOR_TEXT_CTX,NULL, ctx->section_list, tok);

  } else {
    set_error(meta,tok,"NULL context in parse_separator_in_internal_link");
  }
}

/*
 * When we see the first separator inside a template, make a key value for it
 * and jump into a PARAMETER_CTX
 */
static void parse_separator_in_template(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* ctx = meta->ctx;
    wiki_template_t* tpl = (wiki_template_t*) ctx->section;
    ctx->text_stop = tok->start;
    tpl->title = make_stripped_str_from_begin_and_end(ctx->text_start,ctx->text_stop);
    wiki_kv_pair_t* kv = (wiki_kv_pair_t*) build_key_value_section();
    TAILQ_INSERT_TAIL(&tpl->kv_pairs,kv,entries);
    push_ctx_stack(meta,PARAMETER_CTX,(wiki_section_t*) kv, &kv->sections, tok);
  } else {
    set_error(meta,tok,"Null context in parse_separator_in_template");
  }
}

/*
 * We build up a redirect section with the destination link set
 */
static void append_redirect_section(wiki_section_list_t* head,wiki_link_t* destination)
{
  wiki_redirect_t* redirect = (wiki_redirect_t*) calloc(1,sizeof(wiki_redirect_t));
  redirect->link = destination;
  ((wiki_section_t*) redirect)->type = REDIRECT_SECTION;
  TAILQ_INSERT_TAIL(head,(wiki_section_t*) redirect, entries);
}

static void append_template_section(parser_meta_t* meta,wiki_template_t* tpl)
{
  TAILQ_INSERT_TAIL(meta->ctx->section_list,(wiki_section_t*) tpl, entries);
}

/*
 * This method pushes a link ctx on the top of the stack and appends the text
 * up to this point to the current section list.  If it is in an internal link
 * ctx (usually a parse error), let's turn that old one into text and make this
 * the current link context.
 */
static void parse_link_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    if (!meta->redirect && is_text_container_context(parent_ctx)) {
      parent_ctx->text_stop= tok->start;
      append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);
    }

    switch(parent_ctx->type) {
      case EXTERNAL_LINK_CTX:
        parse_space_in_external_link(meta,tok);
        break;
      case INTERNAL_LINK_ANCHOR_TEXT_CTX:
        pop_ctx_stack(meta); // Fall Thru on purpose
      case INTERNAL_LINK_CTX:
        {
          meta->ctx->text_stop = tok->start;
          append_text_section(meta,meta->ctx->tok_start, meta->ctx->text_stop);
          wiki_link_t* link = (wiki_link_t*) meta->ctx->section;
          if (link->target)
            free(link->target);
          free(link);
          pop_ctx_stack(meta);
        }
        break;
      default:
        break;
    }

    wiki_link_t* link = (wiki_link_t*) calloc(1,sizeof(wiki_link_t));

    ctx_t* new_ctx = push_ctx_stack(meta,INTERNAL_LINK_CTX,(wiki_section_t*) link, meta->ctx->section_list, tok);
    new_ctx->target_start = tok->stop;
  } else {
    set_error(meta,tok,"Bad context in parse_link_begin");
  }
}

static void parse_link_end(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  if (ctx) {
    switch(ctx->type) {
      case INTERNAL_LINK_CTX:
        parse_link_end_in_internal_link(meta,tok);
        break;
      case INTERNAL_LINK_ANCHOR_TEXT_CTX:
        parse_link_end_in_internal_link_anchor_text(meta,tok);
        break;
      case IMAGE_CTX:
        parse_link_end_in_image(meta,tok);
        break;
      case PARAMETER_CTX:
        parse_link_end_in_parameter(meta,tok);
        break;
      case VALUE_CTX:
        parse_link_end_in_value(meta,tok);
        break;
      case CATEGORY_LINK_CTX:
        parse_link_end_in_category(meta,tok);
        break;
      case LANGUAGE_LINK_CTX:
        parse_link_end_in_language_link(meta,tok);
        break;
      default:
        break;
    }
  } else {
    set_error(meta,tok,"Null context at parse_link_end");
  }
}

static wiki_seq_tree_t* initialize_tree()
{
  wiki_seq_tree_t* tree = (wiki_seq_tree_t*) calloc(1,sizeof(wiki_seq_tree_t));
  TAILQ_INIT(&(tree->sections));
  return tree;
}

/* Initialize the meta data structure and push a root context onto the context stack */
static parser_meta_t* initialize_parser(wiki_seq_tree_t* tree, const char* content, bool verbose)
{
  parser_meta_t* meta = (parser_meta_t*) calloc(1,sizeof(parser_meta_t));
  meta->verbose = verbose;
  meta->tree = tree;
  TAILQ_INIT(&(meta->ctx_stack));
  ctx_t* root_ctx = (ctx_t*) calloc(1,sizeof(ctx_t));
  root_ctx->section_list = &(meta->tree->sections);
  root_ctx->type = ROOT_CTX;
  root_ctx->text_start = (char*) content;
  root_ctx->text_stop = (char*) content;
  TAILQ_INSERT_HEAD(&(meta->ctx_stack),root_ctx,entries);
  meta->ctx = root_ctx;

  return meta;
}

static void cleanup_parser(parser_meta_t* meta)
{
  ctx_t* ctx;
  while (!TAILQ_EMPTY(&(meta->ctx_stack))) {
    ctx = TAILQ_FIRST(&(meta->ctx_stack));
    TAILQ_REMOVE(&(meta->ctx_stack),ctx,entries);
    if (ctx->section)
      free_section(ctx->section);
    free(ctx);
  }
  free(meta);
}

/* 
 * Append the latest text segment onto the section.  Simulate seeing a newline
 * to close any contexts that end on newline 
 */
static void parse_end_of_file(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  if (ctx) {
    ctx->text_stop = tok->start;
    append_text_section(meta,ctx->text_start,ctx->text_stop);
    parse_newline(meta,tok);
  } else {
    set_error(meta,tok,"Null context at parse_end_of_file");
  }
}

static void parse_template_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* parent_ctx = meta->ctx;
  ctx_type_t ctx_type = meta->ctx->type;

  if (ctx_type == ROOT_CTX || ctx_type == PARAMETER_CTX || ctx_type == VALUE_CTX || ctx_type == REF_CTX) {
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* sec = build_template_section();

    ctx_t* new_ctx = push_ctx_stack(meta,TEMPLATE_CTX,sec, parent_ctx->section_list, tok);
    new_ctx->title_start = tok->stop;
    new_ctx->target_start = tok->stop;
    new_ctx->kv_pairs = &((wiki_template_t*) sec)->kv_pairs;
  }
}

static void parse_template_end(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* ctx = meta->ctx;
    switch(ctx->type) {
      case PARAMETER_CTX:
        parse_template_end_in_parameter(meta,tok);
        break;
      case VALUE_CTX:
        parse_template_end_in_value(meta,tok);
        break;
      case TEMPLATE_CTX:
        parse_template_end_in_template(meta,tok);
        break;
      default:
        break;
    }
  } else
    set_error(meta,tok,"NULL context in parse_template_end");
}

static void parse_separator(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* ctx = meta->ctx;
    switch(ctx->type) {
      case INTERNAL_LINK_CTX:
        parse_separator_in_internal_link(meta,tok);
        break;
      case VALUE_CTX:
        parse_separator_in_value(meta,tok);
        break;
      case TEMPLATE_CTX:
        parse_separator_in_template(meta,tok);
        break;
      case PARAMETER_CTX:
        parse_separator_in_parameter(meta,tok);
        break;
      case IMAGE_CTX:
        parse_separator_in_image(meta,tok);
        break;
      case TABLE_CAPTION_CTX:
        parse_separator_in_table_caption(meta,tok);
        break;
      case TABLE_ROW_CTX:
        parse_separator_in_table_row(meta,tok);
        break;
      case TABLE_CELL_CTX:
        parse_separator_in_table_cell(meta,tok);
        break;
      default:
        break;
    }
  }
}

/* 
 * If we see an equals inside a template or a link ctx then it should build a
 * key value pair type and push a new value ctx onto the ctx stack
 */
static void parse_equals(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* ctx = meta->ctx;
    switch(ctx->type) {
      case PARAMETER_CTX:
        parse_equals_in_parameter(meta,tok);
        break;
      default:
        break;
    }
  } else {
    set_error(meta,tok,"Null context in parse_equals");
  }
}


static void parse_separator_in_image(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* ctx = meta->ctx;
    wiki_image_t* img = (wiki_image_t*) ctx->section;
    ctx->text_stop = tok->start;
    img->href= make_str_from_begin_and_end(ctx->text_start,ctx->text_stop);
    wiki_kv_pair_t* kv = (wiki_kv_pair_t*) build_key_value_section();
    TAILQ_INSERT_TAIL(&img->kv_pairs,kv,entries);
    push_ctx_stack(meta,PARAMETER_CTX,(wiki_section_t*) kv, &kv->sections, tok);
  } else {
    set_error(meta,tok,"Null context in parse_separator_in_image");
  }
}

static void parse_image_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* parent_ctx = meta->ctx;
  if (parent_ctx) { 
    if (!meta->redirect) {
      parent_ctx->text_stop= tok->start;
      append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);
    }
    wiki_section_t* sec = build_image_section();
    ctx_t* new_ctx = push_ctx_stack(meta,IMAGE_CTX,sec,parent_ctx->section_list, tok);
    new_ctx->text_start = tok->stop;
    new_ctx->kv_pairs = &((wiki_image_t*) sec)->kv_pairs;
  } else {
    meta->tree->error = true;
    meta->tree->error_str = (char*) "Context Error in parse_image_begin";
  }
}

/* Pop off a ctx and do error checking */
static void pop_ctx_stack(parser_meta_t* meta)
{
  //printf("Popping %s\n", context_name_to_string(meta->ctx->type));
  if (!TAILQ_EMPTY(&meta->ctx_stack)) {
    if (meta->verbose) {
      ctx_t* iter = NULL;
      TAILQ_FOREACH(iter,&meta->ctx_stack,entries) {
        fprintf(stderr, "  ");
      }
      fprintf(stderr,"POP %s\n", context_name_to_string(meta->ctx->type));
    }
    ctx_t* head = TAILQ_FIRST(&meta->ctx_stack);
    TAILQ_REMOVE(&meta->ctx_stack,head,entries);
    free(head);
    meta->ctx = TAILQ_FIRST(&meta->ctx_stack);
  } else {
    meta->tree->error = true;
    meta->tree->error_str = (char*) "Popped off too many contexts";
  }
}

static ctx_t* push_ctx_stack(parser_meta_t* meta,ctx_type_t type,wiki_section_t* sec, wiki_section_list_t* sec_list, wiki_token_t* tok)
{
  //printf("Pushing %s\n", context_name_to_string(type));
  ctx_t* new_ctx = (ctx_t*) calloc(1,sizeof(ctx_t));
  new_ctx->text_start = tok->stop;
  new_ctx->type = type;
  new_ctx->section = sec;
  new_ctx->section_list = sec_list;
  new_ctx->tok_start = tok->start;
  new_ctx->tok_stop= tok->stop;

  TAILQ_INSERT_HEAD(&(meta->ctx_stack),new_ctx,entries);
  meta->ctx = new_ctx;
  if (meta->verbose) {
    ctx_t* iter = NULL;
    TAILQ_FOREACH(iter,&meta->ctx_stack,entries) {
      fprintf(stderr, "  ");
    }
    fprintf(stderr,"PUSH %s\n", context_name_to_string(meta->ctx->type));
  }
  return new_ctx;
}

static void set_error(parser_meta_t* meta,wiki_token_t* tok,const char* txt)
{
  if (meta && meta->tree) {
    meta->tree->error = true;
    if (txt)
      meta->tree->error_str = (char *) txt;
    if (tok && tok->start) {
      int len = min(strlen(tok->start),100);
      meta->tree->error_loc = (char*) malloc(len + 1);
      memcpy(meta->tree->error_loc,tok->start,len);
      meta->tree->error_loc[len] = '\0';
    }
  }
}

static wiki_section_t* build_template_section()
{
  wiki_template_t* tpl = (wiki_template_t*) calloc(1,sizeof(wiki_template_t));
  tpl->param_count = 0;
  TAILQ_INIT(&tpl->kv_pairs);
  ((wiki_section_t*) tpl)->type = TEMPLATE;
  return (wiki_section_t*) tpl;
}

static wiki_section_t* build_image_section()
{
  wiki_image_t* image = (wiki_image_t*) calloc(1,sizeof(wiki_image_t));
  TAILQ_INIT(&image->kv_pairs);
  ((wiki_section_t*) image)->type = IMAGE;
  return (wiki_section_t*) image;
}

/*
 * When we encounter a link_end inside an image ctx, then we want to append the
 * image to the section list.  
 */
static void parse_link_end_in_image(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  wiki_image_t* img = (wiki_image_t*) ctx->section;
  if (img) {
    if (!ctx->text_stop) {
      img->href = make_str_from_begin_and_end(ctx->text_start,tok->start);
    } 
    append_section(meta,ctx->section);
    pop_ctx_stack(meta);
    ctx_t* parent_ctx = meta->ctx;
    if (parent_ctx)
      parent_ctx->text_start = tok->stop;
    else {
      set_error(meta,tok,"Improper link pop");
    }
  } else {
    set_error(meta,tok,"Image is null in parse_link_end_in_image");
  }
}

/* 
 * When we encounter a link end inside of a parameter, we want to append the
 * unnamed parameter to the kv pairs of the parent ctx, which is most likely an
 * image context.
 */
static void parse_link_end_in_parameter(parser_meta_t* meta,wiki_token_t* tok)
{
  wiki_kv_pair_t* existing_kv = (wiki_kv_pair_t*) meta->ctx->section;
  append_text_section(meta,meta->ctx->text_start,tok->start);
  pop_ctx_stack(meta);

  existing_kv->key = next_numbered_parameter_key(meta->ctx);

  append_section(meta,meta->ctx->section);
  pop_ctx_stack(meta);
  ctx_t* parent_ctx = meta->ctx;
  if (parent_ctx)
    parent_ctx->text_start = tok->stop;
  else 
    set_error(meta,tok,"Null parent in parse_link_end_in_parameter");
}


static void parse_link_end_in_internal_link(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  wiki_link_t* link = (wiki_link_t*) ctx->section;
  if (link) {
    link->target = make_str_from_begin_and_end(ctx->target_start,tok->start);

    if (meta->redirect) {
      append_redirect_section(ctx->section_list,link);
    } else {
      append_link_section(ctx->section_list,link);
    }

    pop_ctx_stack(meta);
    ctx_t* parent_ctx = meta->ctx;
    if (parent_ctx) {
      parent_ctx->text_start = tok->stop;
    } else {
      set_error(meta,tok,"Improper link pop");
    }
  } else {
    set_error(meta,tok,"Link is null in parse_link_end_in_internal_link");
  }
}

static void append_section(parser_meta_t* meta, wiki_section_t* sec)
{
  if (meta->ctx) {
    TAILQ_INSERT_TAIL(meta->ctx->section_list,sec,entries);
  } else 
    set_error(meta,NULL,"NULL ctx in append_section");
}

/*
 * A value needs to grab the key value pair and pop off both the value and the
 * parameter contexts.
 */
static void parse_separator_in_value(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  append_text_section(meta,ctx->text_start,tok->start);
  pop_ctx_stack(meta);
  pop_ctx_stack(meta);
  ctx_t* parent = meta->ctx;
  if (parent) {
    parent->key_start = tok->stop;
    wiki_kv_pair_t* next_kv = (wiki_kv_pair_t*) build_key_value_section();
    TAILQ_INSERT_TAIL(parent->kv_pairs,next_kv,entries);
    push_ctx_stack(meta,PARAMETER_CTX,(wiki_section_t*) next_kv, &next_kv->sections, tok);
  } else {
    set_error(meta,tok,"NULL parent context in parse_separator_in_value");
  }
}

/*
 * Pop off both the VALUE and PARAMETER_CTX
 */
static void parse_template_end_in_value(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  append_text_section(meta,ctx->text_start,tok->start);
  pop_ctx_stack(meta);
  pop_ctx_stack(meta);

  ctx_t* parent = meta->ctx;
  if (parent) {
    if (parent->type == TEMPLATE_CTX) {
      append_section(meta,meta->ctx->section);
      pop_ctx_stack(meta);
      ctx_t* grand_parent = meta->ctx;
      if (grand_parent) 
        grand_parent->text_start = tok->stop;
      else
        set_error(meta,tok,"Null grandparent in parse_template_end_in_value");
    } else 
      set_error(meta,tok,"Parent is not a TEMPLATE inside of parse_template_end_in_value");
  } else {
    set_error(meta,tok,"Null parent in parse_template_end");
  }
}

static void parse_template_end_in_template(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  wiki_template_t* tpl = (wiki_template_t*) ctx->section;
  if (!ctx->title_stop) {
    tpl->title = make_str_from_begin_and_end(ctx->title_start,tok->start);
  } else if (ctx->key_start) {
    wiki_kv_pair_t* kv = (wiki_kv_pair_t*) calloc(1,sizeof(wiki_kv_pair_t));
    kv->key = make_str_from_begin_and_end(ctx->key_start,tok->start);
    TAILQ_INSERT_TAIL(ctx->kv_pairs,kv,entries);
  }

  append_template_section(meta,tpl);
  pop_ctx_stack(meta);

  ctx_t* parent_ctx = meta->ctx;
  if (parent_ctx) 
    parent_ctx->text_start = tok->stop;
  else
    set_error(meta,tok,"Null parent in parse_template_end_in_template");
}

static void parse_separator_in_parameter(parser_meta_t* meta, wiki_token_t* tok)
{
  wiki_kv_pair_t* existing_kv = (wiki_kv_pair_t*) meta->ctx->section;
  append_text_section(meta,meta->ctx->text_start,tok->start);
  pop_ctx_stack(meta);
  existing_kv->key = next_numbered_parameter_key(meta->ctx);

  wiki_kv_pair_t* new_kv = (wiki_kv_pair_t*) build_key_value_section();
  TAILQ_INSERT_TAIL(meta->ctx->kv_pairs,new_kv,entries);
  push_ctx_stack(meta,PARAMETER_CTX,(wiki_section_t*) new_kv, &new_kv->sections, tok);
}

/* Build and initialize the key value section */
static wiki_section_t* build_key_value_section()
{
  wiki_kv_pair_t* kv = (wiki_kv_pair_t*) calloc(1,sizeof(wiki_kv_pair_t));
  TAILQ_INIT(&kv->sections);
  ((wiki_section_t*) kv)->type = KEY_VALUE;
  return (wiki_section_t*) kv;
}

static void parse_equals_in_parameter(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;

  ctx->text_stop = tok->start;
  wiki_kv_pair_t* kv = (wiki_kv_pair_t*) meta->ctx->section;
  //kv->key = make_str_from_begin_and_end(ctx->text_start,ctx->text_stop);
  kv->key = make_stripped_str_from_begin_and_end(ctx->text_start,ctx->text_stop);

  push_ctx_stack(meta,VALUE_CTX,NULL, &kv->sections, tok);
}

/*
 * This needs to pop off two contexts and store the key value pair, as well as
 * finally pushing the section onto the parent
 */
static void parse_link_end_in_value(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  append_text_section(meta,ctx->text_start,tok->start);
  pop_ctx_stack(meta);
  pop_ctx_stack(meta);
  ctx_t* parent = meta->ctx;
  if (parent) {
    append_section(meta,meta->ctx->section);
    pop_ctx_stack(meta);
    ctx_t* grand_parent = meta->ctx;
    if (grand_parent) 
      grand_parent->text_start = tok->stop;
    else
      set_error(meta,tok,"NULL grand parent context in parse_link_end_in_value");
  } else {
    set_error(meta,tok,"NULL parent context in parse_link_end_in_value");
  }
}

static void parse_template_end_in_parameter(parser_meta_t* meta, wiki_token_t* tok)
{
  wiki_kv_pair_t* existing_kv = (wiki_kv_pair_t*) meta->ctx->section;
  append_text_section(meta,meta->ctx->text_start,tok->start);
  pop_ctx_stack(meta);

  existing_kv->key = next_numbered_parameter_key(meta->ctx);

  append_section(meta,meta->ctx->section);
  pop_ctx_stack(meta);

  ctx_t* parent_ctx = meta->ctx;
  if (parent_ctx)
    parent_ctx->text_start = tok->stop;
  else 
    set_error(meta,tok,"Null parent in parse_template_end_in_parameter");
}

/* A beautiful switch statement to free everything! */
static void free_section(wiki_section_t* sec)
{
  switch(sec->type) {
    case TEXT: 
      free_text((wiki_text_t*) sec);
      break;

    case INTERNAL_LINK: 
      free_link((wiki_link_t*) sec);
      break;

    case EXTERNAL_LINK:
      free_external_link((wiki_external_link_t*) sec);
      break;

    case HEADING_SECTION:
      free_heading((wiki_heading_t*) sec);
      break;

    case FORMATTING_SECTION:
      free_formatting((wiki_formatting_t*) sec);
      break;

    case IMAGE:
      free_image((wiki_image_t*) sec);
      break;

    case REDIRECT_SECTION:
      free_redirect((wiki_redirect_t*) sec);
      break;

    case TEMPLATE:
      free_template((wiki_template_t*) sec);
      break;

    case PRE:
      free_pre((wiki_pre_t*) sec);
      break;

    case GALLERY_SECTION:
      free_gallery((wiki_gallery_t*) sec);
      break;

    case MATH_SECTION:
      free_math((wiki_math_t*) sec);
      break;

    case REF_SECTION:
      free_ref((wiki_ref_t*) sec);
      break;

    case SOURCE_SECTION:
      free_source((wiki_source_t*) sec);
      break;

    case CATEGORY_LINK_SECTION:
      free_category_link((wiki_category_link_t*) sec);
      break;

    case LANGUAGE_LINK_SECTION:
      free_language_link((wiki_language_link_t*) sec);
      break;

    case TABLE_SECTION:
      free_table((wiki_table_t*) sec);
      break;

    default:
      break;
  }
}

static void free_text(wiki_text_t* txt)
{
  free(txt->text);
  free(txt);
}

static void free_link(wiki_link_t* link)
{
  free(link->alias);
  free(link->target);
  free(link);
}

static void free_image(wiki_image_t* img)
{
  wiki_kv_pair_t* kv = NULL;
  while((kv = TAILQ_FIRST(&img->kv_pairs))) {
    TAILQ_REMOVE(&img->kv_pairs,kv,entries);
    free_key_value_pair(kv);
  }
  free(img->href);
  free(img);
}

static void free_key_value_pair(wiki_kv_pair_t* kv)
{
  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&kv->sections))) {
    TAILQ_REMOVE(&kv->sections,sec,entries);
    free_section(sec);
  }
  free(kv->key);
  free(kv);
}

static void free_redirect(wiki_redirect_t* red)
{
  free_link(red->link);
  free(red);
}

static void free_template(wiki_template_t* tpl)
{
  free(tpl->title);
  wiki_kv_pair_t* kv = NULL;
  while((kv = TAILQ_FIRST(&tpl->kv_pairs))) {
    TAILQ_REMOVE(&tpl->kv_pairs,kv,entries);
    free_key_value_pair(kv);
  }
  free(tpl);
}

/* This should set the meta information to ignore and a push a nowiki ctx onto
 * the stack.  There is some trickiness because <nowiki> simulates a space
 * delimiter inside of an external link.  I don't know if it is deprecated, but
 * let's fake a space when that happens.
 */
static void parse_nowiki_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    if (is_text_container_context(parent_ctx)) {
      parent_ctx->text_stop = tok->start;
      append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);
    }

    if (parent_ctx->type == EXTERNAL_LINK_CTX) {
      parse_space_in_external_link(meta,tok);
    }

    wiki_section_t* txt = build_text_section();
    ctx_t* nowiki_ctx = push_ctx_stack(meta,NOWIKI_CTX,txt,meta->ctx->section_list,tok);
    nowiki_ctx->ignorable = true;
  } else
    set_error(meta,tok,"NULL context inside of parse_nowiki_begin");
}

static void parse_nowiki_end(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    if (meta->ctx->type == NOWIKI_CTX) {
      wiki_text_t* txt = (wiki_text_t*) meta->ctx->section;
      txt->text = make_str_from_begin_and_end(meta->ctx->text_start,tok->start);
      pop_ctx_stack(meta);
      ctx_t* parent = meta->ctx;
      if (parent) {
        if (txt->text)
          append_section(meta,(wiki_section_t*) txt);
        parent->text_start = tok->stop;
      } else 
        set_error(meta,tok,"NULL parent inside of parse_nowiki_end");
    } else {
      // Add a warning here for a mismatch of nowiki
    }
  } else
    set_error(meta,tok,"NULL context inside of parse_nowiki_end");
}

static void parse_pre_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* pre = build_pre_section();
    ctx_t* pre_ctx = push_ctx_stack(meta,PRE_CTX,pre,parent_ctx->section_list,tok);
    pre_ctx->ignorable = true;
  } else
    set_error(meta,tok,"NULL context inside of parse_pre_begin");
}

static void parse_pre_end(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    if (meta->ctx->type == PRE_CTX) {
      wiki_pre_t* pre= (wiki_pre_t*) meta->ctx->section;
      pre->text = make_str_from_begin_and_end(meta->ctx->text_start,tok->start);
      pop_ctx_stack(meta);
      ctx_t* parent = meta->ctx;
      if (parent) {
        append_section(meta,(wiki_section_t*) pre);
        parent->text_start = tok->stop;
      } else 
        set_error(meta,tok,"NULL parent inside of parse_pre_end");
    } else {
      // Add a warning here for a mismatched pre
    }
  } else
    set_error(meta,tok,"NULL context inside of parse_pre_end");
}

static wiki_section_t* build_text_section()
{
  wiki_section_t* text = (wiki_section_t*) calloc(1,sizeof(wiki_text_t));
  text->type = TEXT;
  return text;
}

static wiki_section_t* build_pre_section()
{
  wiki_section_t* pre = (wiki_section_t*) calloc(1,sizeof(wiki_pre_t));
  pre->type = PRE;
  return pre;
}
      
static void free_pre(wiki_pre_t* pre)
{
  free(pre->text);
  free(pre);
}

static void free_gallery(wiki_gallery_t* sec)
{
  free(sec->text);
  free(sec);
}

static void parse_ref_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* ref = build_ref_section();
    push_ctx_stack(meta,REF_CTX,ref,&((wiki_ref_t*)ref)->sections,tok);
  } else
    set_error(meta,tok,"NULL context inside of parse_ref_begin");
}

static wiki_section_t* build_ref_section()
{
  wiki_ref_t* ref = (wiki_ref_t*) calloc(1,sizeof(wiki_ref_t));
  TAILQ_INIT(&ref->sections);
  ((wiki_section_t*) ref)->type = REF_SECTION;
  return (wiki_section_t*) ref;
}

/*
 * Pop off the ref stack and add the ref section to the tree
 */
static void parse_ref_end(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    if (meta->ctx->type == REF_CTX) {
      ctx_t* ctx = meta->ctx;
      wiki_ref_t* ref = (wiki_ref_t*) meta->ctx->section;
      append_text_section(meta,ctx->text_start,ctx->text_stop);
      pop_ctx_stack(meta);

      ctx_t* parent = meta->ctx;
      if (parent) {
        append_section(meta,(wiki_section_t*) ref);
        parent->text_start = tok->stop;
      } else 
        set_error(meta,tok,"NULL parent inside of parse_ref_end");
    } else {
      // Want to add a warning here, but don't totally error out.
      //set_error(meta,tok,"Mismatched ref tag");
    }
  } else
    set_error(meta,tok,"NULL context inside of parse_ref_end");
}

/*
 * Just build an inline ref at this moment and append it to the parent
 */
static void parse_ref(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* ref = build_ref_section();

    append_section(meta,(wiki_section_t*) ref);
    parent_ctx->text_start = tok->stop;
  } else
    set_error(meta,tok,"NULL context inside of parse_ref");
}

static void free_ref(wiki_ref_t* ref)
{
  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&ref->sections))) {
    TAILQ_REMOVE(&ref->sections,sec,entries);
    free_section(sec);
  }
  free(ref);
}

static void free_heading(wiki_heading_t* heading)
{
  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&heading->sections))) {
    TAILQ_REMOVE(&heading->sections,sec,entries);
    free_section(sec);
  }
  free(heading);
}

static void free_formatting(wiki_formatting_t* formatting)
{
  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&formatting->sections))) {
    TAILQ_REMOVE(&formatting->sections,sec,entries);
    free_section(sec);
  }
  free(formatting);
}

static inline int min(int x, int y)
{
  if (x > y)
    return y;
  else
    return x;
}

static wiki_section_t* build_math_section()
{
  wiki_section_t* math = (wiki_section_t*) calloc(1,sizeof(wiki_math_t));
  math->type = MATH_SECTION;
  return math;
}

static void parse_math_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* math = build_math_section();
    ctx_t* math_ctx = push_ctx_stack(meta,MATH_CTX,math,parent_ctx->section_list,tok);
    math_ctx->ignorable = true;
  } else
    set_error(meta,tok,"NULL context inside of parse_math_begin");
}

static void parse_math_end(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    if (meta->ctx->type == MATH_CTX) {
      wiki_math_t* math = (wiki_math_t*) meta->ctx->section;
      math->text = make_str_from_begin_and_end(meta->ctx->text_start,tok->start);
      pop_ctx_stack(meta);
      ctx_t* parent = meta->ctx;
      if (parent) {
        append_section(meta,(wiki_section_t*) math);
        parent->text_start = tok->stop;
      } else 
        set_error(meta,tok,"NULL parent inside of parse_math_end");
    } else {
      // Add Warning here for a mismatched close
    }
  } else
    set_error(meta,tok,"NULL context inside of parse_math_end");
}

static void free_math(wiki_math_t* math)
{
  free(math->text);
  free(math);
}

static wiki_section_t* build_source_section()
{
  wiki_section_t* source = (wiki_section_t*) calloc(1,sizeof(wiki_source_t));
  source->type = SOURCE_SECTION;
  return source;
}

static void parse_source_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* source = build_source_section();
    ctx_t* source_ctx = push_ctx_stack(meta,SOURCE_CTX,source,parent_ctx->section_list,tok);
    source_ctx->ignorable = true;
  } else
    set_error(meta,tok,"NULL context inside of parse_source_begin");
}

static void parse_source_end(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx && meta->ctx->type == SOURCE_CTX) {
    wiki_source_t* source = (wiki_source_t*) meta->ctx->section;
    source->text = make_str_from_begin_and_end(meta->ctx->text_start,tok->start);
    pop_ctx_stack(meta);
    ctx_t* parent = meta->ctx;
    if (parent) {
      append_section(meta,(wiki_section_t*) source);
      parent->text_start = tok->stop;
    } else 
      set_error(meta,tok,"NULL parent inside of parse_source_end");
  } else
    set_error(meta,tok,"NULL context inside of parse_source_end");
}

static void free_source(wiki_source_t* source)
{
  free(source->text);
  free(source);
}

static void parse_external_link_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* link = build_external_link_section();
    push_ctx_stack(meta,EXTERNAL_LINK_CTX,link,meta->ctx->section_list,tok);
  }
}

static void parse_external_link_end(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  if (ctx) { 
    switch(ctx->type) {
      case EXTERNAL_LINK_CTX:
        parse_external_link_end_in_external_link(meta,tok);
        break;
      case EXTERNAL_LINK_VALUE_CTX:
        parse_external_link_end_in_external_link_value(meta,tok);
        break;
      default:
        break;
    }
  } else {
    set_error(meta,tok,"NULL context inside of parse_external_link_end");
  }
}

static void parse_external_link_end_in_external_link(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  wiki_external_link_t* link = (wiki_external_link_t*) ctx->section;
  link->target = generate_href(ctx,tok);
  append_section(meta,(wiki_section_t*) link);
  pop_ctx_stack(meta);
  meta->ctx->text_start = tok->stop;
}

static void parse_external_link_end_in_external_link_value(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  append_text_section(meta,ctx->text_start,tok->start);
  pop_ctx_stack(meta);
  wiki_external_link_t* link = (wiki_external_link_t*) meta->ctx->section;
  pop_ctx_stack(meta);
  ctx_t* parent = meta->ctx;
  if (parent) {
    append_section(meta,(wiki_section_t*) link);
    parent->text_start = tok->stop;
  } else {
    set_error(meta,tok,"NULL grandparent inside of parse_external_link_end_in_external_link_value");
  }
}

static wiki_section_t* build_external_link_section()
{
  wiki_external_link_t* link = (wiki_external_link_t*) calloc(1,sizeof(wiki_external_link_t));
  ((wiki_section_t*)link)->type = EXTERNAL_LINK;
  TAILQ_INIT(&link->sections);
  return (wiki_section_t*) link;
}

/*
 * We only care about spaces when they are used as a separator for external
 * links, otherwise they should be ignored.  TODO, handle leading whitespace.
 */
static void parse_space(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  if (ctx) {
    switch(ctx->type) {
      case EXTERNAL_LINK_CTX:
        parse_space_in_external_link(meta,tok);
        break;
      default:
        break;
    }
  }
}

static char* generate_href(ctx_t* ctx,wiki_token_t* tok)
{
  char* href = NULL;
  int protocol_len = ctx->tok_stop - ctx->tok_start - 1;
  int suffix_len = tok->start - ctx->text_start;
  if (protocol_len > 0 && suffix_len > 0) {
    int url_len = protocol_len + suffix_len;
    href = (char*) malloc(url_len + 1);
    memcpy(href,ctx->tok_start + 1,protocol_len);
    memcpy(href + protocol_len,ctx->text_start,suffix_len);
    href[url_len] = '\0';
    return href;
  } else {
    return NULL;
  }
}

static void free_external_link(wiki_external_link_t* link)
{
  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&link->sections))) {
    TAILQ_REMOVE(&link->sections,sec,entries);
    free_section(sec);
  }
  free(link->target);
  free(link);
}

/*
 * A text container context is one that can have 0-n text nodes inside it among
 * other things.  An example would be a root context or a parameter value.  An
 * non text container would be something like an internal link, and external
 * link, something that shouldn't have text appended to it
 */
static bool is_text_container_context(ctx_t* ctx)
{
  switch(ctx->type) {
    case ROOT_CTX:
    case PARAMETER_CTX:
    case VALUE_CTX:
    case REF_CTX:
    case EXTERNAL_LINK_VALUE_CTX:
    case HEADING_CTX:
      return true;
    default:
      return false;
  }
}

static void parse_space_in_external_link(parser_meta_t* meta,wiki_token_t* tok)
{
  wiki_external_link_t* link = (wiki_external_link_t*) meta->ctx->section;
  link->target = generate_href(meta->ctx,tok);
  push_ctx_stack(meta,EXTERNAL_LINK_VALUE_CTX,NULL,&link->sections,tok);
}

/*
 * headings don't have a differentiator between start and finish so we are just
 * going to close the existing one if we are in a heading context.
 */
static void parse_heading(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx && meta->ctx->type == HEADING_CTX) {
    append_text_section(meta,meta->ctx->text_start,tok->start);
    wiki_section_t* heading = meta->ctx->section;
    pop_ctx_stack(meta);
    append_section(meta,heading);
    meta->ctx->text_start = tok->stop;
  } else {
    append_text_section(meta,meta->ctx->text_start, tok->start);
    wiki_section_t* heading = build_heading_section();
    push_ctx_stack(meta,HEADING_CTX,heading,&((wiki_heading_t*)heading)->sections,tok);
  }
}

static wiki_section_t* build_heading_section()
{
  wiki_heading_t* heading = (wiki_heading_t*) calloc(1,sizeof(wiki_heading_t));
  TAILQ_INIT(&heading->sections);
  ((wiki_section_t*) heading)->type = HEADING_SECTION;
  return (wiki_section_t*) heading;
}

static void parse_formatting(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    if(is_text_container_context(meta->ctx))
      append_text_section(meta,meta->ctx->text_start,tok->start);
    meta->ctx->text_start = tok->stop;
  } else {
    set_error(meta,tok,"NULL context inside of parse_formatting");
  }
}

/* For newlines, we mainly just want to close any outstanding formatting contexts */
static void parse_newline(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    switch(meta->ctx->type) {
      case FORMATTING_CTX:
        {
          wiki_section_t* format = meta->ctx->section;
          pop_ctx_stack(meta);
          if (meta->ctx) {
            append_section(meta,format);
            meta->ctx->text_start = tok->stop;
          } else {
            set_error(meta,tok,"NULL parent inside of parse_newline");
          }
        }
        break;
      case INTERNAL_LINK_CTX:
        parse_newline_in_internal_link(meta,tok);
        break;
      case TABLE_CTX:
        parse_newline_in_table(meta,tok);
        break;
      case TABLE_ROW_CTX:
        parse_newline_in_table_row(meta,tok);
        break;
      default:
        break;
    }
  } else {
    set_error(meta,tok,"NULL context inside of parse_newline");
  }
}

static void parse_category_link_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    if (!meta->redirect && is_text_container_context(parent_ctx)) {
      parent_ctx->text_stop= tok->start;
      append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);
    }

    wiki_section_t* link = build_category_link();
    ctx_t* new_ctx = push_ctx_stack(meta,CATEGORY_LINK_CTX,link, meta->ctx->section_list, tok);
    new_ctx->target_start = tok->stop;

  } else {
    set_error(meta,tok,"Bad context in parse_category_link_begin");
  }
}

static wiki_section_t* build_category_link()
{
  wiki_section_t* cat = (wiki_section_t*) calloc(1,sizeof(wiki_category_link_t));
  cat->type = CATEGORY_LINK_SECTION;
  return cat;
}

static void parse_link_end_in_category(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  if (ctx) {
    wiki_category_link_t* cat = (wiki_category_link_t*) ctx->section;
    if (cat) {
      cat->category = make_str_from_begin_and_end(ctx->text_start, tok->start);
      pop_ctx_stack(meta);
      append_section(meta,(wiki_section_t*) cat);
      meta->ctx->text_start = tok->stop;
    } else {
      set_error(meta,tok,"Link is null in parse_link_end_in_category");
    }
  } else {
    set_error(meta,tok,"NULL ctx inside of parse_link_end_in_category");
  }
}

static wiki_section_t* build_language_link()
{
  wiki_section_t* lang = (wiki_section_t*) calloc(1,sizeof(wiki_language_link_t));
  lang->type = LANGUAGE_LINK_SECTION;
  return lang;
}

static void parse_language_link_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    if (!meta->redirect && is_text_container_context(parent_ctx)) {
      parent_ctx->text_stop= tok->start;
      append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);
    }

    wiki_section_t* link = build_language_link();
    ((wiki_language_link_t*) link)->language = retrieve_language_from_token(tok);
    ctx_t* new_ctx = push_ctx_stack(meta,LANGUAGE_LINK_CTX,link, meta->ctx->section_list, tok);
    new_ctx->target_start = tok->stop;
  } else {
    set_error(meta,tok,"Bad context in parse_category_link_begin");
  }
}

static void free_category_link(wiki_category_link_t* cat)
{
  free(cat->category);
  free(cat);
}

static void free_language_link(wiki_language_link_t* lang)
{
  free(lang->language);
  free(lang->target);
  free(lang);
}

static void parse_link_end_in_language_link(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  if (ctx) {
    wiki_language_link_t* ll = (wiki_language_link_t*) ctx->section;
    if (ll) {
      ll->target = make_str_from_begin_and_end(ctx->text_start, tok->start);
      pop_ctx_stack(meta);
      append_section(meta,(wiki_section_t*) ll);
      meta->ctx->text_start = tok->stop;
    } else {
      set_error(meta,tok,"ll is null in parse_link_end_in_language_link");
    }
  } else {
    set_error(meta,tok,"NULL ctx inside of parse_link_end_in_language_link");
  }
}

/*
 * Look for a language code in the token
 */
static char* retrieve_language_from_token(wiki_token_t* tok)
{
  char* lang_start = tok->start;
  while((*lang_start == '[' || *lang_start == ' ') && lang_start < tok->stop)
    lang_start++;

  char* lang_stop = lang_start;
  while((*lang_stop != ' ' && *lang_stop != ':') && lang_stop < tok->stop)
    lang_stop++;

  return make_str_from_begin_and_end(lang_start,lang_stop);
}

/*
 * This will make a string but strip any leading or trailing whitespace
 */
static char* make_stripped_str_from_begin_and_end(char* begin, char* end)
{
  if (!begin || !end)
    return NULL;

  int len = end - begin;
  if (len > 0) {
    char* first_nonspace = end;
    for(int i=0; i < len; i++) {
      if (!isspace(begin[i])) {
        first_nonspace = begin + i;
        break;
      }
    }

    if (first_nonspace == end)
      return NULL;

    char* last_nonspace = begin;
    for(int i=1; i < len; i++) {
      if (!isspace(*(end - i))) {
        last_nonspace = end - i;
        break;
      }
    }

    return make_str_from_begin_and_end(first_nonspace,last_nonspace + 1);
  } else {
    return NULL;
  }
}

/*
 * Some contexts should ignore leading and trailing whitespace in text entries.
 * Namely inside of values and parameters
 */
static bool is_trim_whitespace_context(ctx_t* ctx)
{
  switch(ctx->type) {
    case PARAMETER_CTX:
    case VALUE_CTX:
    case HEADING_CTX:
      return true;
    default:
      return false;
  }
}

static void append_text_section(parser_meta_t* meta, char* start, char* stop)
{
  char* new_str = NULL;
  if (is_trim_whitespace_context(meta->ctx)) 
    new_str = make_stripped_str_from_begin_and_end(start,stop); 
  else
    new_str = make_str_from_begin_and_end(start,stop);

  if (new_str) {
    wiki_text_t* section = (wiki_text_t*) calloc(1,sizeof(wiki_text_t));
    ((wiki_section_t*) section)->type = TEXT;
    section->text = new_str;
    TAILQ_INSERT_TAIL(meta->ctx->section_list,(wiki_section_t*) section,entries);
  }
}

static void parse_newline_in_internal_link(parser_meta_t* meta, wiki_token_t* tok)
{
  meta->ctx->text_stop = tok->start;
  append_text_section(meta,meta->ctx->tok_start, meta->ctx->text_stop);
  wiki_link_t* link = (wiki_link_t*) meta->ctx->section;
  if (link) {
    if (link->target)
      free(link->target);
    free(link);
  } else {
    set_error(meta,tok,"NULL Link in parse_newline_in_internal_link");
  }
  pop_ctx_stack(meta);
  meta->ctx->text_start = tok->start;
}

static void parse_link_end_in_internal_link_anchor_text(parser_meta_t* meta,wiki_token_t* tok)
{
  ctx_t* ctx = meta->ctx;
  char* anchor_text = make_str_from_begin_and_end(ctx->text_start,tok->start);

  pop_ctx_stack(meta);
  if (meta->ctx->type == INTERNAL_LINK_CTX) {
    wiki_link_t* link = (wiki_link_t*) meta->ctx->section;
    if (link) {
      if (anchor_text) {
        link->alias = anchor_text;
      } else {
        if (link->target) 
          link->alias = strdup(link->target);
      }

      append_link_section(meta->ctx->section_list,link);
      pop_ctx_stack(meta);
      ctx_t* parent_ctx = meta->ctx;
      if (parent_ctx)
        parent_ctx->text_start = tok->stop;
      else
        set_error(meta,tok,"Improper link pop");
    } else {
      set_error(meta,tok,"Link is null in parse_link_end_in_internal_link_anchor_text");
    }
  } else {
    set_error(meta,tok,"Parent on INTERNAL_LINK_ANCHOR_TEXT_CTX is not INTERNAL_LINK_CTX");
  }
}

/*
 * Use the parameter count to make the next numbered parameter key for a
 * template.
 */
static char* next_numbered_parameter_key(ctx_t* ctx)
{
  ctx->param_count++;
  char param_id[10];
  snprintf(param_id,10,"%d",(int) ctx->param_count);
  return strdup(param_id);
}

/*
 * Handle the }}}} cases which are a little trickier
 */
static void parse_double_template_end(parser_meta_t* meta, wiki_token_t* tok)
{
  parse_template_end(meta,tok);
  parse_template_end(meta,tok);
}

static void parse_gallery_begin(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    ctx_t* parent_ctx = meta->ctx;
    parent_ctx->text_stop = tok->start;
    append_text_section(meta,parent_ctx->text_start,parent_ctx->text_stop);

    wiki_section_t* gallery = build_gallery_section();
    ctx_t* gallery_ctx = push_ctx_stack(meta,GALLERY_CTX,gallery,parent_ctx->section_list,tok);
    gallery_ctx->ignorable = true;
  } else
    set_error(meta,tok,"NULL context inside of parse_gallery_begin");
}

static void parse_gallery_end(parser_meta_t* meta,wiki_token_t* tok)
{
  if (meta->ctx) {
    if (meta->ctx->type == GALLERY_CTX) {
      wiki_gallery_t* gallery= (wiki_gallery_t*) meta->ctx->section;
      gallery->text = make_str_from_begin_and_end(meta->ctx->text_start,tok->start);
      pop_ctx_stack(meta);
      ctx_t* parent = meta->ctx;
      if (parent) {
        append_section(meta,(wiki_section_t*) gallery);
        parent->text_start = tok->stop;
      } else 
        set_error(meta,tok,"NULL parent inside of parse_gallery_end");
    } else {
      // Add a warning for a mismatch here
     }
  } else
    set_error(meta,tok,"NULL context inside of parse_gallery_end");
}

static wiki_section_t* build_gallery_section()
{
  wiki_section_t* gallery = (wiki_section_t*) calloc(1,sizeof(wiki_gallery_t));
  gallery->type = GALLERY_SECTION;
  return gallery;
}

static void parse_table_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  meta->ctx->text_stop = tok->start;
  append_text_section(meta,meta->ctx->text_start,meta->ctx->text_stop);
  wiki_section_t* sec = build_table_section();
  push_ctx_stack(meta,TABLE_CTX, sec, meta->ctx->section_list, tok);
}

static void parse_table_end(parser_meta_t* meta, wiki_token_t* tok)
{
  switch(meta->ctx->type) {
    case TABLE_CTX:
      parse_table_end_in_table(meta,tok);
      break;
    case TABLE_ROW_CTX:
      parse_table_end_in_table_row(meta,tok);
      break;
    case TABLE_CELL_CTX:
      parse_table_end_in_table_cell(meta,tok);
      break;
    default:
      break;
  }
}

static void parse_table_caption_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx->type == TABLE_CTX) {
    wiki_table_t* table = meta->ctx->cur_table;
    wiki_section_t* sec = build_table_caption();
    ctx_t* new_ctx = push_ctx_stack(meta,TABLE_CAPTION_CTX, sec, &((wiki_table_caption_t*) sec)->sections, tok);
    new_ctx->cur_table = table;
  }
}

static void parse_exclamation(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx->type == TABLE_ROW_CTX) {
    if (has_only_leading_whitespace_in_ctx(meta->ctx,tok)) {
      ctx_t* parent = meta->ctx;
      wiki_row_t* row = (wiki_row_t*) parent->section;
      row->style = make_stripped_str_from_begin_and_end(parent->text_start,tok->start);
      wiki_table_cell_t* cell = (wiki_table_cell_t*) build_table_cell();
      cell->header = true;
      ctx_t* cell_ctx = push_ctx_stack(meta,TABLE_CELL_CTX,(wiki_section_t*) cell,&cell->content,tok);
      cell_ctx->cur_table = parent->cur_table;
      cell_ctx->cur_row = parent->cur_row;
    }
  }
}

static void parse_double_exclamation(parser_meta_t* meta, wiki_token_t* tok)
{
  //printf("Parse Double Exclamation\n");
}

static void parse_table_row_begin(parser_meta_t* meta, wiki_token_t* tok)
{
  switch(meta->ctx->type) {
    case TABLE_CAPTION_CTX:
      parse_table_row_begin_in_caption(meta,tok);
      break;
    case TABLE_CTX:
      parse_table_row_begin_in_table(meta,tok);
      break;
    case TABLE_CELL_CTX:
      parse_table_row_begin_in_table_cell(meta,tok);
      break;
    default:
      break;
  }
}

static void parse_table_row_begin_in_caption(parser_meta_t* meta, wiki_token_t* tok)
{
  append_text_section(meta,meta->ctx->text_start,tok->start);
  wiki_table_caption_t* cap = (wiki_table_caption_t*) meta->ctx->section;
  pop_ctx_stack(meta);
  wiki_table_t* table = (wiki_table_t*) meta->ctx->section;
  table->caption = cap;

  wiki_section_t* row = build_table_row();
  meta->ctx->cur_row = (wiki_row_t*) row;

  ctx_t* new_ctx = push_ctx_stack(meta,TABLE_ROW_CTX,row, meta->ctx->section_list, tok);
  new_ctx->cur_table = table;
  new_ctx->cur_row = (wiki_row_t*) row;
}

static void parse_table_row_begin_in_table(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* table_ctx = meta->ctx;
  wiki_table_t* table = (wiki_table_t*) table_ctx->section;
  if (!table_ctx->newline_encountered) {
    table_ctx->newline_encountered = true;
    table->style = make_stripped_str_from_begin_and_end(table_ctx->text_start,tok->start);
  }

  wiki_section_t* row = build_table_row();
  meta->ctx->cur_row = (wiki_row_t*) row;

  ctx_t* new_ctx = push_ctx_stack(meta,TABLE_ROW_CTX,row, meta->ctx->section_list, tok);
  new_ctx->cur_table = table;
  new_ctx->cur_row = (wiki_row_t*) row;
}

/*
 * This needs to add the existing cell into the current row, then append a new row into the table.
 */
static void parse_table_row_begin_in_table_cell(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    append_text_section(meta,meta->ctx->text_start,tok->start);
    wiki_table_cell_t* cell = (wiki_table_cell_t*) meta->ctx->section;
    wiki_table_t* table = meta->ctx->cur_table;
    wiki_row_t* row = meta->ctx->cur_row;
    TAILQ_INSERT_TAIL(&row->cells,cell,entries);
    pop_ctx_stack(meta);
    TAILQ_INSERT_TAIL(&table->rows,row,entries);
    pop_ctx_stack(meta);

    wiki_section_t* new_row = build_table_row();
    meta->ctx->cur_row = (wiki_row_t*) new_row;

    ctx_t* new_ctx = push_ctx_stack(meta,TABLE_ROW_CTX,new_row, meta->ctx->section_list, tok);
    new_ctx->cur_table = table;
    new_ctx->cur_row = (wiki_row_t*) new_row;
  } else 
    set_error(meta,tok,"NULL context inside of parse_table_row_begin_in_table_cell");
}

static void parse_double_separator(parser_meta_t* meta, wiki_token_t* tok)
{
  //printf("Parse Double Seperator\n");
}

static wiki_section_t* build_table_section()
{
  wiki_table_t* table = (wiki_table_t*) calloc(1,sizeof(wiki_table_t));
  TAILQ_INIT(&table->rows);
  ((wiki_section_t*) table)->type = TABLE_SECTION;
  return (wiki_section_t*) table;
}

static void parse_table_end_in_table(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    wiki_section_t* sec = meta->ctx->section;
    pop_ctx_stack(meta);
    append_section(meta,sec);
    meta->ctx->text_start = tok->stop;
  } else
    set_error(meta,tok,"NULL context inside of parse_table_end_in_table");
}

static void parse_table_end_in_table_cell(parser_meta_t* meta, wiki_token_t* tok)
{
  if (meta->ctx) {
    append_text_section(meta,meta->ctx->text_start,tok->start);
    wiki_table_t* table = meta->ctx->cur_table;
    wiki_row_t* row = meta->ctx->cur_row;
    wiki_table_cell_t* cell = (wiki_table_cell_t*) meta->ctx->section;
    TAILQ_INSERT_TAIL(&row->cells,cell,entries);
    pop_ctx_stack(meta);
    TAILQ_INSERT_TAIL(&table->rows,row,entries);
    pop_ctx_stack(meta);
    pop_ctx_stack(meta);
    append_section(meta,(wiki_section_t*) table);
    meta->ctx->text_start = tok->stop;
  } else 
    set_error(meta,tok,"NULL context inside of parse_table_end_in_table_cell");
}

/*
 * If the table style is NULL
 */
static void parse_newline_in_table(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* table_ctx = meta->ctx;
  wiki_table_t* table = (wiki_table_t*) table_ctx->section;
  if (!table_ctx->newline_encountered) {
    table_ctx->newline_encountered = true;
    table->style = make_stripped_str_from_begin_and_end(table_ctx->text_start,tok->start);
  }
}

static wiki_section_t* build_table_caption()
{
  wiki_table_caption_t* caption = (wiki_table_caption_t*) calloc(1,sizeof(wiki_table_caption_t));
  TAILQ_INIT(&caption->sections);
  ((wiki_section_t*) caption)->type = TABLE_CAPTION_SECTION;
  return (wiki_section_t*) caption;
}

/* 
 * When we see a seperator inside a table_caption it is seperating a style
 * part or it is on a newlink
 */
static void parse_separator_in_table_caption(parser_meta_t* meta, wiki_token_t* tok)
{
  wiki_table_caption_t* cap = (wiki_table_caption_t*) meta->ctx->section;
  cap->style = make_str_from_begin_and_end(meta->ctx->text_start, tok->start);
  meta->ctx->text_start = tok->stop;
}

static void parse_newline_in_table_row(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* table_row_ctx = meta->ctx;
  if (!table_row_ctx->newline_encountered) {
    table_row_ctx->newline_encountered = true;
  }
}

static wiki_section_t* build_table_row()
{
  wiki_row_t* row= (wiki_row_t*) calloc(1,sizeof(wiki_row_t));
  TAILQ_INIT(&row->cells);
  ((wiki_section_t*) row)->type = TABLE_ROW_SECTION;
  return (wiki_section_t*) row;
}

/* 
 * This looks to see if it is a leading pipe in a line, and if it is, we can
 * safely begin a table cell context!
 */
static void parse_separator_in_table_row(parser_meta_t* meta, wiki_token_t* tok)
{
  if (has_only_leading_whitespace_in_ctx(meta->ctx,tok)) {
    ctx_t* parent = meta->ctx;
    wiki_row_t* row = (wiki_row_t*) parent->section;
    row->style = make_stripped_str_from_begin_and_end(parent->text_start,tok->start);
    wiki_table_cell_t* cell = (wiki_table_cell_t*) build_table_cell();
    ctx_t* cell_ctx = push_ctx_stack(meta,TABLE_CELL_CTX,(wiki_section_t*) cell,&cell->content,tok);
    cell_ctx->cur_table = parent->cur_table;
    cell_ctx->cur_row = parent->cur_row;
  }
}

/*
 * Append a cell
 */
static void parse_separator_in_table_cell(parser_meta_t* meta, wiki_token_t* tok)
{
  if (has_only_leading_whitespace_in_ctx(meta->ctx,tok)) {
    append_text_section(meta,meta->ctx->text_start,tok->start);
    wiki_row_t* row = meta->ctx->cur_row;
    wiki_table_cell_t* cell = (wiki_table_cell_t*) meta->ctx->section;
    TAILQ_INSERT_TAIL(&row->cells,cell,entries);
    pop_ctx_stack(meta);

    ctx_t* row_ctx = meta->ctx;

    wiki_table_cell_t* new_cell = (wiki_table_cell_t*) build_table_cell();
    ctx_t* cell_ctx = push_ctx_stack(meta,TABLE_CELL_CTX,(wiki_section_t*) new_cell,&new_cell->content,tok);
    cell_ctx->cur_table = row_ctx->cur_table;
    cell_ctx->cur_row = row_ctx->cur_row;
  } else {
    wiki_table_cell_t* cell = (wiki_table_cell_t*) meta->ctx->section;
    cell->style = make_stripped_str_from_begin_and_end(meta->ctx->text_start,tok->start);
    meta->ctx->text_start = tok->stop;
  }
}

/*
 * Some tables will end like this
 *
 * |-
 * |}
 *
 * so we need to detect that empty last row and finish it up without appending
 * it.
 */
static void parse_table_end_in_table_row(parser_meta_t* meta, wiki_token_t* tok)
{
  ctx_t* table_row_ctx = meta->ctx;
  wiki_row_t* row = (wiki_row_t*) table_row_ctx->section;
  wiki_table_t* table = meta->ctx->cur_table;
  free_table_row(row);
  pop_ctx_stack(meta);
  pop_ctx_stack(meta);
  append_section(meta,(wiki_section_t*) table);
  meta->ctx->text_start = tok->stop;
}

/*
 * This looks behind the current token in the given ctx and sees if it is the
 * first token on the line.  This is important for lists and tables where
 * "leadingness" is important.
 */
static bool has_only_leading_whitespace_in_ctx(ctx_t* ctx, wiki_token_t* tok)
{
  if (ctx) {
    char* first_char_in_ctx = ctx->text_start;
    char* first_char_in_tok = tok->start;
    char* cur = first_char_in_tok - 1;
    while(isspace(*cur) && cur >= first_char_in_ctx) {
      if (*cur == '\n')
        return true;
      cur = cur - 1;
    }
  }
  return false;
}

static wiki_section_t* build_table_cell()
{
  wiki_table_cell_t* cell = (wiki_table_cell_t*) calloc(1,sizeof(wiki_table_cell_t));
  TAILQ_INIT(&cell->content);
  ((wiki_section_t*) cell)->type = TABLE_CELL_SECTION;
  return (wiki_section_t*) cell;
}

static void free_table(wiki_table_t* table)
{
  if(table->caption) {
    free_table_caption(table->caption);
  }
  if (table->style)
    free(table->style);
  wiki_row_t* row = NULL;
  while((row= TAILQ_FIRST(&table->rows))) {
    TAILQ_REMOVE(&table->rows,row,entries);
    free_table_row(row);
  }
  free(table);
}

static void free_table_caption(wiki_table_caption_t* cap)
{
  if (cap->style)
    free(cap->style);
  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&cap->sections))) {
    TAILQ_REMOVE(&cap->sections,sec,entries);
    free_section(sec);
  }
  free(cap);
}

static void free_table_row(wiki_row_t* row)
{
  if (row->style) {
    //printf("Freeing Row Style %s\n", row->style);
    free(row->style);
  }

  wiki_table_cell_t* cell = NULL;
  while ((cell = TAILQ_FIRST(&row->cells))) {
    TAILQ_REMOVE(&row->cells,cell,entries);
    free_table_cell(cell);
  }
  free(row);
}

static void free_table_cell(wiki_table_cell_t* cell)
{
  if (cell->style)
    free(cell->style);

  wiki_section_t* sec = NULL;
  while((sec = TAILQ_FIRST(&cell->content))) {
    TAILQ_REMOVE(&cell->content,sec,entries);
    free_section(sec);
  }

  free(cell);
}

static char* context_name_to_string(ctx_type_t ctx_type)
{
  switch(ctx_type) {
    case IMAGE_CTX:
      return (char*) "IMAGE_CTX";
    case INTERNAL_LINK_CTX:
      return (char*) "INTERNAL_LINK_CTX";
    case EXTERNAL_LINK_CTX:
      return (char*) "EXTERNAL_LINK_CTX";
    case TEMPLATE_CTX:
      return (char*) "TEMPLATE_CTX";
    case PARAMETER_CTX:
      return (char*) "PARAMETER_CTX";
    case VALUE_CTX:
      return (char*) "VALUE_CTX";
    case EXTERNAL_LINK_VALUE_CTX:
      return (char*) "EXTERNAL_LINK_VALUE_CTX";
    case IGNORE_CTX:
      return (char*) "IGNORE_CTX";
    case ROOT_CTX:
      return (char*) "ROOT_CTX";
    case NOWIKI_CTX:
      return (char*) "NOWIKI_CTX";
    case PRE_CTX:
      return (char*) "PRE_CTX";
    case REF_CTX:
      return (char*) "REF_CTX";
    case MATH_CTX:
      return (char*) "MATH_CTX";
    case HEADING_CTX:
      return (char*) "HEADING_CTX";
    case FORMATTING_CTX:
      return (char*) "FORMATTING_CTX";
    case CATEGORY_LINK_CTX:
      return (char*) "CATEGORY_LINK_CTX";
    case LANGUAGE_LINK_CTX:
      return (char*) "LANGUAGE_LINK_CTX";
    case INTERNAL_LINK_ANCHOR_TEXT_CTX:
      return (char*) "INTERNAL_LINK_ANCHOR_TEXT_CTX";
    case GALLERY_CTX:
      return (char*) "GALLERY_CTX";
    case TABLE_CTX:
      return (char*) "TABLE_CTX";
    case TABLE_ROW_CTX:
      return (char*) "TABLE_ROW_CTX";
    case TABLE_CAPTION_CTX:
      return (char*) "TABLE_CAPTION_CTX";
    case TABLE_CELL_CTX:
      return (char*) "TABLE_CELL_CTX";
    case SOURCE_CTX:
      return (char*) "SOURCE_CTX";
    default:
      return (char*) "UNKNOWN_CTX";
  }
}
