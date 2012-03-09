#include <stdio.h>
#include <stdlib.h>
#include <pillowtalk.h>
#include "sequential_tree_json_outputter.h"

static pt_node_t* build_section(wiki_section_t* sec);
static pt_node_t* build_text(wiki_text_t* txt);
static pt_node_t* build_internal_link(wiki_link_t* link);
static pt_node_t* build_external_link(wiki_external_link_t* link);
static pt_node_t* build_redirect(wiki_redirect_t* redirect);
static pt_node_t* build_template(wiki_template_t* tpl);
static pt_node_t* build_image(wiki_image_t* img);
static pt_node_t* build_key_values(wiki_key_value_list_t* kv_pairs);
static pt_node_t* build_pre(wiki_pre_t* pre);
static pt_node_t* build_gallery(wiki_gallery_t* gallery);
static pt_node_t* build_ref(wiki_ref_t* ref);
static pt_node_t* build_math(wiki_math_t* math);
static pt_node_t* build_source(wiki_source_t* source);
static pt_node_t* build_heading(wiki_heading_t* heading);
static pt_node_t* build_formatting(wiki_formatting_t* heading);
static pt_node_t* build_category(wiki_category_link_t* cat);
static pt_node_t* build_language_link(wiki_language_link_t* ll);
static pt_node_t* build_table(wiki_table_t* table);
static pt_node_t* build_table_row(wiki_row_t* row);
static pt_node_t* build_table_caption(wiki_table_caption_t* cap);
static pt_node_t* build_table_cell(wiki_table_cell_t* cell);
static pt_node_t* build_error(wiki_seq_tree_t* tree);

static pt_node_t* null_safe_string_node(const char* str);

char* wiki_seq_tree_to_json(wiki_seq_tree_t* tree, int beautify)
{
  wiki_section_t* sec = NULL;
  pt_node_t* root = pt_array_new();
  if (tree) {
    if (tree->error) {
      if (tree->error_str) {
        pt_node_t* error_node = build_error(tree);
        pt_array_push_back(root,error_node);
      }
    } else {
      TAILQ_FOREACH(sec,&tree->sections,entries) {
        pt_node_t* node = build_section(sec);
        pt_array_push_back(root,node);
      }
    }
  } else {
    // Failure
  }
  char* json_str = pt_to_json(root,beautify);
  pt_free_node(root);
  return json_str;
}

/***********************
 * Private Implementation
 ***********************/

static pt_node_t* build_section(wiki_section_t* sec)
{
  if (sec) {
    switch(sec->type) {
      case TEXT: 
        return build_text((wiki_text_t*) sec);

      case INTERNAL_LINK: 
        return build_internal_link((wiki_link_t*) sec);

      case EXTERNAL_LINK:
        return build_external_link((wiki_external_link_t*) sec);

      case IMAGE:
        return build_image((wiki_image_t*) sec);

      case REDIRECT_SECTION:
        return build_redirect((wiki_redirect_t*) sec);

      case TEMPLATE:
        return build_template((wiki_template_t*) sec);

      case PRE:
        return build_pre((wiki_pre_t*) sec);

      case MATH_SECTION:
        return build_math((wiki_math_t*) sec);

      case REF_SECTION:
        return build_ref((wiki_ref_t*) sec);

      case SOURCE_SECTION:
        return build_source((wiki_source_t*) sec);

      case HEADING_SECTION:
        return build_heading((wiki_heading_t*) sec);

      case FORMATTING_SECTION:
        return build_formatting((wiki_formatting_t*) sec);

      case CATEGORY_LINK_SECTION:
        return build_category((wiki_category_link_t*) sec);

      case LANGUAGE_LINK_SECTION:
        return build_language_link((wiki_language_link_t*) sec);

      case GALLERY_SECTION:
        return build_gallery((wiki_gallery_t*) sec);
        
      case TABLE_SECTION:
        return build_table((wiki_table_t*) sec);

      default:
        printf("Unknown Section! %d\n", (int) sec->type);
        return NULL;
    }
  } else {
    return NULL;
  }
}

static pt_node_t* build_text(wiki_text_t* txt)
{
  return pt_string_new(txt->text);
}

static pt_node_t* build_internal_link(wiki_link_t* link)
{
  pt_node_t* link_map = pt_map_new();
  pt_map_set(link_map,"type",pt_string_new("internal_link"));
  pt_map_set(link_map,"target",null_safe_string_node(link->target));
  if (link->alias) {
    pt_map_set(link_map,"anchor_text",pt_string_new(link->alias));
  }
  return link_map;
}

static pt_node_t* build_redirect(wiki_redirect_t* redirect)
{
  pt_node_t* red_map = pt_map_new();
  pt_map_set(red_map,"type",pt_string_new("redirect"));
  pt_map_set(red_map,"target", null_safe_string_node(redirect->link->target));
  return red_map;
}

static pt_node_t* build_template(wiki_template_t* tpl)
{
  pt_node_t* tpl_map = pt_map_new();
  pt_map_set(tpl_map,"type",pt_string_new("template"));
  pt_map_set(tpl_map,"title",null_safe_string_node(tpl->title));
  pt_node_t* kvs = build_key_values(&tpl->kv_pairs);
  if (kvs)
    pt_map_set(tpl_map,"parameters",kvs);
  return tpl_map;
}

static pt_node_t* build_image(wiki_image_t* img)
{
  pt_node_t* img_map = pt_map_new();
  pt_map_set(img_map,"type",pt_string_new("image"));
  pt_map_set(img_map,"image",null_safe_string_node(img->href));
  pt_node_t* kvs = build_key_values(&img->kv_pairs);
  if (kvs)
    pt_map_set(img_map,"parameters",kvs);
  return img_map;
}

static pt_node_t* build_key_values(wiki_key_value_list_t* kv_pairs)
{
  if (TAILQ_FIRST(kv_pairs)) {
    pt_node_t* kv_map = pt_map_new();
    wiki_kv_pair_t* kv = NULL;
    TAILQ_FOREACH(kv,kv_pairs,entries) {
      wiki_section_t* sec = NULL;
      if (TAILQ_FIRST(&kv->sections) == TAILQ_LAST(&kv->sections,wiki_section_head)) {
        sec = TAILQ_FIRST(&kv->sections);
        pt_node_t* section_json = build_section(sec);
        pt_map_set(kv_map,kv->key,section_json);
      } else {
        pt_node_t* value_array = pt_array_new();
        wiki_section_t* sec = NULL;
        TAILQ_FOREACH(sec,&kv->sections,entries) {
          pt_node_t* section_json = build_section(sec);
          pt_array_push_back(value_array,section_json);
        }
        pt_map_set(kv_map,kv->key,value_array);
      }
    }
    return kv_map;
  } else {
    return NULL;
  }
}

static pt_node_t* build_pre(wiki_pre_t* pre)
{
  pt_node_t* pre_map = pt_map_new();
  pt_map_set(pre_map,"type",pt_string_new("pre"));
  pt_map_set(pre_map,"text",null_safe_string_node(pre->text));
  return pre_map;
}

static pt_node_t* build_ref(wiki_ref_t* ref)
{
  pt_node_t* ref_map = pt_map_new();
  pt_map_set(ref_map,"type",pt_string_new("ref"));
  pt_node_t* section_array = pt_array_new();

  wiki_section_t* sec = NULL;
  TAILQ_FOREACH(sec,&ref->sections,entries) {
    pt_node_t* section_json = build_section(sec);
    pt_array_push_back(section_array,section_json);
  }
  pt_map_set(ref_map,"value",section_array);
  return ref_map;
}

static pt_node_t* build_math(wiki_math_t* math)
{
  pt_node_t* math_map = pt_map_new();
  pt_map_set(math_map,"type",pt_string_new("math"));
  pt_map_set(math_map,"text",null_safe_string_node(math->text));
  return math_map;
}

static pt_node_t* build_source(wiki_source_t* source)
{
  pt_node_t* source_map = pt_map_new();
  pt_map_set(source_map,"type",pt_string_new("source"));
  pt_map_set(source_map,"text",null_safe_string_node(source->text));
  return source_map;
}

static pt_node_t* build_external_link(wiki_external_link_t* link)
{
  pt_node_t* link_map = pt_map_new();
  pt_map_set(link_map,"type",pt_string_new("external_link"));
  pt_map_set(link_map,"target",null_safe_string_node(link->target));
  if (!TAILQ_EMPTY(&link->sections)) {
    pt_node_t* section_array = pt_array_new();
    wiki_section_t* sec = NULL;
    TAILQ_FOREACH(sec,&link->sections,entries) {
      pt_node_t* section_json = build_section(sec);
      pt_array_push_back(section_array,section_json);
    }
    pt_map_set(link_map,"anchor_sections", section_array);
  }
  return link_map;
}

static pt_node_t* null_safe_string_node(const char* str)
{
  if (str)
    return pt_string_new(str);
  else
    return pt_null_new();
}

static pt_node_t* build_heading(wiki_heading_t* heading) 
{
  pt_node_t* heading_map = pt_map_new();
  pt_map_set(heading_map,"type",pt_string_new("heading"));

  wiki_section_t* sec = NULL;
  if (TAILQ_FIRST(&heading->sections) == TAILQ_LAST(&heading->sections,wiki_section_head)) {
    sec = TAILQ_FIRST(&heading->sections);
    pt_node_t* section_json = build_section(sec);
    pt_map_set(heading_map,"value",section_json);
  } else {
    pt_node_t* section_array = pt_array_new();
    TAILQ_FOREACH(sec,&heading->sections,entries) {
      pt_node_t* section_json = build_section(sec);
      pt_array_push_back(section_array,section_json);
    }
    pt_map_set(heading_map,"value",section_array);
  }
  return heading_map;
}

static pt_node_t* build_formatting(wiki_formatting_t* formatting) 
{
  pt_node_t* formatting_map = pt_map_new();
  pt_map_set(formatting_map,"type",pt_string_new("formatting"));

  wiki_section_t* sec = NULL;
  if (TAILQ_FIRST(&formatting->sections) == TAILQ_LAST(&formatting->sections,wiki_section_head)) {
    sec = TAILQ_FIRST(&formatting->sections);
    pt_node_t* section_json = build_section(sec);
    pt_map_set(formatting_map,"value",section_json);
  } else {
    pt_node_t* section_array = pt_array_new();
    TAILQ_FOREACH(sec,&formatting->sections,entries) {
      pt_node_t* section_json = build_section(sec);
      pt_array_push_back(section_array,section_json);
    }
    pt_map_set(formatting_map,"value",section_array);
  }
  return formatting_map;
}

static pt_node_t* build_category(wiki_category_link_t* cat)
{
  pt_node_t* cat_map = pt_map_new();
  pt_map_set(cat_map,"type",pt_string_new("category"));
  pt_map_set(cat_map,"category",null_safe_string_node(cat->category));
  return cat_map;
}

static pt_node_t* build_language_link(wiki_language_link_t* ll)
{
  pt_node_t* lang_map = pt_map_new();
  pt_map_set(lang_map,"type",pt_string_new("language_link"));
  pt_map_set(lang_map,"language",null_safe_string_node(ll->language));
  pt_map_set(lang_map,"target",null_safe_string_node(ll->target));
  return lang_map;
}

static pt_node_t* build_gallery(wiki_gallery_t* gallery)
{
  pt_node_t* gallery_map = pt_map_new();
  pt_map_set(gallery_map,"type",pt_string_new("gallery"));
  pt_map_set(gallery_map,"text",null_safe_string_node(gallery->text));
  return gallery_map;
}

static pt_node_t* build_error(wiki_seq_tree_t* tree)
{
  pt_node_t* error_map = pt_map_new();
  pt_map_set(error_map,"type",pt_string_new("error"));
  if (tree && tree->error) {
    pt_map_set(error_map,"string",null_safe_string_node(tree->error_str));
    pt_map_set(error_map,"location",null_safe_string_node(tree->error_loc));
  }
  return error_map;
}

static pt_node_t* build_table(wiki_table_t* table)
{
  pt_node_t* table_map = pt_map_new();
  pt_map_set(table_map,"type",pt_string_new("table"));
  pt_map_set(table_map,"style",null_safe_string_node(table->style));

  if(table->caption) {
    pt_node_t* caption = build_table_caption(table->caption);
    pt_map_set(table_map,"caption",caption);
  }
  pt_node_t* rows = pt_array_new();
  wiki_row_t* row = NULL;
  TAILQ_FOREACH(row,&table->rows,entries) {
    pt_node_t* row_json = build_table_row(row);
    pt_array_push_back(rows,row_json);
  }
  pt_map_set(table_map,"rows",rows);
  return table_map;
}

static pt_node_t* build_table_caption(wiki_table_caption_t* cap)
{
  pt_node_t* cap_map = pt_map_new();
  pt_map_set(cap_map,"type",pt_string_new("table_caption"));
  pt_map_set(cap_map,"style",null_safe_string_node(cap->style));

  pt_node_t* section_array = pt_array_new();
  wiki_section_t* sec = NULL;
  TAILQ_FOREACH(sec,&cap->sections,entries) {
    pt_node_t* section_json = build_section(sec);
    pt_array_push_back(section_array,section_json);
  }

  pt_map_set(cap_map,"sections",section_array);
  return cap_map;
}

static pt_node_t* build_table_row(wiki_row_t* row)
{
  pt_node_t* row_map = pt_map_new();
  pt_map_set(row_map,"type",pt_string_new("row"));
  pt_map_set(row_map,"style",null_safe_string_node(row->style));

  pt_node_t* cells = pt_array_new();
  wiki_table_cell_t* cell = NULL;
  TAILQ_FOREACH(cell,&row->cells,entries) {
    pt_node_t* cell_json = build_table_cell(cell);
    pt_array_push_back(cells,cell_json);
  }
  pt_map_set(row_map,"cells",cells);
  return row_map;
}

static pt_node_t* build_table_cell(wiki_table_cell_t* cell)
{
  pt_node_t* cell_map = pt_map_new();
  pt_map_set(cell_map,"type",pt_string_new("cell"));
  pt_map_set(cell_map,"style",null_safe_string_node(cell->style));
  pt_node_t* content = pt_array_new();
  wiki_section_t* sec = NULL;
  if (cell->header)
    pt_map_set(cell_map,"header", pt_bool_new(1));

  TAILQ_FOREACH(sec,&cell->content,entries) {
    pt_node_t* node = build_section(sec);
    pt_array_push_back(content,node);
  }

  pt_map_set(cell_map,"content",content);
  return cell_map;
}
