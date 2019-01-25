/* taversion.c */

/*
 * Copyright (C) 2017-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */

#include "ta.h"


/* the function declaration is in `ttfautohint.h' */

TA_LIB_EXPORT void
TTF_autohint_version(int *major,
                     int *minor,
                     int *revision)
{
  *major = TTFAUTOHINT_MAJOR;
  *minor = TTFAUTOHINT_MINOR;
  *revision = TTFAUTOHINT_REVISION;
}


/* the function declaration is in `ttfautohint.h' */

TA_LIB_EXPORT const char*
TTF_autohint_version_string(void)
{
  return TTFAUTOHINT_VERSION;
}

/* end of taversion.c */
