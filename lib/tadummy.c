/* tadummy.c */

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


/* originally file `afdummy.c' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#include "tadummy.h"
#include "tahints.h"


static FT_Error
ta_dummy_hints_init(TA_GlyphHints hints,
                    TA_ScriptMetrics metrics)
{
  ta_glyph_hints_rescale(hints, metrics);

  hints->x_scale = metrics->scaler.x_scale;
  hints->y_scale = metrics->scaler.y_scale;
  hints->x_delta = metrics->scaler.x_delta;
  hints->y_delta = metrics->scaler.y_delta;

  return FT_Err_Ok;
}


static FT_Error
ta_dummy_hints_apply(TA_GlyphHints hints,
                     FT_Outline* outline)
{
  FT_Error error;


  error = ta_glyph_hints_reload(hints, outline);
  if (!error)
    ta_glyph_hints_save(hints, outline);

  return error;
}


const TA_WritingSystemClassRec ta_dummy_writing_system_class =
{
  TA_WRITING_SYSTEM_DUMMY,

  sizeof (TA_ScriptMetricsRec),

  (TA_Script_InitMetricsFunc)NULL,
  (TA_Script_ScaleMetricsFunc)NULL,
  (TA_Script_DoneMetricsFunc)NULL,

  (TA_Script_InitHintsFunc)ta_dummy_hints_init,
  (TA_Script_ApplyHintsFunc)ta_dummy_hints_apply
};


const TA_ScriptClassRec ta_none_script_class =
{
  TA_SCRIPT_NONE,
  (TA_Blue_Stringset)0,
  TA_WRITING_SYSTEM_DUMMY,

  NULL,
  '\0'
};

/* end of tadummy.c */
