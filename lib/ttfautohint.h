/* ttfautohint.h */

/*
 * Copyright (C) 2011-2014 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


#ifndef __TTFAUTOHINT_H__
#define __TTFAUTOHINT_H__

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * This file gets processed with a simple sed script to extract the
 * documentation (written in pandoc's markdown format); code between the
 * `pandoc' markers are retained, everything else is discarded.  C comments
 * are uncommented so that column 4 becomes column 1; empty lines outside of
 * comments are removed.
 */


/* pandoc-start */

/*
 * The ttfautohint API
 * ===================
 *
 * This section documents the single function of the ttfautohint library,
 * `TTF_autohint`, together with its callback functions, `TA_Progress_Func`
 * and `TA_Info_Func`.  All information has been directly extracted from the
 * `ttfautohint.h` header file.
 *
 */


/*
 * Preprocessor Macros and Typedefs
 * --------------------------------
 *
 * Some default values.
 *
 * ```C
 */

#define TA_HINTING_RANGE_MIN 8
#define TA_HINTING_RANGE_MAX 50
#define TA_HINTING_LIMIT 200
#define TA_INCREASE_X_HEIGHT 14

/*
 *```
 *
 * An error type.
 *
 * ```C
 */

typedef int TA_Error;

/*
 * ```
 *
 */


/*
 * Callback: `TA_Progress_Func`
 * ----------------------------
 *
 * A callback function to get progress information.  *curr_idx* gives the
 * currently processed glyph index; if it is negative, an error has
 * occurred.  *num_glyphs* holds the total number of glyphs in the font
 * (this value can't be larger than 65535).
 *
 * *curr_sfnt* gives the current subfont within a TrueType Collection (TTC),
 * and *num_sfnts* the total number of subfonts.
 *
 * If the return value is non-zero, `TTF_autohint` aborts with
 * `TA_Err_Canceled`.  Use this for a 'Cancel' button or similar features in
 * interactive use.
 *
 * *progress_data* is a void pointer to user-supplied data.
 *
 * ```C
 */

typedef int
(*TA_Progress_Func)(long curr_idx,
                    long num_glyphs,
                    long curr_sfnt,
                    long num_sfnts,
                    void* progress_data);

/*
 * ```
 *
 */


/*
 * Callback: `TA_Error_Func`
 * -------------------------
 *
 * A callback function to get error information.
 *
 * *error* is the value `TTF_autohint` returns.  See file
 * `ttfautohint-errors.h` for a list.  Error codes not in this list are
 * directly taken from FreeType; see the FreeType header file `fterrdef.h`
 * for more.
 *
 * *error_string*, if non-NULL, is a pointer to an error message that
 * represents *error*.
 *
 * The next three parameters help identify the origin of text string parsing
 * errors.  *linenum*, if non-zero, contains the line number.  *line*, if
 * non-NULL, is a pointer to the input line that can't be processed.
 * *errpos*, if non-NULL, holds a pointer to the position in *line* where
 * the problem occurs.
 *
 * *error_data* is a void pointer to user-supplied data.
 *
 * ```C
 */

typedef void
(*TA_Error_Func)(TA_Error error,
                 const char* error_string,
                 unsigned int linenum,
                 const char* line,
                 const char* errpos,
                 void* error_data);

/*
 * ```
 *
 */


/*
 * Callback: `TA_Info_Func`
 * ------------------------
 *
 * A callback function to manipulate strings in the `name` table.
 * *platform_id*, *encoding_id*, *language_id*, and *name_id* are the
 * identifiers of a `name` table entry pointed to by *str* with a length
 * pointed to by *str_len* (in bytes; the string has no trailing NULL byte).
 * Please refer to the [OpenType specification of the `name` table] for a
 * detailed description of the various parameters, in particular which
 * encoding is used for a given platform and encoding ID.
 *
 * [OpenType specification of the `name` table]: http://www.microsoft.com/typography/otspec/name.htm
 *
 * The string *str* is allocated with `malloc`; the application should
 * reallocate the data if necessary, ensuring that the string length doesn't
 * exceed 0xFFFF.
 *
 * *info_data* is a void pointer to user-supplied data.
 *
 * If an error occurs, return a non-zero value and don't modify *str* and
 * *str_len* (such errors are handled as non-fatal).
 *
 * ```C
 */

typedef int
(*TA_Info_Func)(unsigned short platform_id,
                unsigned short encoding_id,
                unsigned short language_id,
                unsigned short name_id,
                unsigned short* str_len,
                unsigned char** str,
                void* info_data);

/*
 * ```
 *
 */

/* pandoc-end */


/*
 * Error values in addition to the FT_Err_XXX constants from FreeType.
 *
 * All error values specific to ttfautohint start with `TA_Err_'.
 */
#include <ttfautohint-errors.h>


/* pandoc-start */

/*
 * Function: `TTF_autohint`
 * ------------------------
 *
 *
 * Read a TrueType font, remove existing bytecode (in the SFNT tables
 * `prep`, `fpgm`, `cvt `, and `glyf`), and write a new TrueType font with
 * new bytecode based on the autohinting of the FreeType library.
 *
 * It expects a format string *options* and a variable number of arguments,
 * depending on the fields in *options*.  The fields are comma separated;
 * whitespace within the format string is not significant, a trailing comma
 * is ignored.  Fields are parsed from left to right; if a field occurs
 * multiple times, the last field's argument wins.  The same is true for
 * fields that are mutually exclusive.  Depending on the field, zero or one
 * argument is expected.
 *
 * Note that fields marked as 'not implemented yet' are subject to change.
 *
 *
 * ### I/O
 *
 * `in-file`
 * :   A pointer of type `FILE*` to the data stream of the input font,
 *     opened for binary reading.  Mutually exclusive with `in-buffer`.
 *
 * `in-buffer`
 * :   A pointer of type `const char*` to a buffer that contains the input
 *     font.  Needs `in-buffer-len`.  Mutually exclusive with `in-file`.
 *
 * `in-buffer-len`
 * :   A value of type `size_t`, giving the length of the input buffer.
 *     Needs `in-buffer`.
 *
 * `out-file`
 * :   A pointer of type `FILE*` to the data stream of the output font,
 *     opened for binary writing.  Mutually exclusive with `out-buffer`.
 *
 * `out-buffer`
 * :   A pointer of type `char**` to a buffer that contains the output
 *     font.  Needs `out-buffer-len`.  Mutually exclusive with `out-file`.
 *     Deallocate the memory with `free`.
 *
 * `out-buffer-len`
 * :   A pointer of type `size_t*` to a value giving the length of the
 *     output buffer.  Needs `out-buffer`.
 *
 * `control-file`
 * :   A pointer of type `FILE*` to the data stream of control instructions.
 *     Mutually exclusive with `control-buffer`.
 *
 *     An entry in a control instructions file or buffer has one of the
 *     following syntax forms:
 *
 *     > *\[* font-idx *\]*\ \ glyph-id\ \ *`l`|`r`|`n`* points\
 *     > *\[* font-idx *\]*\ \ glyph-id\ \ *`p`* points\ \ *\[* *`x`* x-shift *\]*\ \ *\[* *`y`* y-shift *\]*\ \ *`@`* ppems
 *
 *     *font-idx* gives the index of the font in a TrueType Collection.  If
 *     missing, it is set to zero.  For normal TrueType fonts, only value
 *     zero is valid.  If starting with `0x` the number is interpreted as
 *     hexadecimal.  If starting with `0` it gets interpreted as an octal
 *     value, and as a decimal value otherwise.
 *
 *     *glyph-id* is a glyph's name as listed in the `post` SFNT table or a
 *     glyph index.  A glyph name consists of characters from the set
 *     '`A-Za-z0-9._`' only and does not start with a digit or period, with
 *     the exceptions of the names '`.notdef`' and '`.null`'.  A glyph index
 *     can be specified in decimal, octal, or hexadecimal format, the latter
 *     two indicated by the prefixes `0` and `0x`, respectively.
 *
 *     The mutually exclusive parameters '`l`', '`r`', or\ '`n`' indicate
 *     that the following points have left, right, or no direction,
 *     respectively, overriding ttfautohint's algorithm for setting point
 *     directions.  The 'direction of a point' is the direction of the
 *     outline controlled by this point.  By changing a point's direction
 *     from 'no direction' to either left or right, you can create a
 *     one-point segment with the given direction so that ttfautohint
 *     handles the point similar to other segments.  Setting a point's
 *     direction to 'no direction', ttfautohint no longer considers it as
 *     part of a segment, thus treating it as a 'weak' point.  Changed point
 *     directions don't directly modify the outlines; they only influence
 *     the hinting process.
 *
 *     Parameter '`p`' makes ttfautohint apply delta exceptions for the
 *     given points, shifting the points by the given values.  Note that
 *     those delta exceptions are applied *after* the final `IP`
 *     instructions in the bytecode; as a consequence, they are (partially)
 *     ignored by rasterizers if in ClearType mode.
 *
 *     Both *points* and *ppems* are number ranges, similar to the
 *     `x-height-snapping-exceptions` syntax.
 *
 *     *x-shift* and *y-shift* represent real numbers that get rounded to
 *     multiples of 1/8 pixels.  The entries for '`x`' and '`y`' are
 *     optional; if missing, the corresponding value is set to zero.
 *
 *     Values for *x-shift* and *y-shift* must be in the range [-1.0;1.0].
 *     Values for *ppems* must be in the range [6;53].  Values for *points*
 *     are limited by the number of points in the glyph.
 *
 *     Control instruction entries can be either separated with newlines or
 *     with character '`;`'.  Additionally, a line can be continued on the
 *     next line by ending it with backslash character ('`\`').  A backslash
 *     followed by a newline gets treated similar to a whitespace character.
 *
 *     A comment starts with character '`#`'; the rest of the line is
 *     ignored.  An empty line is ignored also.
 *
 *     Note that only character '`.`' is recognized as a decimal point, and
 *     a thousands separator is not accepted.
 *
 * `control-buffer`
 * :   A pointer of type `const char*` to a buffer that contains control
 *     instructions.  Needs `control-buffer-len`.  Mutually exclusive with
 *     `control-file`.
 *
 * `control-buffer-len`
 * :   A value of type `size_t`, giving the length of the control
 *     instructions buffer.  Needs `control-buffer`.
 *
 *
 * ### Messages and Callbacks
 *
 * `progress-callback`
 * :   A pointer of type [`TA_Progress_Func`](#callback-ta_progress_func),
 *     specifying a callback function for progress reports.  This function
 *     gets called after a single glyph has been processed.  If this field
 *     is not set or set to NULL, no progress callback function is used.
 *
 * `progress-callback-data`
 * :   A pointer of type `void*` to user data that is passed to the
 *     progress callback function.
 *
 * `error-string`
 * :   A pointer of type `unsigned char**` to a string (in UTF-8 encoding)
 *     that verbally describes the error code.  You must not change the
 *     returned value.
 *
 * `error-callback`
 * :   A pointer of type [`TA_Error_Func`](#callback-ta_error_func),
 *     specifying a callback function for error messages.  This function
 *     gets called right before `TTF_autohint` exits.  If this field is not
 *     set or set to NULL, no error callback function is used.
 *
 *     Use it as a more sophisticated alternative to `error-string`.
 *
 * `error-callback-data`
 * :   A point of type `void*` to user data that is passed to the error
 *     callback function.
 *
 * `info-callback`
 * :   A pointer of type [`TA_Info_Func`](#callback-ta_info_func),
 *     specifying a callback function for manipulating the `name` table.
 *     This function gets called for each `name` table entry.  If not set or
 *     set to NULL, the table data stays unmodified.
 *
 * `info-callback-data`
 * :   A pointer of type `void*` to user data that is passed to the info
 *     callback function.
 *
 * `debug`
 * :   If this integer is set to\ 1, lots of debugging information is print
 *     to stderr.  The default value is\ 0.
 *
 *
 * ### General Hinting Options
 *
 * `hinting-range-min`
 * :   An integer (which must be larger than or equal to\ 2) giving the
 *     lowest PPEM value used for autohinting.  If this field is not set, it
 *     defaults to `TA_HINTING_RANGE_MIN`.
 *
 * `hinting-range-max`
 * :   An integer (which must be larger than or equal to the value of
 *     `hinting-range-min`) giving the highest PPEM value used for
 *     autohinting.  If this field is not set, it defaults to
 *     `TA_HINTING_RANGE_MAX`.
 *
 * `hinting-limit`
 * :   An integer (which must be larger than or equal to the value of
 *     `hinting-range-max`) that gives the largest PPEM value at which
 *     hinting is applied.  For larger values, hinting is switched off.  If
 *     this field is not set, it defaults to `TA_HINTING_LIMIT`.  If it is
 *     set to\ 0, no hinting limit is added to the bytecode.
 *
 * `hint-composites`
 * :   If this integer is set to\ 1, composite glyphs get separate hints.
 *     This implies adding a special glyph to the font called
 *     ['.ttfautohint'](#the-.ttfautohint-glyph).  Setting it to\ 0 (which
 *     is the default), the hints of the composite glyphs' components are
 *     used.  Adding hints for composite glyphs increases the size of the
 *     resulting bytecode a lot, but it might deliver better hinting
 *     results.  However, this depends on the processed font and must be
 *     checked by inspection.
 *
 * `adjust-subglyphs`
 * :   An integer (1\ for 'on' and 0\ for 'off', which is the default) to
 *     specify whether native TrueType hinting of the *input font* shall be
 *     applied to all glyphs before passing them to the (internal)
 *     autohinter.  The used resolution is the em-size in font units; for
 *     most fonts this is 2048ppem.  Use this only if the old hints move or
 *     scale subglyphs independently of the output resolution, for example
 *     some exotic CJK fonts.
 *
 *     `pre-hinting` is a deprecated alias name for this option.
 *
 *
 * ### Hinting Algorithms
 *
 * `gray-strong-stem-width`
 * :   An integer (1\ for 'on' and 0\ for 'off', which is the default) that
 *     specifies whether horizontal stems should be snapped and positioned
 *     to integer pixel values for normal grayscale rendering.
 *
 * `gdi-cleartype-strong-stem-width`
 * :   An integer (1\ for 'on', which is the default, and 0\ for 'off') that
 *     specifies whether horizontal stems should be snapped and positioned
 *     to integer pixel values for GDI ClearType rendering, this is, the
 *     rasterizer version (as returned by the GETINFO bytecode instruction)
 *     is in the range 36\ <= version <\ 38 and ClearType is enabled.
 *
 * `dw-cleartype-strong-stem-width`
 * :   An integer (1\ for 'on' and 0\ for 'off', which is the default) that
 *     specifies whether horizontal stems should be snapped and positioned
 *     to integer pixel values for DW ClearType rendering, this is, the
 *     rasterizer version (as returned by the GETINFO bytecode instruction)
 *     is >=\ 38, ClearType is enabled, and subpixel positioning is enabled
 *     also.
 *
 * `increase-x-height`
 * :   An integer.  For PPEM values in the range 6\ <= PPEM
 *     <= `increase-x-height`, round up the font's x\ height much more often
 *     than normally (to use the terminology of TrueType's 'Super Round'
 *     bytecode instruction, the threshold gets increased from 5/8px to
 *     13/16px).  If it is set to\ 0, this feature is switched off.  If this
 *     field is not set, it defaults to `TA_INCREASE_X_HEIGHT`.  Use this
 *     flag to improve the legibility of small font sizes if necessary.
 *
 * `x-height-snapping-exceptions`
 * :   A pointer of type `const char*` to a null-terminated string that
 *     gives a list of comma separated PPEM values or value ranges at which
 *     no x\ height snapping shall be applied.  A value range has the form
 *     *value*~1~`-`*value*~2~, meaning *value*~1~ <= PPEM <= *value*~2~.
 *     *value*~1~ or *value*~2~ (or both) can be missing; a missing value is
 *     replaced by the beginning or end of the whole interval of valid PPEM
 *     values, respectively.  Whitespace is not significant; superfluous
 *     commas are ignored, and ranges must be specified in increasing order.
 *     For example, the string `"3, 5-7, 9-"` means the values 3, 5, 6, 7,
 *     9, 10, 11, 12, etc.  Consequently, if the supplied argument is `"-"`,
 *     no x\ height snapping takes place at all.  The default is the empty
 *     string (`""`), meaning no snapping exceptions.
 *
 * `windows-compatibility`
 * :   If this integer is set to\ 1, two artificial blue zones are used,
 *     positioned at the `usWinAscent` and `usWinDescent` values (from the
 *     font's `OS/2` table).  The idea is to help ttfautohint so that the
 *     hinted glyphs stay within this horizontal stripe since Windows clips
 *     everything falling outside.  The default is\ 0.
 *
 *
 * ### Scripts
 *
 * `default-script`
 * :   A string consisting of four lowercase characters that specifies the
 *     default script for OpenType features.  After applying all features
 *     that are handled specially, use this value for the remaining
 *     features.  The default value is `"latn"`; if set to `"none"`, no
 *     script is used.  Valid values can be found in the header file
 *     `ttfautohint-scripts.h`.
 *
 * `fallback-script`
 * :   A string consisting of four lowercase characters that specifies the
 *     default script for glyphs that can't be mapped to a script
 *     automatically.  If set to `"none"` (which is the default), no script
 *     is used.  Valid values can be found in the header file
 *     `ttfautohint-scripts.h`.
 *
 * `symbol`
 * :   Set this integer to\ 1 if you want to process a font that ttfautohint
 *     would refuse otherwise because it can't find a single standard
 *     character for any of the supported scripts.  ttfautohint then uses a
 *     default (hinting) value for the standard stem width instead of
 *     deriving it from a script's set of standard characters (for the latin
 *     script, one of them is character 'o').  The default value of this
 *     option is\ 0.
 *
 * `fallback-stem-width`
 * :   Set the horizontal stem width (hinting) value for all scripts that
 *     lack proper standard characters.  The value is given in font units
 *     and must be a positive integer.  If not set, or the value is zero,
 *     ttfautohint uses a hard-coded default (50\ units at 2048 units per
 *     EM, and linearly scaled for other UPEM values, for example 24\ units
 *     at 1000 UPEM).
 *
 *     For symbol fonts (i.e., option `symbol` is given),
 *     `fallback-stem-width` has an effect only if `fallback-script` is set
 *     also.
 *
 *
 * ### Miscellaneous
 *
 * `ignore-restrictions`
 * :   If the font has set bit\ 1 in the 'fsType' field of the `OS/2` table,
 *     the ttfautohint library refuses to process the font since a
 *     permission to do that is required from the font's legal owner.  In
 *     case you have such a permission you might set the integer argument to
 *     value\ 1 to make ttfautohint handle the font.  The default value
 *     is\ 0.
 *
 * `TTFA-info`
 * :   If set to\ 1, ttfautohint creates an SFNT table called `TTFA` and
 *     fills it with information on the parameters used while calling
 *     `TTF_autohint`.  The format of the output data resembles the
 *     information at the very beginning of the dump emitted by option
 *     `debug`.  The default value is\ 0.
 *
 *     Main use of this option is for font editing purposes.  For example,
 *     after a font editor has added some glyphs, a front-end to
 *     `TTF_autohint` can parse `TTFA` and feed the parameters into another
 *     call of `TTF_autohint`.  The new glyphs are then hinted while hints
 *     of the old glyphs stay unchanged.
 *
 *     If this option is not set, and the font to be processed contains a
 *     `TTFA` table, it gets removed.
 *
 *     Note that such a `TTFA` table gets ignored by all font rendering
 *     engines.  In TrueType Collections, the `TTFA` table is added to the
 *     first subfont.
 *
 * `dehint`
 * :   If set to\ 1, remove all hints from the font.  All other hinting
 *     options are ignored.
 *
 *
 * ### Remarks
 *
 *   * Obviously, it is necessary to have an input and an output data
 *     stream.  All other options are optional.
 *
 *   * `hinting-range-min` and `hinting-range-max` specify the range for
 *     which the autohinter generates optimized hinting code.  If a PPEM
 *     value is smaller than the value of `hinting-range-min`, hinting still
 *     takes place but the configuration created for `hinting-range-min` is
 *     used.  The analogous action is taken for `hinting-range-max`, only
 *     limited by the value given with `hinting-limit`.  The font's `gasp`
 *     table is set up to always use grayscale rendering with grid-fitting
 *     for standard hinting, and symmetric grid-fitting and symmetric
 *     smoothing for horizontal subpixel hinting (ClearType).
 *
 *   * ttfautohint can process its own output a second time only if option
 *     `hint-composites` is not set (or if the font doesn't contain
 *     composite glyphs at all).  This limitation might change in the
 *     future.
 *
 * ```C
 */

TA_Error
TTF_autohint(const char* options,
             ...);

/*
 * ```
 *
 */

/* pandoc-end */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TTFAUTOHINT_H__ */

/* end of ttfautohint.h */
