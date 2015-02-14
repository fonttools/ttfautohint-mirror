/* tatime.c */

/*
 * Copyright (C) 2011-2015 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


#include <time.h>
#include <stdint.h>

#include "ta.h"

typedef uint64_t TA_ULongLong;


void
TA_get_current_time(FT_ULong* high,
                    FT_ULong* low)
{
  /* there have been 24107 days between January 1st, 1904 (the epoch of */
  /* OpenType), and January 1st, 1970 (the epoch of the `time' function) */
  TA_ULongLong seconds_to_1970 = 24107 * 24 * 60 * 60;
  TA_ULongLong seconds_to_today = seconds_to_1970 + (TA_ULongLong)time(NULL);


  *high = (FT_ULong)(seconds_to_today >> 32);
  *low = (FT_ULong)seconds_to_today;
}

/* end of tatime.c */
