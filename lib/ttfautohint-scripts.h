/* ttfautohint-scripts.h */

/*
 * Copyright (C) 2013-2014 by Werner Lemberg.
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
/* Define `SCRIPT' as needed. */


/*
 * Add new scripts here.  The first and second arguments are the
 * script name in lowercase and uppercase, respectively, followed
 * by a description string.  Then comes the corresponding HarfBuzz
 * script name tag, followed by the default character (to derive
 * the standard width and height of stems).
 */

SCRIPT(cyrl, CYRL,
       "Cyrillic",
       HB_SCRIPT_CYRILLIC,
       0x43E, 0x41E, 0x0) /* оО */

SCRIPT(grek, GREK,
       "Greek",
       HB_SCRIPT_GREEK,
       0x3BF, 0x39F, 0x0) /* οΟ */

SCRIPT(hebr, HEBR,
       "Hebrew",
       HB_SCRIPT_HEBREW,
       0x5DD, 0x0, 0x0) /* ם */

SCRIPT(latn, LATN,
       "Latin",
       HB_SCRIPT_LATIN,
       'o', 'O', '0')

SCRIPT(none, NONE,
       "no script",
       HB_SCRIPT_INVALID,
       0x0, 0x0, 0x0)

/* end of ttfautohint-scripts.h */
