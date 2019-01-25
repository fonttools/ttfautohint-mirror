/* tacontrol.h */

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


#ifndef TACONTROL_H_
#define TACONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <setjmp.h>  /* for flex error handling */


/* see the section `Managing exceptions' in chapter 6 */
/* (`The TrueType Instruction Set') of the OpenType reference */
/* how `delta_shift' works */

#define CONTROL_DELTA_SHIFT 3 /* 1/8px */
#define CONTROL_DELTA_FACTOR (1 << CONTROL_DELTA_SHIFT)

#define CONTROL_DELTA_SHIFT_MAX ((1.0 / CONTROL_DELTA_FACTOR) * 8)
#define CONTROL_DELTA_SHIFT_MIN -CONTROL_DELTA_SHIFT_MAX


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

#define CONTROL_DELTA_PPEM_MIN 6
#define CONTROL_DELTA_PPEM_MAX 53


/*
 * The control type.
 */

typedef enum Control_Type_
{
  Control_Delta_before_IUP,
  Control_Delta_after_IUP,
  Control_Single_Point_Segment_Left,
  Control_Single_Point_Segment_Right,
  Control_Single_Point_Segment_None,
  Control_Script_Feature_Glyphs,
  Control_Script_Feature_Widths
} Control_Type;


/*
 * A structure to hold control instructions.  A linked list of it gets
 * allocated by a successful call to `TA_control_parse_buffer'.  Use
 * `TA_control_free' to deallocate the list.
 *
 * `x_shift' and `y_shift' are in the range [-8;8] for delta exceptions
 * and in the range [SHRT_MIN;SHRT_MAX] for one-point segment offsets.
 *
 * The `Control' typedef is in `ta.h'.
 */

struct Control_
{
  Control_Type type;

  long font_idx;
  long glyph_idx;
  number_range* points;
  int x_shift;
  int y_shift;
  number_range* ppems;

  int line_number;

  struct Control_* next;
};


/*
 * A structure to hold a single control instruction.
 */

typedef struct Ctrl_
{
  Control_Type type;

  long font_idx;
  long glyph_idx;
  int ppem;
  int point_idx;

  int x_shift;
  int y_shift;

  int line_number;
} Ctrl;


/*
 * This structure is used for communication with `TA_control_parse'.
 */

typedef struct Control_Context_
{
  /* The error code returned by the parser or lexer. */
  TA_Error error;

  /* If no error, this holds the parsing result. */
  Control* result;

  /*
   * The parser's or lexer's error message in case of error; might be the
   * empty string.
   */
  char errmsg[256];

  /*
   * In case of error, `errline_num' gives the line number of the offending
   * line in `font->control_buf', starting with value 1; `errline_pos_left'
   * and `errline_pos_right' hold the left and right position of the
   * offending token in this line, also starting with value 1.  For
   * allocation errors or internal parser or lexer errors those values are
   * meaningless, though.
   */
  int errline_num;
  int errline_pos_left;
  int errline_pos_right;

  /*
   * The current font index, useful for `TA_Err_Control_Invalid_Font_Index'.
   */
  long font_idx;

  /*
   * The current glyph index, useful for
   * `TA_Err_Control_Invalid_Glyph_Index'.
   */
  long glyph_idx;

  /*
   * If the returned error is `TA_Err_Control_Invalid_Range', these two
   * values set up the allowed range.
   */
  int number_set_min;
  int number_set_max;

  /*
   * Some sets restrict the number of elements.
   */
  int number_set_num_elems;

  /* private flex data */
  void* scanner;
  int eof;
  jmp_buf jump_buffer;

  /* private bison data */
  FONT* font;
} Control_Context;


/*
 * Create and initialize a `Control' object.  In case of an allocation error,
 * the return value is NULL.  `point_set' and `ppem_set' are expected to be
 * in reverse list order; `TA_control_new' then reverts them to normal order.
 */

Control*
TA_control_new(Control_Type type,
               long font_idx,
               long glyph_idx,
               number_range* point_set,
               double x_shift,
               double y_shift,
               number_range* ppem_set,
               int line_number);


/*
 * Prepend `element' to `list' of `Control' objects.  If `element' is NULL,
 * return `list.
 */

Control*
TA_control_prepend(Control* list,
                   Control* element);


/*
 * Reverse a list of `Control' objects.
 */

Control*
TA_control_reverse(Control* list);


/*
 * Initialize the scanner data within a `Control_Context' object.
 * `font->control_buf' is the control instructions buffer to be parsed,
 * `font->control_len' its length.
 *
 * This function is defined in `tacontrol.flex'.
 */

void
TA_control_scanner_init(Control_Context* context,
                        FONT* font);


/*
 * Free the scanner data within a `Control_Context' object.
 *
 * This function is defined in `tacontrol.flex'.
 */

void
TA_control_scanner_done(Control_Context* context);


/*
 * Parse buffer with ttfautohint control instructions, stored in
 * `font->control_buf' with length `font->control_len'.
 *
 * The format of entries in such a control instructions buffer is given in
 * `ttfautohint.h' (option `control-file'); the following describes more
 * technical details, using the constants defined above.
 *
 * x shift and y shift values represent floating point numbers that get
 * rounded to multiples of 1/(2^CONTROL_DELTA_SHIFT) pixels.
 *
 * Values for x and y shifts must be in the range
 * [CONTROL_DELTA_SHIFT_MIN;CONTROL_DELTA_SHIFT_MAX].  Values for ppems must
 * be in the range [CONTROL_DELTA_PPEM_MIN;CONTROL_DELTA_PPEM_MAX].
 *
 * The returned error codes are 0 (TA_Err_Ok) or in the range 0x200-0x2FF;
 * see `ttfautohint-errors.h' for all possible values.
 *
 * `TA_control_parse_buffer' stores the parsed result in `font->control', to
 * be freed with `TA_control_free' after use.  If there is no control
 * instructions data (for example, an empty string or whitespace only)
 * nothing gets allocated, and `font->control' is set to NULL.
 *
 * In case of error, `error_string_p' holds an error message, `errlinenum_p'
 * gives the line number in the control instructions buffer where the error
 * occurred, `errline_p' the corresponding line, and `errpos_p' the position
 * in this line.  After use, `error_string_p' and `errline_p' must be
 * deallocated by the user.  Note that `errline_p' and `errpos_p' can be
 * NULL even in case of an error.  If there is no error, those four values
 * are undefined.
 */

TA_Error
TA_control_parse_buffer(FONT* font,
                        char** error_string_p,
                        unsigned int* errlinenum_p,
                        char** errline_p,
                        char** errpos_p);


/*
 * Apply coverage data from the control instructions file.
 */

void
TA_control_apply_coverage(SFNT* sfnt,
                          FONT* font);


/*
 * Handle stem width data from the control instructions file.
 */

void
TA_control_set_stem_widths(TA_LatinMetrics metrics,
                           FONT* font);


/*
 * Free the allocated data in `control'.
 */

void
TA_control_free(Control* control);


/*
 * Return a string representation of `font->control'.  After use, the string
 * should be deallocated with a call to `free'.  In case of an allocation
 * error, the return value is NULL.
 */

char*
TA_control_show(FONT* font);


/*
 * Build a tree providing sequential access to the control instructions data
 * in `font->control'.  This also sets `font->control_data_cur' to the first
 * element (or NULL if there isn't one).
 */

TA_Error
TA_control_build_tree(FONT* font);


/*
 * Free the control instructions data tree.
 */

void
TA_control_free_tree(FONT* font);


/*
 * Get next control instruction and store it in `font->control_data_cur'.
 */

void
TA_control_get_next(FONT* font);


/*
 * Access control instruction.  Return NULL if there is no more data.
 */

const Ctrl*
TA_control_get_ctrl(FONT* font);


/*
 * Collect one-point segment data for a given glyph index and store them in
 * `font->control_segment_dirs'.
 */

TA_Error
TA_control_segment_dir_collect(FONT* font,
                               long font_idx,
                               long glyph_idx);


/*
 * Access next one-point segment data.  Returns 1 on success or 0 if no more
 * data.  In the latter case, it resets the internal iterator so that
 * calling this function another time starts at the beginning again.
 */

int
TA_control_segment_dir_get_next(FONT* font,
                                int* point_idx,
                                TA_Direction* dir,
                                int* left_offset,
                                int* right_offset);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TACONTROL_H_ */

/* end of control.h */
