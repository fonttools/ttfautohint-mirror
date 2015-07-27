/* taharfbuzz.h */

/*
 * Copyright (C) 2014-2015 by Werner Lemberg.
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

#ifndef __TAHARFBUZZ_H__
#define __TAHARFBUZZ_H__


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
ta_get_coverage(TA_FaceGlobals globals,
                TA_StyleClass style_class,
                FT_Byte* gstyles);

FT_Error
ta_get_char_index(TA_StyleMetrics metrics,
                  FT_ULong charcode,
                  FT_ULong *codepoint,
                  FT_Long *y_offset);

#endif /* __TAHARFBUZZ_H__ */

/* end of taharfbuzz.h */
