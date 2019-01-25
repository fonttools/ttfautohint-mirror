/* tasort.h */

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


/* originally part of file `aftypes.h' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#ifndef TASORT_H_
#define TASORT_H_

#include "tatypes.h"

#ifdef __cplusplus
extern "C" {
#endif


void
ta_sort_pos(FT_UInt count,
            FT_Pos* table);

void
ta_sort_and_quantize_widths(FT_UInt* count,
                            TA_Width widths,
                            FT_Pos threshold);

#ifdef __cplusplus
}
#endif

#endif /* TASORT_H_ */

/* end of tasort.h */
