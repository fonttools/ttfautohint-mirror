/* ttfautohint-coverages.h */

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

/* originally file `afcover.h' (2014-01-09) from FreeType */


/* This header file can be included multiple times. */
/* Define `COVERAGE' as needed. */


/*
 * Add new coverages here.  The first and second arguments are the coverage
 * name in lowercase and uppercase, respectively, followed by a description
 * string.  The last five arguments are a four-character identifier and its
 * four characters defining the corresponding OpenType feature.
 */

#if 0
/* XXX: It's not possible to define blue zone characters in advance. */
COVERAGE(alternative_fractions, ALTERNATIVE_FRACTIONS,
         "alternative fractions",
         afrc, 'a', 'f', 'r', 'c')
#endif

COVERAGE(petite_capitals_from_capitals, PETITE_CAPITALS_FROM_CAPITALS,
         "petite capitals from capitals",
         c2cp, 'c', '2', 'c', 'p')

COVERAGE(small_capitals_from_capitals, SMALL_CAPITALS_FROM_CAPITALS,
         "small capitals from capitals",
         c2sc, 'c', '2', 's', 'c')

#if 0
/* XXX: Only digits are in this coverage, however, both normal style */
/*      and oldstyle representation forms are possible.              */
COVERAGE(denominators, DENOMINATORS,
         "denominators",
         dnom, 'd', 'n', 'o', 'm')
#endif

#if 0
/* XXX: It's not possible to define blue zone characters in advance. */
COVERAGE(fractions, FRACTIONS,
         "fractions",
         frac, 'f', 'r', 'a', 'c')
#endif

#if 0
/* XXX: Only digits are in this coverage, however, both normal style */
/*      and oldstyle representation forms are possible.              */
COVERAGE(numerators, NUMERATORS,
         "numerators",
         numr, 'n', 'u', 'm', 'r')
#endif

COVERAGE(ordinals, ORDINALS,
         "ordinals",
         ordn, 'o', 'r', 'd', 'n')

COVERAGE(petite_capitals, PETITE_CAPITALS,
         "petite capitals",
         pcap, 'p', 'c', 'a', 'p')

COVERAGE(ruby, RUBY,
         "ruby",
         ruby, 'r', 'u', 'b', 'y')

COVERAGE(scientific_inferiors, SCIENTIFIC_INFERIORS,
         "scientific inferiors",
         sinf, 's', 'i', 'n', 'f')

COVERAGE(small_capitals, SMALL_CAPITALS,
         "small capitals",
         smcp, 's', 'm', 'c', 'p')

COVERAGE(subscript, SUBSCRIPT,
         "subscript",
         subs, 's', 'u', 'b', 's')

COVERAGE(superscript, SUPERSCRIPT,
         "superscript",
         sups, 's', 'u', 'p', 's')

COVERAGE(titling, TITLING,
         "titling",
         titl, 't', 'i', 't', 'l')

#if 0
  /* to be always excluded */
COVERAGE(nalt, 'n', 'a', 'l', 't'); /* Alternate Annotation Forms (?) */
COVERAGE(ornm, 'o', 'r', 'n', 'm'); /* Ornaments (?) */
#endif


/* end of ttfautohint-coverages.h */
