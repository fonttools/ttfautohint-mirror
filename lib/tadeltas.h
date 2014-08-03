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
 * successful call to `ta_deltas_parse'.  Use `ta_deltas_free' to deallocate
 * it.
 */

typedef struct Deltas_
{
  long font_idx;
  long glyph_idx;
  number_range* points;
  double x_shift;
  double y_shift;
  number_range* ppems;
} Deltas;


/*
 * Parse a delta exceptions description in string `s', which has the
 * following syntax:
 *
 *   [<font idx>] <glyph name> p <points> [x <x shift>] [y <y shift>] @ <ppems>
 *
 * <font idx> gives the index of the font in a TrueType Collection.  If
 * missing, it is set to zero.  For normal TrueType fonts, only value zero
 * is valid.  If starting with `0x' the number is interpreted as
 * hexadecimal.  If starting with `0' it gets interpreted as an octal value,
 * and as a decimal value otherwise.
 *
 * <glyph name> is a glyph's name as listed in the `post' SFNT table.
 *
 * A glyph name consists of characters from the set `A-Za-z0-9._' only and
 * does not start with a digit or period, with the exceptions of the names
 * `.notdef' and `.null'.  Alternatively, it might be a number (in decimal,
 * octal, or hexadecimal format, the latter two indicated by the prefixes
 * `0' and `0x', respectively) to directly specify the glyph index.
 *
 * Both <points> and <ppems> are number ranges as described in
 * `numberset.h'.  Point values are limited by the glyph's number of points;
 * ppem values are limited by `ppem_min' and `ppem_max'; those parameters
 * are passed to `number_set_parse'.
 *
 * <x shift> and <y shift> represent floating point numbers.  The entries
 * for `x' and `y' are optional; if missing, the corresponding value is set
 * to zero.
 *
 * A comment starts with character `#'; the rest of the line is ignored.
 *
 * In case of success (this is, the delta exceptions description in `s' is
 * valid) the return value is a pointer to the final zero byte in string
 * `s'.  In case of an error, the return value is a pointer to the beginning
 * position of the offending character in string `s'.
 *
 * If s is NULL, the function exits immediately with NULL as the return
 * value.
 *
 * If the user provides a non-NULL `deltas' value, `ta_deltas_parse' stores
 * the parsed result in `*deltas', allocated with `malloc'.  If there is no
 * data (for example, an empty string or whitespace only) nothing gets
 * allocated, and `*deltas' is set to NULL.  In case of error, `*deltas'
 * returns an error code; you should use the following macros to compare
 * with.
 *
 * Values for <x shift>, <y shift>, and <ppems> must be in the ranges given
 * by `x_min' and `x_max', `y_min' and `y_max', and `ppem_min' and
 * `ppem_max', respectively.  Values for <points> are limited by the number
 * of points in the glyph.
 *
 * In case of error, `*deltas' returns an error code; you should use the
 * following macros definitions to compare with.
 *
 *   TA_DELTAS_SYNTAX_ERROR
 *     the function cannot parse the delta exceptions description string
 *   TA_DELTAS_INVALID_FONT_INDEX
 *     the specified font index in a TTC doesn't exist
 *   TA_DELTAS_INVALID_GLYPH_INDEX
 *     the glyph index is not present in the font or subfont
 *   TA_DELTAS_INVALID_GLYPH_NAME
 *     the glyph name is invalid or not present in the font or subfont
 *   TA_DELTAS_INVALID_CHARACTER
 *     invalid character in delta exceptions description string
 *   TA_DELTAS_INVALID_X_RANGE
 *     invalid range for the x shift, exceeding `x_min' or `x_max'
 *   TA_DELTAS_INVALID_Y_RANGE
 *     invalid range for the y shift, exceeding `y_min' or `y_max'
 *   TA_DELTAS_INVALID_POINT_RANGE
 *     invalid range for points, exceeding the number of glyph points
 *   TA_DELTAS_INVALID_PPEM_RANGE
 *     invalid range for ppems, exceeding `ppem_min' or `ppem_max'
 *   TA_DELTAS_OVERLAPPING_RANGES
 *     overlapping point or ppem ranges
 *   TA_DELTAS_NOT_ASCENDING
 *     not ascending point or ppem ranges or values
 *   TA_DELTAS_ALLOCATION_ERROR
 *     allocation error
 */

#define TA_DELTAS_SYNTAX_ERROR (Deltas*)-1
#define TA_DELTAS_INVALID_FONT_INDEX (Deltas*)-2
#define TA_DELTAS_INVALID_GLYPH_INDEX (Deltas*)-3
#define TA_DELTAS_INVALID_GLYPH_NAME (Deltas*)-4
#define TA_DELTAS_INVALID_CHARACTER (Deltas*)-5
#define TA_DELTAS_INVALID_X_RANGE (Deltas*)-6
#define TA_DELTAS_INVALID_Y_RANGE (Deltas*)-7
#define TA_DELTAS_INVALID_POINT_RANGE (Deltas*)-8
#define TA_DELTAS_INVALID_GLYPH (Deltas*)-9
#define TA_DELTAS_INVALID_PPEM_RANGE (Deltas*)-10
#define TA_DELTAS_OVERFLOW (Deltas*)-11
#define TA_DELTAS_OVERLAPPING_RANGES (Deltas*)-12
#define TA_DELTAS_NOT_ASCENDING (Deltas*)-13
#define TA_DELTAS_ALLOCATION_ERROR (Deltas*)-14

const char*
TA_deltas_parse(FONT* font,
                const char* s,
                Deltas** deltas,
                int x_min, int x_max,
                int y_min, int y_max,
                int ppem_min, int ppem_max);


/*
 * Free the allocated data in `deltas'.
 */

void
TA_deltas_free(Deltas* deltas);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DELTAS_H__ */

/* end of deltas.h */
