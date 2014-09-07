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
 * This structure is used for both the parser and lexer,
 * holding local variables and the parsing result.
 */

typedef struct Deltas_Context_
{
  TA_Error error;

  int errline_num;
  int errline_pos_left;
  int errline_pos_right;

  char errmsg[256];

  Deltas* result;

  /* flex data */
  void* scanner;
  int eof;
  jmp_buf jump_buffer;

  /* bison data */
  FONT* font;

  long font_idx;
  long glyph_idx;

  long number_set_min;
  long number_set_max;
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
 * Initialize the scanner data within a `Deltas_Context' object.  `input' is
 * the delta exceptions string to be parsed.
 *
 * This function is defined in `tadeltas.l'.
 */

void
TA_deltas_scanner_init(Deltas_Context* context,
                       const char* input);


/*
 * Free the scanner data within a `Deltas_Context' object.
 *
 * This function is defined in `tadeltas.l'.
 */

void
TA_deltas_scanner_done(Deltas_Context* context);


/*
 * Parse a delta exceptions file.
 *
 * The format of lines in a delta exceptions file is given in
 * `ttfautohint.h' (option `deltas-file'); the following gives more
 * technical details, using the constants defined above.
 *
 * x shift and y shift values represent floating point numbers that get
 * rounded to multiples of 1/(2^DELTA_SHIFT) pixels.
 *
 * Values for x and y shifts must be in the range
 * [DELTA_SHIFT_MIN;DELTA_SHIFT_MAX].  Values for ppems must be in the range
 * [DELTA_PPEM_MIN;DELTA_PPEM_MAX].
 *
 * The returned error codes are in the range 0x200-0x2FF; see
 * `ttfautohint-errors.h' for all possible values.
 *
 * If the user provides a non-NULL `deltas' value, `TA_deltas_parse_buffer'
 * stores the parsed result in `*deltas'.  If there is no data (for example,
 * an empty string or whitespace only) nothing gets allocated, and `*deltas'
 * is set to NULL.
 *
 * In case of error, `errlinenum_p' gives the line number in the delta
 * exceptions file where the error occurred, `errline_p' the corresponding
 * line, and `errpos_p' the position in this line.  If there is no error,
 * those three values are undefined.  Both `errline_p' and `errpos_p' can be
 * NULL even in case of an error; otherwise `errline_p' must be deallocated
 * by the user.
 */

TA_Error
TA_deltas_parse_buffer(FONT* font,
                       Deltas** deltas,
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
