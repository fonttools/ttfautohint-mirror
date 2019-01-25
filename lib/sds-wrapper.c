/* sds-wrapper.c */

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


/* Load `config.h' before library source code. */
#include "config.h"

/* for va_copy, via gnulib -- */
/* we don't enforce a fully C99-compliant compiler */
#include <stdarg.h>

#include "sds.c"

/* end of sds-wrapper.c */
