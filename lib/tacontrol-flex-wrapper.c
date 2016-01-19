/* tacontrol-flex-wrapper.c */

/*
 * Copyright (C) 2014-2016 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/*
 * flex doesn't provide a means to load `config.h' early enough so that
 * stuff like _GNU_SOURCE can be defined before everything else.  As a
 * work-around, we compile this file.
 */

#include "config.h"

#include "tacontrol-flex.c"

/* end of tacontrol-flex-wrapper.c */
