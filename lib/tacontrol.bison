/* tacontrol.bison */

/*
 * Copyright (C) 2014 by Werner Lemberg.
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
 * (http://www.phpcompiler.org/articles/reentrantparser.html)
 * was extremely helpful in writing this code.
 */

%output "tacontrol-bison.c"
%defines "tacontrol-bison.h"

%define api.pure
%error-verbose
%expect 2
%glr-parser
%lex-param { void* scanner }
%locations
%name-prefix "TA_control_"
%parse-param { Control_Context* context }
%require "2.5" /* note that some versions < 2.7 use `//' in comments */

%code requires {
#include "ta.h"

/* we don't change the name prefix of flex functions */
#define TA_control_lex yylex
}

%union {
  char character;
  long integer;
  char* name;
  number_range* range;
  double real;
  Control* control;
}

%{
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
%token <name> NAME "glyph name"
%token <real> REAL "real number"

%type <control> entry
%type <integer> font_idx
%type <integer> glyph_idx
%type <name> glyph_name
%type <control> input
%type <integer> integer
%type <range> left_dir_set
%type <range> left_limited
%type <range> none_dir_set
%type <range> number_set
%type <range> point_set
%type <range> ppem_set
%type <range> range
%type <range> range_elem
%type <range> range_elems
%type <real> real
%type <range> right_dir_set
%type <range> right_limited
%type <real> shift
%type <range> unlimited
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
| font_idx glyph_idx point_set x_shift y_shift ppem_set EOE
    {
      $entry = TA_control_new(Control_Delta_after_IUP,
                              $font_idx,
                              $glyph_idx,
                              $point_set,
                              $x_shift,
                              $y_shift,
                              $ppem_set);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx glyph_idx left_dir_set EOE
    {
      $entry = TA_control_new(Control_Point_Dir,
                              $font_idx,
                              $glyph_idx,
                              $left_dir_set,
                              TA_DIR_LEFT,
                              0,
                              NULL);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx glyph_idx right_dir_set EOE
    {
      $entry = TA_control_new(Control_Point_Dir,
                              $font_idx,
                              $glyph_idx,
                              $right_dir_set,
                              TA_DIR_RIGHT,
                              0,
                              NULL);
      if (!$entry)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| font_idx glyph_idx none_dir_set EOE
    {
      $entry = TA_control_new(Control_Point_Dir,
                              $font_idx,
                              $glyph_idx,
                              $none_dir_set,
                              TA_DIR_NONE,
                              0,
                              NULL);
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
        $glyph_idx = FT_Get_Name_Index(face, $glyph_name);
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
  'l'
    {
      $glyph_name = strdup("l");
      if ($glyph_name)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| 'n'
    {
      $glyph_name = strdup("n");
      if ($glyph_name)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| 'p'
    {
      $glyph_name = strdup("p");
      if ($glyph_name)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| 'r'
    {
      $glyph_name = strdup("r");
      if ($glyph_name)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| 'x'
    {
      $glyph_name = strdup("x");
      if ($glyph_name)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| 'y'
    {
      $glyph_name = strdup("y");
      if ($glyph_name)
      {
        store_error_data(&@$, context, TA_Err_Control_Allocation_Error);
        YYABORT;
      }
    }
| NAME
    {
      /* `$NAME' was allocated in the lexer */
      if (context->error)
      {
        /* lexing error */
        store_error_data(&@$, context, context->error);
        YYABORT;
      }

      $glyph_name = $NAME;
    }
;

point_set:
  'p'
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face, context->glyph_idx, FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;
    }
  number_set
    { $point_set = $number_set; }
;

left_dir_set:
  'l'
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face, context->glyph_idx, FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;
    }
  number_set
    { $left_dir_set = $number_set; }
;

right_dir_set:
  'r'
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face, context->glyph_idx, FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;
    }
  number_set
    { $right_dir_set = $number_set; }
;

none_dir_set:
  'n'
    {
      FT_Error error;
      FT_Face face = context->font->sfnts[context->font_idx].face;
      int num_points;


      error = FT_Load_Glyph(face, context->glyph_idx, FT_LOAD_NO_SCALE);
      if (error)
      {
        store_error_data(&@$, context, TA_Err_Control_Invalid_Glyph);
        YYABORT;
      }

      num_points = face->glyph->outline.n_points;

      context->number_set_min = 0;
      context->number_set_max = num_points - 1;
    }
  number_set
    { $none_dir_set = $number_set; }
;


x_shift:
  /* empty */
    { $x_shift = 0; }
| 'x' shift
    { $x_shift = $shift; }
;

y_shift:
  /* empty */
    { $y_shift = 0; }
| 'y' shift
    { $y_shift = $shift; }
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

real[result]:
  integer
    { $result = $integer; }
| REAL
    {
      if (context->error)
      {
        /* lexing error */
        store_error_data(&@$, context, context->error);
        YYABORT;
      }

      $result = $REAL;
    }
| '+' real[unsigned]
    { $result = $unsigned; }
| '-' real[unsigned]
    { $result = -$unsigned; }
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
                                      $integer,
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
      $left_limited = number_set_new($integer,
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
      $range_elem = number_set_new($integer,
                                   $integer,
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
      $range = number_set_new($left,
                              $right,
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
