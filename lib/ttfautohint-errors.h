/* ttfautohint-errors.h */

/*
 * Copyright (C) 2011-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


#ifndef TTFAUTOHINT_ERRORS_H_
#define TTFAUTOHINT_ERRORS_H_

/* We duplicate FreeType's error handling macros for simplicity; */
/* however, we don't use module-specific error codes. */

#undef TA_ERR_XCAT
#undef TA_ERR_CAT
#define TA_ERR_XCAT(x, y) x ## y
#define TA_ERR_CAT(x, y) TA_ERR_XCAT(x, y)

#undef TA_ERR_PREFIX
#define TA_ERR_PREFIX TA_Err_

#ifndef TA_ERRORDEF
#  define TA_ERRORDEF(e, v, s) e = v,
#  define TA_ERROR_START_LIST enum {
#  define TA_ERROR_END_LIST TA_ERR_CAT(TA_ERR_PREFIX, Max) };
#endif

#define TA_ERRORDEF_(e, v, s) \
          TA_ERRORDEF(TA_ERR_CAT(TA_ERR_PREFIX, e), v, s)
#define TA_NOERRORDEF_ TA_ERRORDEF_


/* The error codes. */

#ifdef TA_ERROR_START_LIST
  TA_ERROR_START_LIST
#endif

TA_NOERRORDEF_(Ok, 0x00,
               "no error")

TA_ERRORDEF_(Invalid_FreeType_Version, 0x0E,
             "invalid FreeType version (need 2.4.5 or higher)")
TA_ERRORDEF_(Missing_Legal_Permission, 0x0F,
             "legal permission bit in `OS/2' font table is set")
TA_ERRORDEF_(Invalid_Stream_Write,     0x5F,
             "invalid stream write")
TA_ERRORDEF_(Hinter_Overflow,          0xF0,
             "hinter overflow")
TA_ERRORDEF_(Missing_Glyph,            0xF1,
             "missing standard character glyph")
TA_ERRORDEF_(Missing_Unicode_CMap,     0xF2,
             "missing Unicode character map")
TA_ERRORDEF_(Missing_Symbol_CMap,      0xF3,
             "missing symbol character map")
TA_ERRORDEF_(Canceled,                 0xF4,
             "execution canceled")
TA_ERRORDEF_(Already_Processed,        0xF5,
             "font already processed by ttfautohint")
TA_ERRORDEF_(Invalid_Font_Type,        0xF6,
             "not a font with TrueType outlines in SFNT format")
TA_ERRORDEF_(Unknown_Argument,         0xF7,
             "unknown argument")

TA_ERRORDEF_(XHeightSnapping_Invalid_Character,  0x101,
             "invalid character")
TA_ERRORDEF_(XHeightSnapping_Overflow,           0x102,
             "numerical overflow")
TA_ERRORDEF_(XHeightSnapping_Invalid_Range,      0x103,
             "invalid range")
TA_ERRORDEF_(XHeightSnapping_Overlapping_Ranges, 0x104,
             "overlapping ranges")
TA_ERRORDEF_(XHeightSnapping_Not_Ascending,      0x105,
             "not ascending ranges or values")
TA_ERRORDEF_(XHeightSnapping_Allocation_Error,   0x106,
             "allocation error")

TA_ERRORDEF_(Control_Syntax_Error,         0x201,
             "syntax error")
TA_ERRORDEF_(Control_Invalid_Font_Index,   0x202,
             "invalid font index")
TA_ERRORDEF_(Control_Invalid_Glyph_Index,  0x203,
             "invalid glyph index")
TA_ERRORDEF_(Control_Invalid_Glyph_Name,   0x204,
             "invalid glyph name")
TA_ERRORDEF_(Control_Invalid_Character,    0x205,
             "invalid character")
TA_ERRORDEF_(Control_Invalid_Style,        0x206,
             "invalid style")
TA_ERRORDEF_(Control_Invalid_Script,       0x207,
             "invalid script")
TA_ERRORDEF_(Control_Invalid_Feature,      0x208,
             "invalid feature")
TA_ERRORDEF_(Control_Invalid_Shift,        0x209,
             "invalid shift")
TA_ERRORDEF_(Control_Invalid_Offset,       0x20A,
             "invalid offset")
TA_ERRORDEF_(Control_Invalid_Range,        0x20B,
             "invalid range")
TA_ERRORDEF_(Control_Invalid_Glyph,        0x20C,
             "invalid glyph")
TA_ERRORDEF_(Control_Overflow,             0x20D,
             "overflow")
TA_ERRORDEF_(Control_Overlapping_Ranges,   0x20E,
             "overlapping ranges")
TA_ERRORDEF_(Control_Ranges_Not_Ascending, 0x20F,
             "ranges not ascending")
TA_ERRORDEF_(Control_Allocation_Error,     0x210,
             "allocation error")
TA_ERRORDEF_(Control_Flex_Error,           0x211,
             "internal flex error")
TA_ERRORDEF_(Control_Too_Much_Widths,      0x212,
             "too much stem width values")

/* error codes in the range 0x300-0x3FF are related to the reference font; */
/* subtract 0x300 to get the normal FreeType meaning */

#ifdef TA_ERROR_END_LIST
  TA_ERROR_END_LIST
#endif


#undef TA_ERROR_START_LIST
#undef TA_ERROR_END_LIST
#undef TA_ERRORDEF
#undef TA_ERRORDEF_
#undef TA_NOERRORDEF_

#endif /* TTFAUTOHINT_ERRORS_H_ */

/* end of ttfautohint-errors.h */
