/* tadeltas.h */

/*
 * Copyright (C) 2012-2014 by Werner Lemberg.
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
 * gets allocated by a successful call to `TA_deltas_parse'.  Use
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
 * Parse a delta exceptions file.
 *
 * A line in a delta exceptions file has the following syntax:
 *
 *   [<font idx>] <glyph id> p <points> [x <x shift>] [y <y shift>] @ <ppems>
 *
 * <font idx> gives the index of the font in a TrueType Collection.  If
 * missing, it is set to zero.  For normal TrueType fonts, only value zero
 * is valid.  If starting with `0x' the number is interpreted as
 * hexadecimal.  If starting with `0' it gets interpreted as an octal value,
 * and as a decimal value otherwise.
 *
 * <glyph id> is a glyph's name as listed in the `post' SFNT table or a
 * glyph index.  A glyph name consists of characters from the set
 * `A-Za-z0-9._' only and does not start with a digit or period, with the
 * exceptions of the names `.notdef' and `.null'.  A glyph index can be
 * specified in decimal, octal, or hexadecimal format, the latter two
 * indicated by the prefixes `0' and `0x', respectively.
 *
 * Both <points> and <ppems> are number ranges as described in
 * `numberset.h'; they are parsed using `number_set_parse'.
 *
 * <x shift> and <y shift> represent floating point numbers that get rounded
 * to multiples of 1/8 pixels.  The entries for `x' and `y' are optional; if
 * missing, the corresponding value is set to zero.
 *
 * Values for <x shift>, <y shift> must be in the range
 * [DELTA_SHIFT_MIN;DELTA_SHIFT_MAX].  Values for <ppems> must be in the
 * range [DELTA_PPEM_MIN;DELTA_PPEM_MAX].  Values for <points> are limited
 * by the number of points in the glyph.
 *
 * A comment starts with character `#'; the rest of the line is ignored.  An
 * empty line is ignored also.
 *
 * The returned error codes are in the range 0x200-0x2FF; see
 * `ttfautohint-errors.h' for all possible values.
 *
 * If the user provides a non-NULL `deltas' value, `TA_deltas_parse' stores
 * the parsed result in `*deltas'.  If there is no data (for example, an
 * empty string or whitespace only) nothing gets allocated, and `*deltas' is
 * set to NULL.
 *
 * In case of error, `errlinenum_p' gives the line number in the delta
 * exceptions file where the error occurred, `errline_p' the corresponding
 * line, and `errpos_p' the position in this line.  If there is no error,
 * those three values are undefined.  Both `errline_p' and `errpos_p' can be
 * NULL even in case of an error; otherwise `errline_p' must be deallocated
 * by the user.
 */

TA_Error
TA_deltas_parse(FONT* font,
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DELTAS_H__ */

/* end of deltas.h */
