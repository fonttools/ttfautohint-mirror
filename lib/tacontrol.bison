/* tacontrol.bison */

/*
 * Copyright (C) 2014-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/*
 * grammar for parsing ttfautohint control instructions
 *
 * Parsing errors that essentially belong to the lexing stage are handled
 * with `store_error_data'; in case the lexer detects an error, it returns
 * the right token type but sets `context->error'.  Syntax errors and fatal
 * lexer errors (token `INTERNAL_FLEX_ERROR') are handled with
 * `TA_control_error'.
 */

/*
 * Edsko de Vries's article `Writing a Reentrant Parser with Flex and Bison'
 * (https://web.archive.org/web/20160130155657/http://www.phpcompiler.org:80/articles/reentrantparser.html)
 * was extremely helpful in writing this code.
 */

%require "2.5" /* we use named references */

%output "tacontrol-bison.c"
%defines "tacontrol-bison.h"

%define api.pure
%error-verbose
%expect 7
%glr-parser
%lex-param { void* scanner }
%locations
%name-prefix "TA_control_"
%parse-param { Control_Context* context }

%code requires {
#include "ta.h"
#include "tashaper.h"

/* we don't change the name prefix of flex functions */
#define TA_control_lex yylex
}

%union {
  char character;
  Control_Type type;
  long integer;
  char* name;
  number_range* range;
  double real;
  Control* control;
}

%{
#include <limits.h>
#include "tacontrol-flex.h"

void
TA_control_error(YYLTYPE *locp,
                 Control_Context* context,
                 char const* msg);

void
store_error_data(const YYLTYPE *locp,
                 Control_Context* context,
                 TA_Error error);


/* calls to `yylex' in the generated bison code use `scanner' directly */
#define scanner context->scanner
%}

/* INVALID_CHARACTER and INTERNAL_FLEX_ERROR are flex errors */
%token EOE
%token <integer> INTEGER "integer number"
%token INTERNAL_FLEX_ERROR "internal flex error"
%token <character> INVALID_CHARACTER "invalid character"
%token <name> LEFT "left"
%token <name> NAME "glyph name"
%token <name> NODIR "no direction"
%token <name> POINT "point"
%token <real> REAL "real number"
%token <name> RIGHT "right"
%token <name> TOUCH "touch"
%token <name> WIDTH "width"
%token <name> XSHIFT "x shift"
%token <name> YSHIFT "y shift"

%type <control> entry
%type <integer> font_idx
%type <integer> glyph_idx
%type <range> glyph_idx_range
%type <range> glyph_idx_range_elem
%type <range> glyph_idx_range_elems
%type <range> glyph_idx_set
%type <name> glyph_name
%type <name> glyph_name_
%type <control> input
%type <integer> integer
%type <type> left_right
%type <type> left_right_
%type <type> point_touch
%type <type> point_touch_
%type <range> left_limited
%type <type> no_dir
%type <range> number_set
%type <integer> offset
%type <range> ppem_set
%type <range> range
%type <range> range_elem
%type <range> range_elems
%type <real> real
%type <range> right_limited
%type <integer> script_feature
%type <real> shift
%type <integer> signed_integer
%type <range> unlimited
%type <range> width_elem
%type <range> width_elems
%type <range> width_set
%type <integer> wildcard_script_feature
%type <real> x_shift
%type <real> y_shift

%destructor { TA_control_free($$); } <control>
%destructor { number_set_free($$); } <range>

%printer { fprintf(yyoutput, "`%ld'", $$); } <integer>
%printer { fprintf(yyoutput, "`%s'", $$); } <name>
%printer { fprintf(yyoutput, "`%g'", $$); } <real>
%printer {
           char* s;
           number_range* nr;


           nr = number_set_reverse($$);
           s = number_set_show(nr, -1, -1);
           (void)number_set_reverse(nr);
           if (s)
           {
             fprintf(yyoutput, "`%s'", s);
             free(s);
           }
           else
             fprintf(yyoutput, "allocation error");
         } <range>
%printer { fprintf(yyoutput, "`%c'", $$); } INVALID_CHARACTER


%%


/* `number_range' list elements are stored in reversed order; */
/* the call to `TA_control_new' fixes this */

/* `Control' list elements are stored in reversed order, too; */
/* this gets fixed by an explicit call to `TA_control_reverse' */

start:
  input
    { context->result = TA_control_reverse($input); }
;

input[result]:
  /* empty */
    { $result = NULL; }
| input[left] entry
    { $result = TA_control_prepend($left, $entry); }
;

entry:
  EOE
    { $entry = NULL; }
| font_idx glyph_idx point_touch number_set x_shift y_shift ppem_set EOE
    {
      $entry = TA_control_new($point_touch,
                              $font_idx,
                              $glyph_idx,
                              $number_set,
                              $x_shift,
                              $y_shift,
                              $ppem_set,
                              @$.first_line);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx glyph_idx left_right number_set EOE
    {
      $entry = TA_control_new($left_right,
                              $font_idx,
                              $glyph_idx,
                              $number_set,
                              0,
                              0,
                              NULL,
                              @$.first_line);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx glyph_idx left_right number_set '(' offset[left] ',' offset[right] ')' EOE
    {
      $entry = TA_control_new($left_right,
                              $font_idx,
                              $glyph_idx,
                              $number_set,
                              $left,
                              $right,
                              NULL,
                              @$.first_line);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx glyph_idx no_dir number_set EOE
    {
      $entry = TA_control_new($no_dir,
                              $font_idx,
                              $glyph_idx,
                              $number_set,
                              0,
                              0,
                              NULL,
                              @$.first_line);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx script_feature glyph_idx_set EOE
    {
      $entry = TA_control_new(Control_Script_Feature_Glyphs,
                              $font_idx,
                              $script_feature,
                              $glyph_idx_set,
                              0,
                              0,
                              NULL,
                              @$.first_line);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx wildcard_script_feature width_set EOE
    {
      $entry = TA_control_new(Control_Script_Feature_Widths,
                              $font_idx,
                              $wildcard_script_feature,
                              $width_set,
                              0,
                              0,
                              NULL,
                              @$.first_line);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;

font_idx:
  /* empty */
    {
      $font_idx = 0;
      context->font_idx = $font_idx;
    }
| integer
    {
      $font_idx = $integer;
      if ($font_idx >= context->font->num_sfnts)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Font_Index);
        YYABORT;
      }
      context->font_idx = $font_idx;
    }
;

glyph_idx:
  integer
    {
      FT_Face face = context->font->sfnts[context->font_idx].face;


      $glyph_idx = $integer;
      if ($glyph_idx >= face->num_glyphs)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph_Index);
        YYABORT;
      }
      context->glyph_idx = $glyph_idx;
    }
| glyph_name
    {
      FT_Face face = context->font->sfnts[context->font_idx].face;


      /* explicitly compare with `.notdef' */
      /* since `FT_Get_Name_Index' returns glyph index 0 */
      /* for both this glyph name and invalid ones */
      if (!strcmp($glyph_name, ".notdef"))
        $glyph_idx = 0;
      else
      {
        $glyph_idx = (long)FT_Get_Name_Index(face, $glyph_name);
        if ($glyph_idx == 0)
          $glyph_idx = -1;
      }

      free($glyph_name);

      if ($glyph_idx < 0)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph_Name);
        YYABORT;
      }
      context->glyph_idx = $glyph_idx;
    }
;

glyph_name:
  glyph_name_
    {
      /* `$glyph_name_' was allocated in the lexer */
      if (context->error)
      {
        /* lexing error */
        store_error_data(&@$, context, context->error);
        YYABORT;
      }

      $glyph_name = $glyph_name_;
    }
;

glyph_name_:
  LEFT
| NAME
| NODIR
| POINT
| RIGHT
| TOUCH
| WIDTH
| XSHIFT
| YSHIFT
;

point_touch:
  point_touch_
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face,
                            (FT_UInt)context->glyph_idx,
                            FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;

      $point_touch = $point_touch_;
    }
;

point_touch_:
  POINT
    {
      $point_touch_ = Control_Delta_after_IUP;
      free($POINT);
    }
| TOUCH
    {
      $point_touch_ = Control_Delta_before_IUP;
      free($TOUCH);
    }
;

left_right:
  left_right_
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face,
                            (FT_UInt)context->glyph_idx,
                            FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;

      $left_right = $left_right_;
    }
;

left_right_:
  LEFT
    {
      $left_right_ = Control_Single_Point_Segment_Left;
      free($LEFT);
    }
| RIGHT
    {
      $left_right_ = Control_Single_Point_Segment_Right;
      free($RIGHT);
    }
;

no_dir:
  NODIR
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face,
                            (FT_UInt)context->glyph_idx,
                            FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;

      $no_dir = Control_Single_Point_Segment_None;
      free($NODIR);
    }
;

wildcard_script_feature:
  script_feature
    { $wildcard_script_feature = $script_feature; }
| '*' glyph_name[feature]
    {
      size_t i;
      size_t feature_idx = 0;
      char feature_name[5];


      feature_name[4] = '\0';

      for (i = 0; i < feature_tags_size; i++)
      {
        hb_tag_to_string(feature_tags[i], feature_name);

        if (!strcmp($feature, feature_name))
        {
          feature_idx = i;
          break;
        }
      }

      free($feature);

      if (i == feature_tags_size)
      {
        store_error_data(&@2, context, TA_Err_Control_Invalid_Feature);
        YYABORT;
      }

      /* simply use negative values for features applied to all scripts */
      $wildcard_script_feature = -(long)feature_idx;
    }

script_feature:
  glyph_name[script] glyph_name[feature]
    {
      long ss;
      size_t i, j;
      size_t script_idx = 0;
      size_t feature_idx = 0;
      char feature_name[5];


      for (i = 0; i < script_names_size; i++)
      {
        if (!strcmp($script, script_names[i]))
        {
          script_idx = i;
          break;
        }
      }

      feature_name[4] = '\0';

      for (j = 0; j < feature_tags_size; j++)
      {
        hb_tag_to_string(feature_tags[j], feature_name);

        if (!strcmp($feature, feature_name))
        {
          feature_idx = j;
          break;
        }
      }

      free($script);
      free($feature);

      if (i == script_names_size)
      {
        store_error_data(&@1, context, TA_Err_Control_Invalid_Script);
        YYABORT;
      }
      if (j == feature_tags_size)
      {
        store_error_data(&@2, context, TA_Err_Control_Invalid_Feature);
        YYABORT;
      }

      for (ss = 0; ta_style_classes[ss]; ss++)
      {
        TA_StyleClass style_class = ta_style_classes[ss];


        if (script_idx == style_class->script
            && feature_idx == style_class->coverage)
        {
          $script_feature = ss;
          break;
        }
      }
      if (!ta_style_classes[ss])
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Style);
        YYABORT;
      }
    }
;

x_shift:
  /* empty */
    { $x_shift = 0; }
| XSHIFT shift
    {
      $x_shift = $shift;
      free($XSHIFT);
    }
;

y_shift:
  /* empty */
    { $y_shift = 0; }
| YSHIFT shift
    {
      $y_shift = $shift;
      free($YSHIFT);
    }
;

shift:
  real
    {
      if ($real < CONTROL_DELTA_SHIFT_MIN || $real > CONTROL_DELTA_SHIFT_MAX)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Shift);
        YYABORT;
      }
      $shift = $real;
    }
;

offset:
  signed_integer
    {
      if ($signed_integer < SHRT_MIN || $signed_integer > SHRT_MAX)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Offset);
        YYABORT;
      }
      $offset = $signed_integer;
    }
;

ppem_set:
  '@'
    {
      context->number_set_min = CONTROL_DELTA_PPEM_MIN;
      context->number_set_max = CONTROL_DELTA_PPEM_MAX;
    }
  number_set
    { $ppem_set = $number_set; }
;

integer:
  '0'
    { $integer = 0; }
| INTEGER
    {
      if (context->error)
      {
        /* lexing error */
        store_error_data(&@$, context, context->error);
        YYABORT;
      }

      $integer = $INTEGER;
    }
;

signed_integer:
  integer
    { $signed_integer = $integer; }
| '+' integer
    { $signed_integer = $integer; }
| '-' integer
    { $signed_integer = -$integer; }
;

real:
  signed_integer
    { $real = $signed_integer; }
| REAL
    {
      if (context->error)
      {
        /* lexing error */
        store_error_data(&@$, context, context->error);
        YYABORT;
      }

      $real = $REAL;
    }
| '+' REAL
    { $real = $REAL; }
| '-' REAL
    { $real = -$REAL; }
;

number_set:
  unlimited
    { $number_set = $unlimited; }
| right_limited
    { $number_set = $right_limited; }
| left_limited
    { $number_set = $left_limited; }
| range_elems
    { $number_set = $range_elems; }
| right_limited ',' range_elems
    {
      $number_set = number_set_prepend($right_limited, $range_elems);
      if ($number_set == NUMBERSET_NOT_ASCENDING)
      {
        number_set_free($right_limited);
        number_set_free($range_elems);
        store_error_data(&@3, context, TA_Err_Control_Ranges_Not_Ascending);
        YYABORT;
      }
      if ($number_set == NUMBERSET_OVERLAPPING_RANGES)
      {
        number_set_free($right_limited);
        number_set_free($range_elems);
        store_error_data(&@3, context, TA_Err_Control_Overlapping_Ranges);
        YYABORT;
      }
    }
| range_elems ',' left_limited
    {
      $number_set = number_set_prepend($range_elems, $left_limited);
      if ($number_set == NUMBERSET_NOT_ASCENDING)
      {
        number_set_free($range_elems);
        number_set_free($left_limited);
        store_error_data(&@3, context, TA_Err_Control_Ranges_Not_Ascending);
        YYABORT;
      }
      if ($number_set == NUMBERSET_OVERLAPPING_RANGES)
      {
        number_set_free($range_elems);
        number_set_free($left_limited);
        store_error_data(&@3, context, TA_Err_Control_Overlapping_Ranges);
        YYABORT;
      }
    }
;

unlimited:
  '-'
    {
      $unlimited = number_set_new(context->number_set_min,
                                  context->number_set_max,
                                  context->number_set_min,
                                  context->number_set_max);
      /* range of `$unlimited' is always valid */
      if ($unlimited == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;

right_limited:
  '-' integer
    {
      $right_limited = number_set_new(context->number_set_min,
                                      (int)$integer,
                                      context->number_set_min,
                                      context->number_set_max);
      if ($right_limited == NUMBERSET_INVALID_RANGE)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Range);
        YYABORT;
      }
      if ($right_limited == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;

left_limited:
  integer '-'
    {
      $left_limited = number_set_new((int)$integer,
                                     context->number_set_max,
                                     context->number_set_min,
                                     context->number_set_max);
      if ($left_limited == NUMBERSET_INVALID_RANGE)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Range);
        YYABORT;
      }
      if ($left_limited == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;

range_elems[result]:
  range_elem
    { $result = $range_elem; }
| range_elems[left] ',' range_elem
    {
      $result = number_set_prepend($left, $range_elem);
      if ($result == NUMBERSET_NOT_ASCENDING)
      {
        number_set_free($left);
        number_set_free($range_elem);
        store_error_data(&@3, context, TA_Err_Control_Ranges_Not_Ascending);
        YYABORT;
      }
      if ($result == NUMBERSET_OVERLAPPING_RANGES)
      {
        number_set_free($left);
        number_set_free($range_elem);
        store_error_data(&@3, context, TA_Err_Control_Overlapping_Ranges);
        YYABORT;
      }
    }
;

range_elem:
  integer
    {
      $range_elem = number_set_new((int)$integer,
                                   (int)$integer,
                                   context->number_set_min,
                                   context->number_set_max);
      if ($range_elem == NUMBERSET_INVALID_RANGE)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Range);
        YYABORT;
      }
      if ($range_elem == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| range
    { $range_elem = $range; }
;

range:
  integer[left] '-' integer[right]
    {
      $range = number_set_new((int)$left,
                              (int)$right,
                              context->number_set_min,
                              context->number_set_max);
      if ($range == NUMBERSET_INVALID_RANGE)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Range);
        YYABORT;
      }
      if ($range == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;

glyph_idx_set:
  '@'
    {
      FT_Face face = context->font->sfnts[context->font_idx].face;


      context->number_set_min = 0;
      context->number_set_max = (int)(face->num_glyphs - 1);
    }
  glyph_idx_range_elems
    { $glyph_idx_set = $glyph_idx_range_elems; }
;

glyph_idx_range_elems[result]:
  glyph_idx_range_elem
    { $result = $glyph_idx_range_elem; }
| glyph_idx_range_elems[left] ',' glyph_idx_range_elem
    {
      /* for glyph_idx_set, ascending order is not enforced */
      $result = number_set_insert($left, $glyph_idx_range_elem);
      if ($result == NUMBERSET_OVERLAPPING_RANGES)
      {
        number_set_free($left);
        number_set_free($glyph_idx_range_elem);
        store_error_data(&@3, context, TA_Err_Control_Overlapping_Ranges);
        YYABORT;
      }
    }
;

glyph_idx_range_elem:
  glyph_idx
    {
      $glyph_idx_range_elem = number_set_new((int)$glyph_idx,
                                             (int)$glyph_idx,
                                             context->number_set_min,
                                             context->number_set_max);
      /* glyph_idx is always valid */
      /* since its value was already tested for validity */
      if ($glyph_idx_range_elem == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| glyph_idx_range
    { $glyph_idx_range_elem = $glyph_idx_range; }
;

glyph_idx_range:
  glyph_idx[left] '-' glyph_idx[right]
    {
      $glyph_idx_range = number_set_new((int)$left,
                                        (int)$right,
                                        context->number_set_min,
                                        context->number_set_max);
      /* glyph range is always valid */
      /* since both `glyph_idx' values were already tested for validity */
      if ($glyph_idx_range == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;

width_set:
  WIDTH
    {
      context->number_set_min = 1;
      context->number_set_max = 65535;
      context->number_set_num_elems = 0;
    }
  width_elems
    { $width_set = $width_elems; }
;

width_elems[result]:
  width_elem
    {
      context->number_set_num_elems++;
      $result = $width_elem;
    }
| width_elems[left] ',' width_elem
    {
      context->number_set_num_elems++;
      if (context->number_set_num_elems > TA_LATIN_MAX_WIDTHS)
      {
        number_set_free($left);
        number_set_free($width_elem);
        store_error_data(&@3, context, TA_Err_Control_Too_Much_Widths);
        YYABORT;
      }

      /* for width_set, the order of entries is preserved */
      $result = number_set_prepend_unsorted($left, $width_elem);
    }
;

width_elem:
  integer
    {
      $width_elem = number_set_new($integer,
                                   $integer,
                                   context->number_set_min,
                                   context->number_set_max);
      if ($width_elem == NUMBERSET_ALLOCATION_ERROR)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
;


%%


void
TA_control_error(YYLTYPE *locp,
                 Control_Context* context,
                 char const* msg)
{
  /* if `error' is already set, we have a fatal flex error */
  if (!context->error)
  {
    context->error = TA_Err_Control_Syntax_Error;
    strncpy(context->errmsg, msg, sizeof (context->errmsg));
  }

  context->errline_num = locp->first_line;
  context->errline_pos_left = locp->first_column;
  context->errline_pos_right = locp->last_column;
}


void
store_error_data(const YYLTYPE *locp,
                 Control_Context* context,
                 TA_Error error)
{
  context->error = error;

  context->errline_num = locp->first_line;
  context->errline_pos_left = locp->first_column;
  context->errline_pos_right = locp->last_column;

  context->errmsg[0] = '\0';
}


#if 0

/*
 * compile this test program with
 *
 *   make libnumberset.la
 *
 *   flex -d tacontrol.flex \
 *   && bison -t tacontrol.bison \
 *   && gcc -g3 -O0 \
 *          -Wall -W \
 *          -I.. \
 *          -I. \
 *          -I/usr/local/include/freetype2 \
 *          -I/usr/local/include/harfbuzz \
 *          -L.libs \
 *          -o tacontrol-bison \
 *          tacontrol-bison.c \
 *          tacontrol-flex.c \
 *          tacontrol.c \
 *          -lfreetype \
 *          -lnumberset
 */

const char* input =
  "# Test\n"
  "\n"
  "# 0 a p 1-3 x 0.3 y -0.2 @ 20-30; \\\n"
  "0 exclam p 2-4 x 0.7 y -0.4 @ 6,7,9,10,11; \\\n"
  "a p / 12 x 0.5 @ 23-25";


#undef scanner

int
main(int argc,
     char** argv)
{
  TA_Error error;
  int bison_error;
  int retval = 1;

  Control_Context context;
  FONT font;
  SFNT sfnts[1];
  FT_Library library;
  FT_Face face;
  const char* filename;


  if (argc != 2)
  {
    fprintf(stderr, "need an outline font as an argument\n");
    goto Exit0;
  }

  filename = argv[1];

  error = FT_Init_FreeType(&library);
  if (error)
  {
    fprintf(stderr, "error while initializing FreeType library (0x%X)\n",
                    error);
    goto Exit0;
  }

  error = FT_New_Face(library, filename, 0, &face);
  if (error)
  {
    fprintf(stderr, "error while loading font `%s' (0x%X)\n",
                    filename,
                    error);
    goto Exit1;
  }

  /* we construct a minumum `Font' object */
  sfnts[0].face = face;

  font.num_sfnts = 1;
  font.sfnts = sfnts;
  font.control_buf = (char*)input;
  font.control_len = strlen(input);

  TA_control_debug = 1;

  TA_control_scanner_init(&context, &font);
  if (context.error)
    goto Exit2;

  bison_error = TA_control_parse(&context);
  if (bison_error)
    goto Exit3;

  retval = 0;

Exit3:
  TA_control_scanner_done(&context);
  TA_control_free(context.result);

Exit2:
  FT_Done_Face(face);

Exit1:
  FT_Done_FreeType(library);

Exit0:
  return retval;
}

#endif

/* end of tacontrol.bison */
