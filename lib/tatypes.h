/* tatypes.h */

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


/* originally file `aftypes.h' (2011-Mar-30) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#ifndef TATYPES_H_
#define TATYPES_H_

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "tablue.h"

#ifdef __cplusplus
extern "C" {
#endif


/* enable one of the following three definitions for debugging */
/* #define TA_DEBUG */
/* #define TA_DEBUG_HORZ */
#define TA_DEBUG_VERT

#if defined TA_DEBUG_HORZ
#  define TA_DEBUG_STARTDIM TA_DIMENSION_HORZ
#  define TA_DEBUG_ENDDIM TA_DIMENSION_HORZ
#  define TA_DEBUG
#elif defined TA_DEBUG_VERT
#  define TA_DEBUG_STARTDIM TA_DIMENSION_VERT
#  define TA_DEBUG_ENDDIM TA_DIMENSION_VERT
#  define TA_DEBUG
#elif defined TA_DEBUG
#  define TA_DEBUG_STARTDIM TA_DIMENSION_VERT
#  define TA_DEBUG_ENDDIM TA_DIMENSION_HORZ
#endif

#ifdef TA_DEBUG

#define TA_LOG(x) \
  do \
  { \
    if (_ta_debug) \
      _ta_message x; \
  } while (0)

#define TA_LOG_GLOBAL(x) \
  do \
  { \
    if (_ta_debug_global) \
      _ta_message x; \
  } while (0)

void
_ta_message(const char* format,
            ...);

extern int _ta_debug;
extern int _ta_debug_global;
extern int _ta_debug_disable_horz_hints;
extern int _ta_debug_disable_vert_hints;
extern int _ta_debug_disable_blue_hints;
extern void* _ta_debug_hints;

#else /* !TA_DEBUG */

#define TA_LOG(x) \
  do { } while (0) /* nothing */

#define TA_LOG_GLOBAL(x) \
  do { } while (0) /* nothing */

#endif /* !TA_DEBUG */


#define TA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define TA_MAX(a, b) ((a) > (b) ? (a) : (b))

#define TA_ABS(a) ((a) < 0 ? -(a) : (a))

/* from file `ftconfig.h' (2015-03-02) from FreeType */
/* typeof condition taken from gnulib's `intprops.h' header file */
#if (__GNUC__ >= 2 \
     ||  defined(__IBM__TYPEOF__) \
     || (__SUNPRO_C >= 0x5110 && !__STDC__))
#define TYPEOF(type) (__typeof__ (type))
#else
#define TYPEOF(type) /* empty */
#endif

/* from file `ftobjs.h' from FreeType */
/* we use the TYPEOF macro to suppress signedness compilation warnings */
#define TA_PAD_FLOOR(x, n) ((x) & ~TYPEOF(x)((n) - 1))
#define TA_PAD_ROUND(x, n) TA_PAD_FLOOR((x) + ((n) / 2), n)
#define TA_PAD_CEIL(x, n) TA_PAD_FLOOR((x) + ((n) - 1), n)

#define TA_PIX_FLOOR(x) ((x) & ~TYPEOF(x)63)
#define TA_PIX_ROUND(x) TA_PIX_FLOOR((x) + 32)
#define TA_PIX_CEIL(x) TA_PIX_FLOOR((x) + 63)


typedef struct TA_WidthRec_
{
  FT_Pos org; /* original position/width in font units */
  FT_Pos cur; /* current/scaled position/width in device subpixels */
  FT_Pos fit; /* current/fitted position/width in device subpixels */
} TA_WidthRec, *TA_Width;


/* the auto-hinter doesn't need a very high angular accuracy */
typedef FT_Int TA_Angle;

#define TA_ANGLE_PI 256
#define TA_ANGLE_2PI (TA_ANGLE_PI * 2)
#define TA_ANGLE_PI2 (TA_ANGLE_PI / 2)
#define TA_ANGLE_PI4 (TA_ANGLE_PI / 4)

#define TA_ANGLE_DIFF(result, angle1, angle2) \
          do \
          { \
            TA_Angle _delta = (angle2) - (angle1); \
\
\
            while (_delta <= -TA_ANGLE_PI) \
              _delta += TA_ANGLE_2PI; \
\
            if (_delta > TA_ANGLE_PI) \
              _delta -= TA_ANGLE_2PI; \
\
            result = _delta; \
          } while (0)


/* opaque handle to glyph-specific hints -- */
/* see `tahints.h' for more details */
typedef struct TA_GlyphHintsRec_* TA_GlyphHints;


/* a scaler models the target pixel device that will receive */
/* the auto-hinted glyph image */
#define TA_SCALER_FLAG_NO_HORIZONTAL 0x01U /* disable horizontal hinting */
#define TA_SCALER_FLAG_NO_VERTICAL 0x02U /* disable vertical hinting */
#define TA_SCALER_FLAG_NO_ADVANCE 0x04U /* disable advance hinting */
#define TA_SCALER_FLAG_NO_WARPER 0x08U /* disable warper */


typedef struct TA_ScalerRec_
{
  FT_Face face; /* source font face */
  FT_Fixed x_scale; /* from font units to 1/64th device pixels */
  FT_Fixed y_scale; /* from font units to 1/64th device pixels */
  FT_Pos x_delta; /* in 1/64th device pixels */
  FT_Pos y_delta; /* in 1/64th device pixels */
  FT_Render_Mode render_mode; /* monochrome, anti-aliased, LCD, etc.*/
  FT_UInt32 flags; /* additional control flags, see above */
} TA_ScalerRec, *TA_Scaler;

#define TA_SCALER_EQUAL_SCALES(a, b) \
          ((a)->x_scale == (b)->x_scale \
           && (a)->y_scale == (b)->y_scale \
           && (a)->x_delta == (b)->x_delta \
           && (a)->y_delta == (b)->y_delta)


typedef struct TA_StyleMetricsRec_* TA_StyleMetrics;

/* this function parses an FT_Face to compute global metrics */
/* for a specific style, optionally using another FT_Face to */
/* derive blue zones                                         */
typedef FT_Error
(*TA_WritingSystem_InitMetricsFunc)(TA_StyleMetrics metrics,
                                    FT_Face face,
                                    FT_Face reference);
typedef void
(*TA_WritingSystem_ScaleMetricsFunc)(TA_StyleMetrics metrics,
                                     TA_Scaler scaler);
typedef void
(*TA_WritingSystem_DoneMetricsFunc)(TA_StyleMetrics metrics);
typedef FT_Error
(*TA_WritingSystem_InitHintsFunc)(TA_GlyphHints hints,
                                  TA_StyleMetrics metrics);
typedef FT_Error
(*TA_WritingSystem_ApplyHintsFunc)(FT_UInt glyph_index,
                                   TA_GlyphHints hints,
                                   FT_Outline* outline,
                                   TA_StyleMetrics metrics);


/*
 * For the auto-hinter, a writing system consists of multiple scripts that
 * can be handled similarly *in a typographical way*; the relationship is
 * not based on history.  For example, both the Greek and the unrelated
 * Armenian scripts share the same features like ascender, descender,
 * x-height, etc.  Essentially, a writing system is covered by a
 * submodule of the auto-fitter; it contains
 *
 * - a specific global analyzer that computes global metrics specific to
 *   the script (based on script-specific characters to identify ascender
 *   height, x-height, etc.),
 *
 * - a specific glyph analyzer that computes segments and edges for each
 *   glyph covered by the script,
 *
 * - a specific grid-fitting algorithm that distorts the scaled glyph
 *   outline according to the results of the glyph analyzer.
 */

#define TAWRTSYS_H_ /* don't load header files */
#undef WRITING_SYSTEM
#define WRITING_SYSTEM(ws, WS) \
          TA_WRITING_SYSTEM_ ## WS,

/* The list of known writing systems. */
typedef enum TA_WritingSystem_
{
#include "tawrtsys.h"

  TA_WRITING_SYSTEM_MAX /* do not remove */
} TA_WritingSystem;

#undef TAWRTSYS_H_


typedef struct TA_WritingSystemClassRec_
{
  TA_WritingSystem writing_system;

  FT_Offset style_metrics_size;
  TA_WritingSystem_InitMetricsFunc style_metrics_init;
  TA_WritingSystem_ScaleMetricsFunc style_metrics_scale;
  TA_WritingSystem_DoneMetricsFunc style_metrics_done;

  TA_WritingSystem_InitHintsFunc style_hints_init;
  TA_WritingSystem_ApplyHintsFunc style_hints_apply;
} TA_WritingSystemClassRec;

typedef const TA_WritingSystemClassRec* TA_WritingSystemClass;


/*
 * Each script is associated with two sets of Unicode ranges to test
 * whether the font face supports the script, and which base characters
 * the script contains.
 *
 * We use four-letter script tags from the OpenType specification,
 * extended by `NONE', which indicates `no script'.
 */

#undef SCRIPT
#define SCRIPT(s, S, d, h, H, ss) \
          TA_SCRIPT_ ## S,

/* The list of known scripts. */
typedef enum TA_Script_
{
#include <ttfautohint-scripts.h>

  TA_SCRIPT_MAX /* do not remove */
} TA_Script;


typedef struct TA_Script_UniRangeRec_
{
  FT_UInt32 first;
  FT_UInt32 last;
} TA_Script_UniRangeRec;

#define TA_UNIRANGE_REC(a, b) \
          { (FT_UInt32)(a), (FT_UInt32)(b) }

#define TA_HINTING_BOTTOM_TO_TOP 0
#define TA_HINTING_TOP_TO_BOTTOM 1

typedef const TA_Script_UniRangeRec* TA_Script_UniRange;

typedef struct TA_ScriptClassRec_
{
  TA_Script script;

  /* last element in the ranges must be { 0, 0 } */
  TA_Script_UniRange script_uni_ranges;
  TA_Script_UniRange script_uni_nonbase_ranges;

  FT_Bool top_to_bottom_hinting;

  const char* standard_charstring; /* for default width and height */
} TA_ScriptClassRec;

typedef const TA_ScriptClassRec* TA_ScriptClass;


/*
 * Usually, a font contains more glyphs than can be addressed by its
 * character map.
 *
 * In the PostScript font world, encoding vectors specific to a given
 * task are used to select such glyphs, and these glyphs can be often
 * recognized by having a suffix in its glyph names.  For example, a
 * superscript glyph `A' might be called `A.sup'.  Unfortunately, this
 * naming scheme is not standardized and thus unusable for us.
 *
 * In the OpenType world, a better solution was invented, namely
 * `features', which cleanly separate a character's input encoding from
 * the corresponding glyph's appearance, and which don't use glyph names
 * at all.  For our purposes, and slightly generalized, an OpenType
 * feature is a name of a mapping that maps character codes to
 * non-standard glyph indices (features get used for other things also).
 * For example, the `sups' feature provides superscript glyphs, thus
 * mapping character codes like `A' or `B' to superscript glyph
 * representation forms.  How this mapping happens is completely
 * uninteresting to us.
 *
 * For the auto-hinter, a `coverage' represents all glyphs of an OpenType
 * feature collected in a set (as listed below) that can be hinted
 * together.  To continue the above example, superscript glyphs must not
 * be hinted together with normal glyphs because the blue zones
 * completely differ.
 *
 * To compute coverages, we use the HarfBuzz library, which has many
 * functions exactly for this purpose.
 *
 * TA_COVERAGE_DEFAULT is special: It should cover everything that isn't
 * listed separately (including the glyphs addressable by the character
 * map).
 */

#undef COVERAGE
#define COVERAGE(name, NAME, description, \
                 tag, tag1, tag2, tag3, tag4) \
          TA_COVERAGE_ ## NAME,

typedef enum TA_Coverage_
{
#include "ttfautohint-coverages.h"

  TA_COVERAGE_DEFAULT
} TA_Coverage;


/*
 * The topmost structure for modelling the auto-hinter glyph input data
 * is a `style class', grouping everything together.
 */

#undef STYLE
#define STYLE(s, S, d, ws, sc, ss, c) \
          TA_STYLE_ ## S,

/* The list of known styles. */
typedef enum TA_Style_
{
#include "tastyles.h"

  TA_STYLE_MAX /* do not remove */
} TA_Style;


typedef struct TA_StyleClassRec_
{
  TA_Style style;

  TA_WritingSystem writing_system;
  TA_Script script;
  TA_Blue_Stringset blue_stringset;
  TA_Coverage coverage;
} TA_StyleClassRec;

typedef const TA_StyleClassRec* TA_StyleClass;


typedef struct TA_FaceGlobalsRec_* TA_FaceGlobals;

/* This is the main structure that combines everything.  Autofit modules */
/* specific to writing systems derive their structures from it, for      */
/* example `TA_LatinMetrics'.                                            */

typedef struct TA_StyleMetricsRec_
{
  TA_StyleClass style_class;
  TA_ScalerRec scaler;
  FT_Bool digits_have_same_width;

  TA_FaceGlobals globals; /* to access properties */
} TA_StyleMetricsRec;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TATYPES_H_ */

/* end of tatypes.h */
