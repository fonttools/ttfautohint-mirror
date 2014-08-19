/* tascript.c */

/*
 * Copyright (C) 2014 by Werner Lemberg.
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


#undef SCRIPT
#define SCRIPT(s, S, d, h, sc1, sc2, sc3) #s,

const char* script_names[] =
{

#include <ttfautohint-scripts.h>

};

/* end of tascript.c */
