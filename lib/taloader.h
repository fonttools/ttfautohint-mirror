/* taloader.h */

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


/* originally file `afloader.h' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#ifndef TALOADER_H_
#define TALOADER_H_

#include "tahints.h"
#include "tagloadr.h"


typedef struct FONT_ FONT;

typedef struct TA_LoaderRec_
{
  /* current face data */
  FT_Face face;
  TA_FaceGlobals globals;

  /* current glyph data */
  TA_GlyphLoader gloader;
  TA_GlyphHintsRec hints;
  TA_StyleMetrics metrics;
  FT_Bool transformed;
  FT_Matrix trans_matrix;
  FT_Vector trans_delta;
  FT_Vector pp1;
  FT_Vector pp2;
  /* we don't handle vertical phantom points */
} TA_LoaderRec, *TA_Loader;


FT_Error
ta_loader_init(FONT* font);


FT_Error
ta_loader_reset(FONT* font,
                FT_Face face);


void
ta_loader_done(FONT* font);


FT_Error
ta_loader_load_glyph(FONT* font,
                     FT_Face face,
                     FT_UInt gindex,
                     FT_Int32 load_flags);


void
ta_loader_register_hints_recorder(TA_Loader loader,
                                  TA_Hints_Recorder hints_recorder,
                                  void* user);

#endif /* TALOADER_H_ */

/* end of taloader.h */
