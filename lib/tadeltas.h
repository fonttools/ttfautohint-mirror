/* tadeltas.h */

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


#ifndef __DELTAS_H__
#define __DELTAS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <setjmp.h>  /* for flex error handling */


/* see the section `Managing exceptions' in chapter 6 */
/* (`The TrueType Instruction Set') of the OpenType reference */
/* how `delta_shift' works */

#define DELTA_SHIFT 3 /* 1/8px */
#define DELTA_FACTOR (1 << DELTA_SHIFT)

#define DELTA_SHIFT_MAX ((1.0 / DELTA_FACTOR) * 8)
#define DELTA_SHIFT_MIN -DELTA_SHIFT_MAX


/*
 * For the generated TrueType bytecode, we use
 *
 *   delta_base = 6   ,
 *
 * which gives us the following ppem ranges for the three delta
 * instructions:
 *
 *   DELTAP1    6-21ppem
 *   DELTAP2   22-37ppem
 *   DELTAP3   38-53ppem   .
 */

#define DELTA_PPEM_MIN 6
#define DELTA_PPEM_MAX 53


/*
 * A structure to hold delta exceptions for a glyph.  A linked list of it
 * gets allocated by a successful call to `TA_deltas_parse_buffer'.  Use
 * `TA_deltas_free' to deallocate the list.
 *
 * `x_shift' and `y_shift' are always in the range [-8;8].
 */

typedef struct Deltas_
{
  long font_idx;
  long glyph_idx;
  number_range* points;
  char x_shift;
  char y_shift;
  number_range* ppems;

  struct Deltas_* next;
} Deltas;


/*
 * A structure to hold a single delta exception.
 */

typedef struct Delta_
{
  long font_idx;
  long glyph_idx;
  int ppem;
  int point_idx;

  char x_shift;
  char y_shift;
} Delta;


/*
 * This structure is used for communication with `TA_deltas_parse'.
 */

typedef struct Deltas_Context_
{
  /* The error code returned by the parser or lexer. */
  TA_Error error;

  /* If no error, this holds the parsing result. */
  Deltas* result;

  /*
   * The parser's or lexer's error message in case of error; might be the
   * empty string.
   */
  char errmsg[256];

  /*
   * In case of error, `errline_num' gives the line number of the offending
   * line in `font->delta_buf', starting with value 1; `errline_pos_left'
   * and `errline_pos_right' hold the left and right position of the
   * offending token in this line, also starting with value 1.  For
   * allocation errors or internal parser or lexer errors those values are
   * meaningless, though.
   */
  int errline_num;
  int errline_pos_left;
  int errline_pos_right;

  /*
   * The current font index, useful for `TA_Err_Deltas_Invalid_Font_Index'.
   */
  long font_idx;

  /*
   * The current glyph index, useful for
   * `TA_Err_Deltas_Invalid_Glyph_Index'.
   */
  long glyph_idx;

  /*
   * If the returned error is `TA_Err_Deltas_Invalid_Range', these two
   * values set up the allowed range.
   */
  long number_set_min;
  long number_set_max;

  /* private flex data */
  void* scanner;
  int eof;
  jmp_buf jump_buffer;

  /* private bison data */
  FONT* font;
} Deltas_Context;


/*
 * Create and initialize a `Deltas' object.  In case of an allocation error,
 * the return value is NULL.  `point_set' and `ppem_set' are expected to be
 * in reverse list order; `TA_deltas_new' then reverts them to normal order.
 */

Deltas*
TA_deltas_new(long font_idx,
              long glyph_idx,
              number_range* point_set,
              double x_shift,
              double y_shift,
              number_range* ppem_set);


/*
 * Prepend `element' to `list' of `Deltas' objects.  If `element' is NULL,
 * return `list.
 */

Deltas*
TA_deltas_prepend(Deltas* list,
                  Deltas* element);


/*
 * Reverse a list of `Deltas' objects.
 */

Deltas*
TA_deltas_reverse(Deltas* list);


/*
 * Initialize the scanner data within a `Deltas_Context' object.
 * `font->delta_buf' is the delta exceptions buffer to be parsed,
 * `font->delta_len' its length.
 *
 * This function is defined in `tadeltas.l'.
 */

void
TA_deltas_scanner_init(Deltas_Context* context,
                       FONT* font);


/*
 * Free the scanner data within a `Deltas_Context' object.
 *
 * This function is defined in `tadeltas.l'.
 */

void
TA_deltas_scanner_done(Deltas_Context* context);


/*
 * Parse buffer with descriptions of delta exceptions.
 *
 * The format of lines in such a delta exceptions buffer is given in
 * `ttfautohint.h' (option `deltas-file'); the following describes more
 * technical details, using the constants defined above.
 *
 * x shift and y shift values represent floating point numbers that get
 * rounded to multiples of 1/(2^DELTA_SHIFT) pixels.
 *
 * Values for x and y shifts must be in the range
 * [DELTA_SHIFT_MIN;DELTA_SHIFT_MAX].  Values for ppems must be in the range
 * [DELTA_PPEM_MIN;DELTA_PPEM_MAX].
 *
 * The returned error codes are 0 (TA_Err_Ok) or in the range 0x200-0x2FF;
 * see `ttfautohint-errors.h' for all possible values.
 *
 * If the user provides a non-NULL `deltas' value, `TA_deltas_parse_buffer'
 * stores the parsed result in `*deltas', to be freed with `TA_deltas_free'
 * after use.  If there is no delta exceptions data (for example, an empty
 * string or whitespace only) nothing gets allocated, and `*deltas' is set
 * to NULL.
 *
 * In case of error, `error_string_p' holds an error message, `errlinenum_p'
 * gives the line number in the delta exceptions buffer where the error
 * occurred, `errline_p' the corresponding line, and `errpos_p' the position
 * in this line.  After use, `error_string_p' and `errline_p' must be
 * deallocated by the user.  Note that `errline_p' and `errpos_p' can be
 * NULL even in case of an error.  If there is no error, those four values
 * are undefined.
 */

TA_Error
TA_deltas_parse_buffer(FONT* font,
                       Deltas** deltas,
                       char** error_string_p,
                       unsigned int* errlinenum_p,
                       char** errline_p,
                       char** errpos_p);


/*
 * Free the allocated data in `deltas'.
 */

void
TA_deltas_free(Deltas* deltas);


/*
 * Return a string representation of `deltas'.  After use, the string should
 * be deallocated with a call to `free'.  In case of an allocation error,
 * the return value is NULL.
 */

char*
TA_deltas_show(FONT* font,
               Deltas* deltas);


/*
 * Build a tree providing sequential access to the delta exceptions data.
 * This also sets `deltas_data_cur' to the first element (or NULL if there
 * isn't one).
 */

TA_Error
TA_deltas_build_tree(FONT* font,
                     Deltas* deltas);


/*
 * Free the delta exceptions data tree.
 */

void
TA_deltas_free_tree(FONT* font);


/*
 * Get next delta exception and store it in `font->deltas_data_cur'.
 */

void
TA_deltas_get_next(FONT* font);


/*
 * Access delta exception.  Return NULL if there is no more data.
 */
const Delta*
TA_deltas_get_delta(FONT* font);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DELTAS_H__ */

/* end of deltas.h */
