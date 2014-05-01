/* taranges.c */

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


/* originally file `afranges.c' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */

#include "taranges.h"


const TA_Script_UniRangeRec ta_cyrl_uniranges[] =
{
  TA_UNIRANGE_REC(0x0400UL, 0x04FFUL), /* Cyrillic */
  TA_UNIRANGE_REC(0x0500UL, 0x052FUL), /* Cyrillic Supplement */
  TA_UNIRANGE_REC(0x2DE0UL, 0x2DFFUL), /* Cyrillic Extended-A */
  TA_UNIRANGE_REC(0xA640UL, 0xA69FUL), /* Cyrillic Extended-B */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_deva_uniranges[] =
{
  TA_UNIRANGE_REC(0x0900UL, 0x097FUL), /* Devanagari */
  TA_UNIRANGE_REC(0x20B9UL, 0x20B9UL), /* (new) Rupee sign */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_grek_uniranges[] =
{
  TA_UNIRANGE_REC(0x0370UL, 0x03FFUL), /* Greek and Coptic */
  TA_UNIRANGE_REC(0x1F00UL, 0x1FFFUL), /* Greek Extended */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_hebr_uniranges[] =
{
  TA_UNIRANGE_REC(0x0590UL, 0x05FFUL), /* Hebrew */
  TA_UNIRANGE_REC(0xFB1DUL, 0xFB4FUL), /* Alphab. Present. Forms (Hebrew) */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_latn_uniranges[] =
{
  TA_UNIRANGE_REC( 0x0020UL,  0x007FUL), /* Basic Latin (no control chars) */
  TA_UNIRANGE_REC( 0x00A0UL,  0x00FFUL), /* Latin-1 Supplement (no control chars) */
  TA_UNIRANGE_REC( 0x0100UL,  0x017FUL), /* Latin Extended-A */
  TA_UNIRANGE_REC( 0x0180UL,  0x024FUL), /* Latin Extended-B */
  TA_UNIRANGE_REC( 0x0250UL,  0x02AFUL), /* IPA Extensions */
  TA_UNIRANGE_REC( 0x02B0UL,  0x02FFUL), /* Spacing Modifier Letters */
  TA_UNIRANGE_REC( 0x0300UL,  0x036FUL), /* Combining Diacritical Marks */
  TA_UNIRANGE_REC( 0x1D00UL,  0x1D7FUL), /* Phonetic Extensions */
  TA_UNIRANGE_REC( 0x1D80UL,  0x1DBFUL), /* Phonetic Extensions Supplement */
  TA_UNIRANGE_REC( 0x1DC0UL,  0x1DFFUL), /* Combining Diacritical Marks Supplement */
  TA_UNIRANGE_REC( 0x1E00UL,  0x1EFFUL), /* Latin Extended Additional */
  TA_UNIRANGE_REC( 0x2000UL,  0x206FUL), /* General Punctuation */
  TA_UNIRANGE_REC( 0x2070UL,  0x209FUL), /* Superscripts and Subscripts */
  TA_UNIRANGE_REC( 0x20A0UL,  0x20B8UL), /* Currency Symbols ... */
  TA_UNIRANGE_REC( 0x20BAUL,  0x20CFUL), /* ... except new Rupee sign */
  TA_UNIRANGE_REC( 0x2150UL,  0x218FUL), /* Number Forms */
  TA_UNIRANGE_REC( 0x2460UL,  0x24FFUL), /* Enclosed Alphanumerics */
  TA_UNIRANGE_REC( 0x2C60UL,  0x2C7FUL), /* Latin Extended-C */
  TA_UNIRANGE_REC( 0x2E00UL,  0x2E7FUL), /* Supplemental Punctuation */
  TA_UNIRANGE_REC( 0xA720UL,  0xA7FFUL), /* Latin Extended-D */
  TA_UNIRANGE_REC( 0xFB00UL,  0xFB06UL), /* Alphab. Present. Forms (Latin Ligs) */
  TA_UNIRANGE_REC(0x1D400UL, 0x1D7FFUL), /* Mathematical Alphanumeric Symbols */
  TA_UNIRANGE_REC(0x1F100UL, 0x1F1FFUL), /* Enclosed Alphanumeric Supplement */
  TA_UNIRANGE_REC(      0UL,       0UL)
};

const TA_Script_UniRangeRec ta_none_uniranges[] =
{
  TA_UNIRANGE_REC(0UL, 0UL)
};

/* end of taranges.c */
