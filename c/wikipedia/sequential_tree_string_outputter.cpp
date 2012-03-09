#include <sstream>
#include <string.h>

#include "sequential_tree_string_outputter.h"

using namespace std;

/* Prototypes */
static void indent(stringstream& strm, int spaces);
static void output_text(stringstream& strm, wiki_text_t* txt, int indent_count);
static void output_section(stringstream& strm, wiki_section_t* sec, int indent_count = 0);
static void output_redirect(stringstream& strm, wiki_redirect_t* redirect, int indent_count = 0);
static void output_template(stringstream& strm, wiki_template_t* tpl, int indent_count = 0);
static void output_image(stringstream& strm, wiki_image_t* img, int indent_count);
static void output_key_values(stringstream& strm,wiki_key_value_list_t* kv_pairs,int indent_count);
static void output_pre(stringstream& strm, wiki_pre_t* pre, int indent_count);
static void output_ref(stringstream& strm, wiki_ref_t* ref, int indent_count);
static void output_math(stringstream& strm, wiki_math_t* math, int indent_count);
static void output_source(stringstream& strm, wiki_source_t* source, int indent_count);

static inline int min(int x, int y)
{
  if (x > y)
    return y;
  else
    return x;
}

char* wiki_seq_tree_to_string(wiki_seq_tree_t* tree)
{
  wiki_section_t* sec = NULL;
  stringstream strm;
  if (tree) {
    if (tree->error) {
      strm << "Parse Error" << endl;
      if (tree->error_str)
        strm << tree->error_str << endl;
      if (tree->error_loc)
        strm << string(tree->error_loc,min(strlen(tree->error_loc),100)) << endl;
    } else {
      strm << "Parse Success" << endl;
      TAILQ_FOREACH(sec,&tree->sections,entries) {
        output_section(strm,sec,0);
      }
    }
  } else {
    strm << "NULL Tree Parse Failure" << endl;
  }

  return strdup(strm.str().c_str());
}

/* Private Implementation */
static void indent(stringstream& strm, int spaces)
{
  for(int i=0; i < spaces; i++) {
    strm << " ";
  }
}

static void output_text(stringstream& strm, wiki_text_t* txt, int indent_count)
{
  indent(strm,indent_count);
  strm << "Text:" << txt->text << endl;
}

static void output_link(stringstream& strm, wiki_link_t* link, int indent_count)
{
  indent(strm,indent_count);
  strm << "Internal Link: [[" << link->target;
  if (link->alias)
    strm << "|" << link->alias;
  strm << "]]" << endl;
}

static void output_template(stringstream& strm, wiki_template_t* tpl, int indent_count)
{
  indent(strm,indent_count);
  strm << "Template: " << tpl->title << endl;
  output_key_values(strm,&tpl->kv_pairs,indent_count + 2);
}

static void output_redirect(stringstream& strm, wiki_redirect_t* redirect, int indent_count)
{
  indent(strm,indent_count);
  wiki_link_t* link = redirect->link;
  strm << "Redirect: [[" << link->target << "]]" << endl;
}

static void output_image(stringstream& strm, wiki_image_t* img, int indent_count)
{
  indent(strm,indent_count);
  strm << "Image: " << img->href << endl;
  output_key_values(strm,&img->kv_pairs,indent_count + 2);
}

static void output_key_values(stringstream& strm,wiki_key_value_list_t* kv_pairs,int indent_count)
{
  wiki_kv_pair_t* kv = NULL;
  TAILQ_FOREACH(kv,kv_pairs,entries) {
    indent(strm,indent_count + 2);
    strm << kv->key << ": {" << endl;
    wiki_section_t* sec = NULL;
    TAILQ_FOREACH(sec,&kv->sections,entries) {
      output_section(strm,sec,indent_count + 4);
    }
    indent(strm,indent_count + 2);
    strm << "}" << endl;
  }
}

static void output_section(stringstream& strm, wiki_section_t* sec, int indent_count)
{
  switch(sec->type) {
    case TEXT: 
      output_text(strm,(wiki_text_t*) sec,indent_count);
      break;
    case INTERNAL_LINK: 
      output_link(strm,(wiki_link_t*) sec,indent_count);
      break;

    case IMAGE:
      output_image(strm,(wiki_image_t*) sec,indent_count);
      break;

    case REDIRECT_SECTION:
      output_redirect(strm,(wiki_redirect_t*) sec,indent_count);
      break;

    case TEMPLATE:
      output_template(strm,(wiki_template_t*) sec, indent_count);
      break;

    case PRE:
      output_pre(strm,(wiki_pre_t*) sec, indent_count);
      break;

    case MATH_SECTION:
      output_math(strm,(wiki_math_t*) sec, indent_count);
      break;

    case REF_SECTION:
      output_ref(strm,(wiki_ref_t*) sec, indent_count);
      break;

    case SOURCE_SECTION:
      output_source(strm,(wiki_source_t*) sec, indent_count);
      break;

    default:
      strm << "Unknown Section:" << sec->type << endl;
      break;
  }
}

static void output_pre(stringstream& strm, wiki_pre_t* pre, int indent_count)
{
  indent(strm,indent_count);
  strm << "Pre:" << pre->text << endl;
}

static void output_ref(stringstream& strm, wiki_ref_t* ref, int indent_count)
{
  indent(strm,indent_count);
  strm << "Ref:" << endl;
  wiki_section_t* sec = NULL;
  TAILQ_FOREACH(sec,&ref->sections,entries) {
    output_section(strm,sec,indent_count + 2);
  }
}

static void output_math(stringstream& strm, wiki_math_t* math, int indent_count)
{
  indent(strm,indent_count);
  strm << "Math:" << math->text << endl;
}

static void output_source(stringstream& strm, wiki_source_t* source, int indent_count)
{
  indent(strm,indent_count);
  strm << "Source:" << source->text << endl;
}
