/* tashaper.h */

/*
 * Copyright (C) 2014-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `hbshim.h' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */

#ifndef TASHAPER_H_
#define TASHAPER_H_


#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ot.h>
#include <hb-ft.h>

#include "ta.h"


/* in file `tafeature.c' */
extern const hb_tag_t feature_tags[];
extern size_t feature_tags_size;


FT_Error
ta_shaper_get_coverage(TA_FaceGlobals globals,
                       TA_StyleClass style_class,
                       FT_UShort* gstyles,
                       FT_UInt* sample_glyph,
                       FT_Bool default_script);

void*
ta_shaper_buf_create(FT_Face face);

void
ta_shaper_buf_destroy(FT_Face face,
                      void* buf);

const char*
ta_shaper_get_cluster(const char* p,
                      TA_StyleMetrics metrics,
                      void* buf_,
                      unsigned int* count);

FT_ULong
ta_shaper_get_elem(TA_StyleMetrics metrics,
                   void* buf_,
                   unsigned int idx,
                   FT_Long* x_advance,
                   FT_Long* y_offset);

#endif /* TASHAPER_H_ */

/* end of tashaper.h */
