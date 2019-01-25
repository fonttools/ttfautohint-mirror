/* taranges.h */

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


/* originally file `afranges.h' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */

#ifndef TARANGES_H_
#define TARANGES_H_


#include "tatypes.h"


#undef SCRIPT
#define SCRIPT(s, S, d, h, H, ss) \
          extern const TA_Script_UniRangeRec ta_ ## s ## _uniranges[];

#include "ttfautohint-scripts.h"

#undef SCRIPT
#define SCRIPT(s, S, d, h, H, ss) \
          extern const TA_Script_UniRangeRec ta_ ## s ## _nonbase_uniranges[];

#include "ttfautohint-scripts.h"

#endif /* TARANGES_H_ */

/* end of taranges.h */
