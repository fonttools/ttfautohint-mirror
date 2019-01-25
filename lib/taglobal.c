/* taglobal.c */

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


/* originally file `afglobal.c' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#include <stdlib.h>

#include "taglobal.h"
#include "taranges.h"


#undef SCRIPT
#define SCRIPT(s, S, d, h, H, ss) \
          const TA_ScriptClassRec ta_ ## s ## _script_class = \
          { \
            TA_SCRIPT_ ## S, \
            ta_ ## s ## _uniranges, \
            ta_ ## s ## _nonbase_uniranges, \
            TA_ ## H, \
            ss \
          };

#include <ttfautohint-scripts.h>


#undef STYLE
#define STYLE(s, S, d, ws, sc, ss, c) \
          const TA_StyleClassRec ta_ ## s ## _style_class = \
          { \
            TA_STYLE_ ## S, \
            ws, \
            sc, \
            ss, \
            c \
          };

#include "tastyles.h"


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
#define SCRIPT(s, S, d, h, H, ss) \
          &ta_ ## s ## _script_class,

TA_ScriptClass const ta_script_classes[] =
{

#include <ttfautohint-scripts.h>

  NULL /* do not remove */
};


#undef STYLE
#define STYLE(s, S, d, ws, sc, ss, c) \
          &ta_ ## s ## _style_class,

TA_StyleClass const ta_style_classes[] =
{

#include "tastyles.h"

  NULL /* do not remove */
};


#ifdef TA_DEBUG

#undef STYLE
#define STYLE(s, S, d, ws, sc, ss, c) #s,

const char* ta_style_names[] =
{

#include <tastyles.h>

};

#endif /* TA_DEBUG */


/* Recursively assign a style to all components of a composite glyph. */

static FT_Error
ta_face_globals_scan_composite(FT_Face face,
                               FT_Long gindex,
                               FT_UShort gstyle,
                               FT_UShort* gstyles,
                               FT_Int nesting_level)
{
  FT_Error error;
  FT_UInt i;

  FT_GlyphSlot glyph;

  FT_Int* subglyph_indices = NULL;
  FT_UInt used_subglyphs;

  FT_Int p_index;
  FT_UInt p_flags;
  FT_Int p_arg1;
  FT_Int p_arg2;
  FT_Matrix p_transform;


  /* limit recursion arbitrarily */
  if (nesting_level > 100)
    return FT_Err_Invalid_Table;

  error = FT_Load_Glyph(face, (FT_UInt)gindex, FT_LOAD_NO_RECURSE);
  if (error)
    return error;

  glyph = face->glyph;

  /* in FreeType < 2.5.4, FT_Get_SubGlyph_Info always returns an error */
  /* due to a bug even if the call was successful; */
  /* for this reason we do the argument checking by ourselves */
  /* and ignore the returned error code of FT_Get_SubGlyph_Info */
  if (!glyph->subglyphs
      || glyph->format != FT_GLYPH_FORMAT_COMPOSITE)
    return FT_Err_Ok;

  /* since a call to FT_Load_Glyph changes `glyph', */
  /* we have to first store the subglyph indices, then do the recursion */
  subglyph_indices = (FT_Int*)malloc(sizeof (FT_Int) * glyph->num_subglyphs);
  if (!subglyph_indices)
    return FT_Err_Out_Of_Memory;

  used_subglyphs = 0;
  for (i = 0; i < glyph->num_subglyphs; i++)
  {
    (void)FT_Get_SubGlyph_Info(glyph,
                               i,
                               &p_index,
                               &p_flags,
                               &p_arg1,
                               &p_arg2,
                               &p_transform);

    if (p_index >= face->num_glyphs
        || (gstyles[p_index] & TA_STYLE_MASK) != TA_STYLE_UNASSIGNED)
      continue;

    /* only take subglyphs that are not shifted vertically; */
    /* otherwise blue zones don't fit */
    if (p_flags & ARGS_ARE_XY_VALUES
        && p_arg2 == 0)
    {
      gstyles[p_index] = gstyle;
      subglyph_indices[used_subglyphs++] = p_index;
    }
  }

  /* recurse */
  for (i = 0; i < used_subglyphs; i++)
  {
    error = ta_face_globals_scan_composite(face,
                                           subglyph_indices[i],
                                           gstyle,
                                           gstyles,
                                           nesting_level + 1);
    if (error)
      break;
  }

  free(subglyph_indices);

  return error;
}


/* Compute the style index of each glyph within a given face. */

static FT_Error
ta_face_globals_compute_style_coverage(TA_FaceGlobals globals)
{
  FT_Error error;
  FT_Face face = globals->face;
  FT_CharMap old_charmap = face->charmap;
  FT_UShort* gstyles = globals->glyph_styles;
  FT_UInt ss;
  FT_UInt i;
  FT_UInt dflt = ~0U; /* a non-valid value */


  /* the value TA_STYLE_UNASSIGNED means `uncovered glyph' */
  for (i = 0; i < (unsigned int)globals->glyph_count; i++)
    gstyles[i] = TA_STYLE_UNASSIGNED;

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
    FT_UInt* sample_glyph = &globals->sample_glyphs[ss];
    TA_ScriptClass script_class = ta_script_classes[style_class->script];
    TA_Script_UniRange range;


    if (!script_class->script_uni_ranges->first)
      continue;

    /* scan all Unicode points in the range and */
    /* set the corresponding glyph style index */
    if (style_class->coverage == TA_COVERAGE_DEFAULT)
    {
      if ((FT_UInt)style_class->script == globals->font->default_script)
        dflt = ss;

      for (range = script_class->script_uni_ranges;
           range->first != 0;
           range++)
      {
        FT_ULong charcode = range->first;
        FT_UInt gindex;


        gindex = FT_Get_Char_Index(face, charcode);

        if (gindex != 0
            && gindex < (FT_ULong)globals->glyph_count
            && (gstyles[gindex] & TA_STYLE_MASK) == TA_STYLE_UNASSIGNED)
        {
          gstyles[gindex] = (FT_UShort)ss;
          if (!*sample_glyph)
            *sample_glyph = gindex;
        }

        for (;;)
        {
          charcode = FT_Get_Next_Char(face, charcode, &gindex);

          if (gindex == 0 || charcode > range->last)
            break;

          if (gindex < (FT_ULong)globals->glyph_count
              && (gstyles[gindex] & TA_STYLE_MASK) == TA_STYLE_UNASSIGNED)
          {
            gstyles[gindex] = (FT_UShort)ss;
            if (!*sample_glyph)
              *sample_glyph = gindex;
          }
        }
      }

      /* do the same for the script's non-base characters */
      for (range = script_class->script_uni_nonbase_ranges;
           range->first != 0;
           range++)
      {
        FT_ULong charcode = range->first;
        FT_UInt gindex;


        gindex = FT_Get_Char_Index(face, charcode);

        if (gindex != 0
            && gindex < (FT_ULong)globals->glyph_count
            && (gstyles[gindex] & TA_STYLE_MASK) == (FT_UShort)ss)
        {
          gstyles[gindex] |= TA_NONBASE;
          if (!*sample_glyph)
            *sample_glyph = gindex;
        }

        for (;;)
        {
          charcode = FT_Get_Next_Char(face, charcode, &gindex);

          if (gindex == 0 || charcode > range->last)
            break;

          if (gindex < (FT_ULong)globals->glyph_count
              && (gstyles[gindex] & TA_STYLE_MASK) == (FT_UShort)ss)
          {
            gstyles[gindex] |= TA_NONBASE;
            if (!*sample_glyph)
              *sample_glyph = gindex;
          }
        }
      }
    }
    else
    {
      /* get glyphs not directly addressable by cmap */
      ta_shaper_get_coverage(globals,
                             style_class,
                             gstyles,
                             sample_glyph,
                             0);
    }
  }

  /* handle the remaining default OpenType features ... */
  for (ss = 0; ta_style_classes[ss]; ss++)
  {
    TA_StyleClass style_class = ta_style_classes[ss];
    FT_UInt* sample_glyph = &globals->sample_glyphs[ss];


    if (style_class->coverage == TA_COVERAGE_DEFAULT)
      ta_shaper_get_coverage(globals,
                             style_class,
                             gstyles,
                             sample_glyph,
                             0);
  }

  /* ... and finally the default OpenType features of the default script */
  ta_shaper_get_coverage(globals,
                         ta_style_classes[dflt],
                         gstyles,
                         &globals->sample_glyphs[dflt],
                         1);

  /* mark ASCII digits */
  for (i = 0x30; i <= 0x39; i++)
  {
    FT_UInt gindex = FT_Get_Char_Index(face, i);


    if (gindex != 0
        && gindex < (FT_ULong)globals->glyph_count)
      gstyles[gindex] |= TA_DIGIT;
  }

  /* now walk over all glyphs and check for composites: */
  /* since subglyphs are hinted separately if option `hint-composites' */
  /* isn't set, we have to tag them with style indices, too */
  {
    FT_Long nn;


    /* no need for updating `sample_glyphs'; */
    /* the composite itself is certainly a valid sample glyph */

    for (nn = 0; nn < globals->glyph_count; nn++)
    {
      if ((gstyles[nn] & TA_STYLE_MASK) == TA_STYLE_UNASSIGNED)
        continue;

      error = ta_face_globals_scan_composite(globals->face,
                                             nn,
                                             gstyles[nn],
                                             gstyles,
                                             0);
      if (error)
        return error;
    }
  }

Exit:
  /* by default, all uncovered glyphs are set to the fallback style */
  /* XXX: Shouldn't we disable hinting or do something similar? */
  if (globals->font->fallback_style != (TA_Style)TA_STYLE_UNASSIGNED)
  {
    FT_Long nn;


    for (nn = 0; nn < globals->glyph_count; nn++)
    {
      if ((gstyles[nn] & TA_STYLE_MASK) == TA_STYLE_UNASSIGNED)
      {
        gstyles[nn] &= ~TA_STYLE_MASK;
        gstyles[nn] |= globals->font->fallback_style;
      }
    }
  }

#ifdef TA_DEBUG

  if (face->num_faces > 1)
    TA_LOG_GLOBAL(("\n"
                   "style coverage (subfont %d, glyf table index %d)\n"
                   "================================================\n"
                   "\n",
                   face->face_index,
                   globals->font->sfnts[face->face_index].glyf_idx));
  else
    TA_LOG_GLOBAL(("\n"
                   "style coverage\n"
                   "==============\n"
                   "\n"));

  for (ss = 0; ta_style_classes[ss]; ss++)
  {
    TA_StyleClass style_class = ta_style_classes[ss];
    FT_UInt count = 0;
    FT_Long idx;


    TA_LOG_GLOBAL(("%s:\n", ta_style_names[style_class->style]));

    for (idx = 0; idx < globals->glyph_count; idx++)
    {
      if ((gstyles[idx] & TA_STYLE_MASK) == style_class->style)
      {
        if (!(count % 10))
          TA_LOG_GLOBAL((" "));

        TA_LOG_GLOBAL((" %d", idx));
        count++;

        if (!(count % 10))
          TA_LOG_GLOBAL(("\n"));
      }
    }

    if (!count)
      TA_LOG_GLOBAL(("  (none)\n"));
    if (count % 10)
      TA_LOG_GLOBAL(("\n"));
  }

#endif /* TA_DEBUG */

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


  /* we allocate an TA_FaceGlobals structure together */
  /* with the glyph_styles array */
  globals = (TA_FaceGlobals)calloc(
              1, sizeof (TA_FaceGlobalsRec) +
                 (FT_ULong)face->num_glyphs * sizeof (FT_UShort));
  if (!globals)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err;
  }

  globals->face = face;
  globals->glyph_count = face->num_glyphs;
  /* right after the globals structure come the glyph styles */
  globals->glyph_styles = (FT_UShort*)(globals + 1);
  globals->font = font;
  globals->hb_font = hb_ft_font_create(face, NULL);
  globals->hb_buf = hb_buffer_create();

  error = ta_face_globals_compute_style_coverage(globals);
  if (error)
  {
    ta_face_globals_free(globals);
    globals = NULL;
  }
  else
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
      }
    }

    hb_font_destroy(globals->hb_font);
    hb_buffer_destroy(globals->hb_buf);

    /* no need to free `globals->glyph_styles'; */
    /* it is part of the `globals' array */
    free(globals);
  }
}


FT_Error
ta_face_globals_get_metrics(TA_FaceGlobals globals,
                            FT_UInt gindex,
                            FT_UInt options,
                            TA_StyleMetrics *ametrics)
{
  TA_StyleMetrics metrics = NULL;
  TA_Style style = (TA_Style)options;
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
  if (style == TA_STYLE_NONE_DFLT || style + 1 >= TA_STYLE_MAX)
    style = (TA_Style)(globals->glyph_styles[gindex]
                       & TA_STYLE_UNASSIGNED);

  style_class =
    ta_style_classes[style];
  writing_system_class =
    ta_writing_system_classes[style_class->writing_system];

  metrics = globals->metrics[style];
  if (!metrics)
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
      error = writing_system_class->style_metrics_init(
                                      metrics,
                                      globals->face,
                                      globals->font->reference);
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
