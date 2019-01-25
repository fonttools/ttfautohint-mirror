/* tadummy.c */

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


/* originally file `afdummy.c' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#include "tadummy.h"
#include "tahints.h"


static FT_Error
ta_dummy_hints_init(TA_GlyphHints hints,
                    TA_StyleMetrics metrics)
{
  ta_glyph_hints_rescale(hints, metrics);

  hints->x_scale = metrics->scaler.x_scale;
  hints->y_scale = metrics->scaler.y_scale;
  hints->x_delta = metrics->scaler.x_delta;
  hints->y_delta = metrics->scaler.y_delta;

  return FT_Err_Ok;
}


static FT_Error
ta_dummy_hints_apply(FT_UInt glyph_index,
                     TA_GlyphHints hints,
                     FT_Outline* outline)
{
  FT_Error error;

  FT_UNUSED(glyph_index);


  error = ta_glyph_hints_reload(hints, outline);
  if (!error)
    ta_glyph_hints_save(hints, outline);

  return error;
}


const TA_WritingSystemClassRec ta_dummy_writing_system_class =
{
  TA_WRITING_SYSTEM_DUMMY,

  sizeof (TA_StyleMetricsRec),

  (TA_WritingSystem_InitMetricsFunc)NULL, /* style_metrics_init */
  (TA_WritingSystem_ScaleMetricsFunc)NULL, /* style_metrics_scale */
  (TA_WritingSystem_DoneMetricsFunc)NULL, /* style_metrics_done */

  (TA_WritingSystem_InitHintsFunc)ta_dummy_hints_init, /* style_hints_init */
  (TA_WritingSystem_ApplyHintsFunc)ta_dummy_hints_apply /* style_hints_apply */
};

/* end of tadummy.c */
