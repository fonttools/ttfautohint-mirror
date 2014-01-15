/* taglobal.c */

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


/* originally file `afglobal.c' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#include <stdlib.h>

#include "taglobal.h"

/* get writing system specific header files */
#undef WRITING_SYSTEM
#define WRITING_SYSTEM(ws, WS) /* empty */
#include "tawrtsys.h"


#undef WRITING_SYSTEM
#define WRITING_SYSTEM(ws, WS) \
          &ta_ ## ws ## _writing_system_class,

TA_WritingSystemClass const ta_writing_system_classes[] =
{

#include "tawrtsys.h"

  NULL /* do not remove */
};


#undef SCRIPT
#define SCRIPT(s, S, d) \
          &ta_ ## s ## _script_class,

TA_ScriptClass const ta_script_classes[] =
{

#include <ttfautohint-scripts.h>

  NULL  /* do not remove */
};


#ifdef TA_DEBUG

#undef STYLE
#define STYLE(s, S, d) #s,

const char* ta_style_names[] =
{

#include <tastyles.h>

};

#endif /* TA_DEBUG */


/* Compute the style index of each glyph within a given face. */

static FT_Error
ta_face_globals_compute_style_coverage(TA_FaceGlobals globals)
{
  FT_Error error;
  FT_Face face = globals->face;
  FT_CharMap old_charmap = face->charmap;
  FT_Byte* gstyles = globals->glyph_styles;
  FT_UInt ss;
  FT_UInt i;


  /* the value TA_STYLE_UNASSIGNED means `uncovered glyph' */
  memset(globals->glyph_styles, TA_STYLE_UNASSIGNED, globals->glyph_count);

  error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
  if (error)
  {
    /* ignore this error; we simply use the fallback style */
    /* XXX: Shouldn't we rather disable hinting? */
    error = FT_Err_Ok;
    goto Exit;
  }

  /* scan each style in a Unicode charmap */
  for (ss = 0; ta_style_classes[ss]; ss++)
  {
    TA_StyleClass style_class = ta_style_classes[ss];
    TA_Script_UniRange range;


    if (script_class->script_uni_ranges == NULL)
      continue;

    /* scan all Unicode points in the range and */
    /* set the corresponding glyph style index */
    for (range = script_class->script_uni_ranges; range->first != 0; range++)
    {
      FT_ULong charcode = range->first;
      FT_UInt gindex;


      gindex = FT_Get_Char_Index(face, charcode);

      if (gindex != 0
          && gindex < (FT_ULong)globals->glyph_count
          && gstyles[gindex] == TA_STYLE_UNASSIGNED)
        gstyles[gindex] = (FT_Byte)ss;

      for (;;)
      {
        charcode = FT_Get_Next_Char(face, charcode, &gindex);

        if (gindex == 0 || charcode > range->last)
          break;

        if (gindex < (FT_ULong)globals->glyph_count
            && gstyles[gindex] == TA_STYLE_UNASSIGNED)
          gstyles[gindex] = (FT_Byte)ss;
      }
    }
  }

  /* mark ASCII digits */
  for (i = 0x30; i <= 0x39; i++)
  {
    FT_UInt gindex = FT_Get_Char_Index(face, i);


    if (gindex != 0
        && gindex < (FT_ULong)globals->glyph_count)
      gstyles[gindex] |= TA_DIGIT;
  }

Exit:
  /* by default, all uncovered glyphs are set to the fallback style */
  /* XXX: Shouldn't we disable hinting or do something similar? */
  if (globals->font->fallback_style != TA_STYLE_UNASSIGNED)
  {
    FT_Long nn;


    for (nn = 0; nn < globals->glyph_count; nn++)
    {
      if ((gstyles[nn] & ~TA_DIGIT) == TA_STYLE_UNASSIGNED)
      {
        gstyles[nn] &= ~TA_STYLE_UNASSIGNED;
        gstyles[nn] |= globals->font->fallback_style;
      }
    }
  }

  FT_Set_Charmap(face, old_charmap);
  return error;
}


FT_Error
ta_face_globals_new(FT_Face face,
                    TA_FaceGlobals *aglobals,
                    FONT* font)
{
  FT_Error error;
  TA_FaceGlobals globals;


  globals = (TA_FaceGlobals)calloc(1, sizeof (TA_FaceGlobalsRec) +
                                      face->num_glyphs * sizeof (FT_Byte));
  if (!globals)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err;
  }

  globals->face = face;
  globals->glyph_count = face->num_glyphs;
  globals->glyph_styles = (FT_Byte*)(globals + 1);
  globals->font = font;

  error = ta_face_globals_compute_style_coverage(globals);
  if (error)
  {
    ta_face_globals_free(globals);
    globals = NULL;
  }

  globals->increase_x_height = TA_PROP_INCREASE_X_HEIGHT_MAX;

Err:
  *aglobals = globals;
  return error;
}


void
ta_face_globals_free(TA_FaceGlobals globals)
{
  if (globals)
  {
    FT_UInt nn;


    for (nn = 0; nn < TA_STYLE_MAX; nn++)
    {
      if (globals->metrics[nn])
      {
        TA_StyleClass style_class =
          ta_style_classes[nn];
        TA_WritingSystemClass writing_system_class =
          ta_writing_system_classes[style_class->writing_system];


        if (writing_system_class->style_metrics_done)
          writing_system_class->style_metrics_done(globals->metrics[nn]);

        free(globals->metrics[nn]);
        globals->metrics[nn] = NULL;
      }
    }

    globals->glyph_count = 0;
    globals->glyph_styles = NULL; /* no need to free this one! */
    globals->face = NULL;

    free(globals);
    globals = NULL;
  }
}


FT_Error
ta_face_globals_get_metrics(TA_FaceGlobals globals,
                            FT_UInt gindex,
                            FT_UInt options,
                            TA_StyleMetrics *ametrics)
{
  TA_StyleMetrics metrics = NULL;
  TA_Style style = (TA_Style)(options & 15);
  TA_WritingSystemClass writing_system_class;
  TA_StyleClass style_class;
  FT_Error error = FT_Err_Ok;


  if (gindex >= (FT_ULong)globals->glyph_count)
  {
    error = FT_Err_Invalid_Argument;
    goto Exit;
  }

  /* if we have a forced style (via `options'), use it, */
  /* otherwise look into `glyph_styles' array */
  if (style == TA_STYLE_NONE || style + 1 >= TA_STYLE_MAX)
    style = (TA_Style)(globals->glyph_styles[gindex]
                       & TA_STYLE_UNASSIGNED);

  style_class =
    ta_style_classes[style];
  writing_system_class =
    ta_writing_system_classes[style_class->writing_system];

  metrics = globals->metrics[style];
  if (metrics == NULL)
  {
    /* create the global metrics object if necessary */
    metrics = (TA_StyleMetrics)
                calloc(1, writing_system_class->style_metrics_size);
    if (!metrics)
    {
      error = FT_Err_Out_Of_Memory;
      goto Exit;
    }

    metrics->style_class = style_class;
    metrics->globals = globals;

    if (writing_system_class->style_metrics_init)
    {
      error = writing_system_class->style_metrics_init(metrics,
                                                       globals->face);
      if (error)
      {
        if (writing_system_class->style_metrics_done)
          writing_system_class->style_metrics_done(metrics);

        free(metrics);
        metrics = NULL;
        goto Exit;
      }
    }

    globals->metrics[style] = metrics;
  }

Exit:
  *ametrics = metrics;

  return error;
}


FT_Bool
ta_face_globals_is_digit(TA_FaceGlobals globals,
                         FT_UInt gindex)
{
  if (gindex < (FT_ULong)globals->glyph_count)
    return (FT_Bool)(globals->glyph_styles[gindex] & TA_DIGIT);

  return (FT_Bool)0;
}

/* end of taglobal.c */
