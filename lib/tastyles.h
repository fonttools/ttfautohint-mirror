/* tastyles.h */

/*
 * Copyright (C) 2014-2015 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `afstyles.h' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */


/* The following part can be included multiple times. */
/* Define `STYLE' as needed. */


/*
 * Add new styles here.  The first and second arguments are the
 * style name in lowercase and uppercase, respectively, followed
 * by a description string.  The next arguments are the
 * corresponding writing system, script, blue stringset, and
 * coverage.
 *
 * Note that styles using `TA_COVERAGE_DEFAULT' should always
 * come after styles with other coverages.
 *
 * Example:
 *
 *   STYLE(cyrl_dflt, CYRL_DFLT,
 *         "Cyrillic default style",
 *         TA_WRITING_SYSTEM_LATIN,
 *         TA_SCRIPT_CYRL,
 *         TA_BLUE_STRINGSET_CYRL,
 *         TA_COVERAGE_DEFAULT)
 */

#undef STYLE_LATIN
#define STYLE_LATIN(s, S, f, F, ds, df, C) \
          STYLE(s ## _ ## f, S ## _ ## F, \
                ds " " df " style", \
                TA_WRITING_SYSTEM_LATIN, \
                TA_SCRIPT_ ## S, \
                TA_BLUE_STRINGSET_ ## S, \
                TA_COVERAGE_ ## C)

#undef META_STYLE_LATIN
#define META_STYLE_LATIN(s, S, ds) \
          STYLE_LATIN(s, S, c2cp, C2CP, ds, \
                      "petite capticals from capitals", \
                      PETITE_CAPITALS_FROM_CAPITALS) \
          STYLE_LATIN(s, S, c2sc, C2SC, ds, \
                      "small capticals from capitals", \
                      SMALL_CAPITALS_FROM_CAPITALS) \
          STYLE_LATIN(s, S, ordn, ORDN, ds, \
                      "ordinals", \
                      ORDINALS) \
          STYLE_LATIN(s, S, pcap, PCAP, ds, \
                      "petite capitals", \
                      PETITE_CAPITALS) \
          STYLE_LATIN(s, S, sinf, SINF, ds, \
                      "scientific inferiors", \
                      SCIENTIFIC_INFERIORS) \
          STYLE_LATIN(s, S, smcp, SMCP, ds, \
                      "small capitals", \
                      SMALL_CAPITALS) \
          STYLE_LATIN(s, S, subs, SUBS, ds, \
                      "subscript", \
                      SUBSCRIPT) \
          STYLE_LATIN(s, S, sups, SUPS, ds, \
                      "superscript", \
                      SUPERSCRIPT) \
          STYLE_LATIN(s, S, titl, TITL, ds, \
                      "titling", \
                      TITLING) \
          STYLE_LATIN(s, S, dflt, DFLT, ds, \
                      "default", \
                      DEFAULT)

META_STYLE_LATIN(cyrl, CYRL, "Cyrillic")

STYLE(deva_dflt, DEVA_DFLT,
      "Devanagari default style",
      TA_WRITING_SYSTEM_LATIN,
      TA_SCRIPT_DEVA,
      TA_BLUE_STRINGSET_DEVA,
      TA_COVERAGE_DEFAULT)

META_STYLE_LATIN(grek, GREK, "Greek")

STYLE(hebr_dflt, HEBR_DFLT,
      "Hebrew default style",
      TA_WRITING_SYSTEM_LATIN,
      TA_SCRIPT_HEBR,
      TA_BLUE_STRINGSET_HEBR,
      TA_COVERAGE_DEFAULT)

META_STYLE_LATIN(latn, LATN, "Latin")

#ifdef FT_OPTION_AUTOFIT2
STYLE(ltn2_dflt, LTN2_DFLT,
      "Latin 2 default style",
      TA_WRITING_SYSTEM_LATIN2,
      TA_SCRIPT_LATN,
      TA_BLUE_STRINGSET_LATN,
      TA_COVERAGE_DEFAULT)
#endif

STYLE(telu_dflt, TELU_DFLT,
      "Telugu default style",
      TA_WRITING_SYSTEM_LATIN,
      TA_SCRIPT_TELU,
      TA_BLUE_STRINGSET_TELU,
      TA_COVERAGE_DEFAULT)

STYLE(none_dflt, NONE_DFLT,
      "no style",
      TA_WRITING_SYSTEM_DUMMY,
      TA_SCRIPT_NONE,
      (TA_Blue_Stringset)0,
      TA_COVERAGE_DEFAULT)

/* end of tastyles.h */
