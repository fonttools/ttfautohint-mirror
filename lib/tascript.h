/* tascript.h */

/*
 * Copyright (C) 2013 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `afscript.h' (2013-Aug-05) from FreeType */


/* The following part can be included multiple times. */
/* Define `SCRIPT' as needed.                         */


/* Add new scripts here. */

SCRIPT(dflt, DFLT)
SCRIPT(latn, LATN)
#if 0
SCRIPT(hani, HANI)
SCRIPT(deva, DEVA)
#endif
#ifdef FT_OPTION_AUTOFIT2
SCRIPT(ltn2, LTN2)
#endif

/* end of tascript.h */
