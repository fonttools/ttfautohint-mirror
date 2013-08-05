/* tatypes.h */

/*
 * Copyright (C) 2011-2013 by Werner Lemberg.
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

#ifndef __TATYPES_H__
#define __TATYPES_H__

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_OUTLINE_H


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

void
_ta_message(const char* format,
            ...);

extern int _ta_debug;
extern int _ta_debug_disable_horz_hints;
extern int _ta_debug_disable_vert_hints;
extern int _ta_debug_disable_blue_hints;
extern void* _ta_debug_hints;

#else /* !TA_DEBUG */

#define TA_LOG(x) \
  do { } while (0) /* nothing */

#endif /* !TA_DEBUG */


#define TA_ABS(a) ((a) < 0 ? -(a) : (a))

/* from file `ftobjs.h' from FreeType */
#define TA_PAD_FLOOR(x, n) ((x) & ~((n) - 1))
#define TA_PAD_ROUND(x, n) TA_PAD_FLOOR((x) + ((n) / 2), n)
#define TA_PAD_CEIL(x, n) TA_PAD_FLOOR((x) + ((n) - 1), n)

#define TA_PIX_FLOOR(x) ((x) & ~63)
#define TA_PIX_ROUND(x) TA_PIX_FLOOR((x) + 32)
#define TA_PIX_CEIL(x) TA_PIX_FLOOR((x) + 63)


typedef struct TA_WidthRec_
{
  FT_Pos org; /* original position/width in font units */
  FT_Pos cur; /* current/scaled position/width in device sub-pixels */
  FT_Pos fit; /* current/fitted position/width in device sub-pixels */
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
            _delta %= TA_ANGLE_2PI; \
            if (_delta < 0) \
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
#define TA_SCALER_FLAG_NO_HORIZONTAL 0x01 /* disable horizontal hinting */
#define TA_SCALER_FLAG_NO_VERTICAL 0x02 /* disable vertical hinting */
#define TA_SCALER_FLAG_NO_ADVANCE 0x04 /* disable advance hinting */

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


/* This is the main structure which combines writing systems and script */
/* data (for a given face object, see below). */

typedef struct TA_WritingSystemClassRec_ const* TA_WritingSystemClass;
typedef struct TA_ScriptClassRec_ const* TA_ScriptClass;
typedef struct TA_FaceGlobalsRec_* TA_FaceGlobals;

typedef struct TA_ScriptMetricsRec_
{
  TA_ScriptClass script_class;
  TA_ScalerRec scaler;
  FT_Bool digits_have_same_width;

  TA_FaceGlobals globals; /* to access properties */
} TA_ScriptMetricsRec, *TA_ScriptMetrics;


/* this function parses an FT_Face to compute global metrics */
/* for a specific script */
typedef FT_Error
(*TA_Script_InitMetricsFunc)(TA_ScriptMetrics metrics,
                             FT_Face face);
typedef void
(*TA_Script_ScaleMetricsFunc)(TA_ScriptMetrics metrics,
                              TA_Scaler scaler);
typedef void
(*TA_Script_DoneMetricsFunc)(TA_ScriptMetrics metrics);
typedef FT_Error
(*TA_Script_InitHintsFunc)(TA_GlyphHints hints,
                           TA_ScriptMetrics metrics);
typedef void
(*TA_Script_ApplyHintsFunc)(TA_GlyphHints hints,
                            FT_Outline* outline,
                            TA_ScriptMetrics metrics);


/*
 *  In FreeType, a writing system consists of multiple scripts which can
 *  be handled similarly *in a typographical way*; the relationship is not
 *  based on history.  For example, both the Greek and the unrelated
 *  Armenian scripts share the same features like ascender, descender,
 *  x-height, etc.  Essentially, a writing system is covered by a
 *  submodule of the auto-fitter; it contains
 *
 *  - a specific global analyzer which computes global metrics specific to
 *    the script (based on script-specific characters to identify ascender
 *    height, x-height, etc.),
 *
 *  - a specific glyph analyzer that computes segments and edges for each
 *    glyph covered by the script,
 *
 *  - a specific grid-fitting algorithm that distorts the scaled glyph
 *    outline according to the results of the glyph analyzer.
 */

#define __TAWRTSYS_H__ /* don't load header files */
#undef WRITING_SYSTEM
#define WRITING_SYSTEM(ws, WS) \
          TA_WRITING_SYSTEM_ ## WS,

/* The list of known writing systems. */
typedef enum TA_WritingSystem_
{
#include "tawrtsys.h"

  TA_WRITING_SYSTEM_MAX /* do not remove */
} TA_WritingSystem;

#undef __TAWRTSYS_H__


typedef struct TA_WritingSystemClassRec_
{
  TA_WritingSystem writing_system;

  FT_Offset script_metrics_size;
  TA_Script_InitMetricsFunc script_metrics_init;
  TA_Script_ScaleMetricsFunc script_metrics_scale;
  TA_Script_DoneMetricsFunc script_metrics_done;

  TA_Script_InitHintsFunc script_hints_init;
  TA_Script_ApplyHintsFunc script_hints_apply;
} TA_WritingSystemClassRec;


/*
 *  Each script is associated with a set of Unicode ranges which gets used
 *  to test whether the font face supports the script.  It also references
 *  the writing system it belongs to.
 *
 *  We use four-letter script tags from the OpenType specification.
 */

#undef SCRIPT
#define SCRIPT(s, S) \
          TA_SCRIPT_ ## S,

/* The list of known scripts. */
typedef enum TA_Script_
{

#include "tascript.h"

  TA_SCRIPT_MAX /* do not remove */
} TA_Script;


typedef struct TA_Script_UniRangeRec_
{
  FT_UInt32 first;
  FT_UInt32 last;
} TA_Script_UniRangeRec;

#define TA_UNIRANGE_REC(a, b) \
          { (FT_UInt32)(a), (FT_UInt32)(b) }

typedef const TA_Script_UniRangeRec* TA_Script_UniRange;

typedef struct TA_ScriptClassRec_
{
  TA_Script script;
  TA_WritingSystem writing_system;

  TA_Script_UniRange script_uni_ranges; /* last must be { 0, 0 } */
  FT_UInt32 standard_char; /* for default width and height */
} TA_ScriptClassRec;

#endif /* __TATYPES_H__ */

/* end of tatypes.h */
