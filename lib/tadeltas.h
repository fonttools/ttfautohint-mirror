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


/*
 * A structure to hold delta exceptions for a glyph.  It gets allocated by a
 * successful call to `TA_deltas_parse'.  Use `TA_deltas_free' to deallocate
 * it.
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
} Deltas;


/*
 * Parse a delta exceptions description in string `s', which has the
 * following syntax:
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
 * Values for <x shift>, <y shift>, and <ppems> must be in the ranges given
 * by `x_min' and `x_max', `y_min' and `y_max', and `ppem_min' and
 * `ppem_max', respectively.  Values for <points> are limited by the number
 * of points in the glyph.
 *
 * A comment starts with character `#'; the rest of the line is ignored.  An
 * empty line is ignored also.
 *
 * In case of success (this is, the delta exceptions description in `s' is
 * valid), `pos' is a pointer to the final zero byte in string `s'.  In case
 * of an error, it points to the offending character in string `s'.
 *
 * If s is NULL, the function exits immediately, with NULL as the value of
 * `pos'.
 *
 * If the user provides a non-NULL `deltas' value, `TA_deltas_parse' stores
 * the parsed result in `*deltas', allocated with `malloc'.  If there is no
 * data (for example, an empty string or whitespace only) nothing gets
 * allocated, and `*deltas' is set to NULL.
 *
 * The returned error codes are in the range 0x200-0x2FF; see
 * `ttfautohint-errors.h' for all possible values.
 */

TA_Error
TA_deltas_parse(FONT* font,
                const char* s,
                const char** err_pos,
                Deltas** deltas,
                double x_min, double x_max,
                double y_min, double y_max,
                int ppem_min, int ppem_max);


/*
 * Free the allocated data in `deltas'.
 */

void
TA_deltas_free(Deltas* deltas);


/*
 * Return a string representation of `deltas'.  After use, the string should
 * be deallocated with a call to `free'.  In case of an allocationn error,
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
