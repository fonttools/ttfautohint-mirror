/* ttfautohint-scripts.h */

/*
 * Copyright (C) 2013-2015 by Werner Lemberg.
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
 * script name tag, followed by the default characters (to derive
 * the standard width of stems).
 *
 * Note that fallback scripts only have a default style, thus we
 * use `HB_SCRIPT_INVALID' as the HarfBuzz script name tag for
 * them.
 */

SCRIPT(arab, ARAB,
       "Arabic",
       HB_SCRIPT_ARABIC,
       "\xD9\x84 \xD8\xAD \xD9\x80") /* ل ح ـ */

SCRIPT(cyrl, CYRL,
       "Cyrillic",
       HB_SCRIPT_CYRILLIC,
       "\xD0\xBE \xD0\x9E") /* о О */

SCRIPT(deva, DEVA,
       "Devanagari",
       HB_SCRIPT_DEVANAGARI,
       "\xE0\xA4\xA0 \xE0\xA4\xB5 \xE0\xA4\x9F") /* ठ व ट */

SCRIPT(grek, GREK,
       "Greek",
       HB_SCRIPT_GREEK,
       "\xCE\xBF \xCE\x9F") /* ο Ο */

SCRIPT(hebr, HEBR,
       "Hebrew",
       HB_SCRIPT_HEBREW,
       "\xD7\x9D") /* ם */

/* only digit zero has a simple shape in the Khmer script */
SCRIPT(khmr, KHMR,
       "Khmer",
       HB_SCRIPT_KHMER,
       "\xE1\x9F\xA0") /* ០ */

SCRIPT(khms, KHMS,
       "Khmer Symbols",
       HB_SCRIPT_INVALID,
       "\xE1\xA7\xA1 \xE1\xA7\xAA") /* ᧡ ᧪ */

/* only digit zero has a simple shape in the Lao script */
SCRIPT(lao, LAO,
       "Lao",
       HB_SCRIPT_LAO,
       "\xE0\xBB\x90") /* ໐ */

SCRIPT(latn, LATN,
       "Latin",
       HB_SCRIPT_LATIN,
       "o O 0")

SCRIPT(latb, LATB,
       "Latin Subscript Fallback",
       HB_SCRIPT_INVALID,
       "\xE2\x82\x92 \xE2\x82\x80") /* ₒ ₀ */

SCRIPT(latp, LATP,
       "Latin Superscript Fallback",
       HB_SCRIPT_INVALID,
       "\xE1\xB5\x92 \xE1\xB4\xBC \xE2\x81\xB0") /* ᵒ ᴼ ⁰ */

/* there are no simple forms for letters; we thus use two digit shapes */
SCRIPT(telu, TELU,
       "Telugu",
       HB_SCRIPT_TELUGU,
       "\xE0\xB1\xA6 \xE0\xB1\xA7") /* ౦ ౧ */

SCRIPT(thai, THAI,
       "Thai",
       HB_SCRIPT_THAI,
       "\xE0\xB8\xB2 \xE0\xB9\x85 \xE0\xB9\x90") /* า ๅ ๐ */

SCRIPT(none, NONE,
       "no script",
       HB_SCRIPT_INVALID,
       "")

/* end of ttfautohint-scripts.h */
