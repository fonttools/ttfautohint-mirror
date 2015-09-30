/* taranges.c */

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


/* originally file `afranges.c' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */

#include "taranges.h"

/*
 * The algorithm for assigning properties and styles to the `glyph_styles'
 * array is as follows (cf. the implementation in
 * `af_face_globals_compute_style_coverage').
 *
 *   Walk over all scripts (as listed in `afscript.h').
 *
 *   For a given script, walk over all styles (as listed in `afstyles.h').
 *   The order of styles is important and should be as follows.
 *
 *   - First come styles based on OpenType features (small caps, for
 *     example).  Since features rely on glyph indices, thus completely
 *     bypassing character codes, no properties are assigned.
 *
 *   - Next comes the default style, using the character ranges as defined
 *     below.  This also assigns properties.
 *
 *   Note that there also exist fallback scripts, mainly covering
 *   superscript and subscript glyphs of a script that are not present as
 *   OpenType features.  Fallback scripts are defined below, also
 *   assigning properties; they are applied after the corresponding
 *   script.
 *
 */


/* XXX Check base character ranges again: */
/*     Right now, they are quickly derived by visual inspection. */
/*     I can imagine that fine-tuning is necessary. */

/* for the auto-hinter, a `non-base character' is something that should */
/* not be affected by blue zones, regardless of whether this is a */
/* spacing or no-spacing glyph */

/* the `ta_xxxx_nonbase_uniranges' ranges must be strict subsets */
/* of the corresponding `ta_xxxx_uniranges' ranges */


const TA_Script_UniRangeRec ta_arab_uniranges[] =
{
  TA_UNIRANGE_REC( 0x0600UL,  0x06FFUL), /* Arabic */
  TA_UNIRANGE_REC( 0x0750UL,  0x07FFUL), /* Arabic Supplement */
  TA_UNIRANGE_REC( 0x08A0UL,  0x08FFUL), /* Arabic Extended-A */
  TA_UNIRANGE_REC( 0xFB50UL,  0xFDFFUL), /* Arabic Presentation Forms-A */
  TA_UNIRANGE_REC( 0xFE70UL,  0xFEFFUL), /* Arabic Presentation Forms-B */
  TA_UNIRANGE_REC(0x1EE00UL, 0x1EEFFUL), /* Arabic Mathematical Alphabetic Symbols */
  TA_UNIRANGE_REC(      0UL,       0UL)
};

const TA_Script_UniRangeRec ta_arab_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0600UL, 0x0605UL),
  TA_UNIRANGE_REC(0x0610UL, 0x061AUL),
  TA_UNIRANGE_REC(0x064BUL, 0x065FUL),
  TA_UNIRANGE_REC(0x0670UL, 0x0670UL),
  TA_UNIRANGE_REC(0x06D6UL, 0x06DCUL),
  TA_UNIRANGE_REC(0x06DFUL, 0x06E4UL),
  TA_UNIRANGE_REC(0x06E7UL, 0x06E8UL),
  TA_UNIRANGE_REC(0x06EAUL, 0x06EDUL),
  TA_UNIRANGE_REC(0x08E3UL, 0x08FFUL),
  TA_UNIRANGE_REC(0xFBB2UL, 0xFBC1UL),
  TA_UNIRANGE_REC(0xFE70UL, 0xFE70UL),
  TA_UNIRANGE_REC(0xFE72UL, 0xFE72UL),
  TA_UNIRANGE_REC(0xFE74UL, 0xFE74UL),
  TA_UNIRANGE_REC(0xFE76UL, 0xFE76UL),
  TA_UNIRANGE_REC(0xFE78UL, 0xFE78UL),
  TA_UNIRANGE_REC(0xFE7AUL, 0xFE7AUL),
  TA_UNIRANGE_REC(0xFE7CUL, 0xFE7CUL),
  TA_UNIRANGE_REC(0xFE7EUL, 0xFE7EUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_cyrl_uniranges[] =
{
  TA_UNIRANGE_REC(0x0400UL, 0x04FFUL), /* Cyrillic */
  TA_UNIRANGE_REC(0x0500UL, 0x052FUL), /* Cyrillic Supplement */
  TA_UNIRANGE_REC(0x2DE0UL, 0x2DFFUL), /* Cyrillic Extended-A */
  TA_UNIRANGE_REC(0xA640UL, 0xA69FUL), /* Cyrillic Extended-B */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_cyrl_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0483UL, 0x0489UL),
  TA_UNIRANGE_REC(0x2DE0UL, 0x2DFFUL),
  TA_UNIRANGE_REC(0xA66FUL, 0xA67FUL),
  TA_UNIRANGE_REC(0xA69EUL, 0xA69FUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


/* there are some characters in the Devanagari Unicode block that are */
/* generic to Indic scripts; we omit them so that their presence doesn't */
/* trigger Devanagari */

const TA_Script_UniRangeRec ta_deva_uniranges[] =
{
  TA_UNIRANGE_REC(0x0900UL, 0x093BUL), /* Devanagari */
  /* omitting U+093C nukta */
  TA_UNIRANGE_REC(0x093DUL, 0x0950UL),
  /* omitting U+0951 udatta, U+0952 anudatta */
  TA_UNIRANGE_REC(0x0953UL, 0x0963UL),
  /* omitting U+0964 danda, U+0965 double danda */
  TA_UNIRANGE_REC(0x0966UL, 0x097FUL),
  TA_UNIRANGE_REC(0x20B9UL, 0x20B9UL), /* (new) Rupee sign */
  TA_UNIRANGE_REC(0xA8E0UL, 0xA8FFUL), /* Devanagari Extended */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_deva_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0900UL, 0x0902UL),
  TA_UNIRANGE_REC(0x093AUL, 0x093AUL),
  TA_UNIRANGE_REC(0x093CUL, 0x093CUL),
  TA_UNIRANGE_REC(0x0941UL, 0x0948UL),
  TA_UNIRANGE_REC(0x094DUL, 0x094DUL),
  TA_UNIRANGE_REC(0x0951UL, 0x0957UL),
  TA_UNIRANGE_REC(0x0962UL, 0x0963UL),
  TA_UNIRANGE_REC(0xA8E0UL, 0xA8F1UL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_grek_uniranges[] =
{
  TA_UNIRANGE_REC(0x0370UL, 0x03FFUL), /* Greek and Coptic */
  TA_UNIRANGE_REC(0x1F00UL, 0x1FFFUL), /* Greek Extended */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_grek_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x037AUL, 0x037AUL),
  TA_UNIRANGE_REC(0x0384UL, 0x0385UL),
  TA_UNIRANGE_REC(0x1FBDUL, 0x1FC1UL),
  TA_UNIRANGE_REC(0x1FCDUL, 0x1FCFUL),
  TA_UNIRANGE_REC(0x1FDDUL, 0x1FDFUL),
  TA_UNIRANGE_REC(0x1FEDUL, 0x1FEFUL),
  TA_UNIRANGE_REC(0x1FFDUL, 0x1FFEUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_hebr_uniranges[] =
{
  TA_UNIRANGE_REC(0x0590UL, 0x05FFUL), /* Hebrew */
  TA_UNIRANGE_REC(0xFB1DUL, 0xFB4FUL), /* Alphab. Present. Forms (Hebrew) */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_hebr_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0591UL, 0x05BFUL),
  TA_UNIRANGE_REC(0x05C1UL, 0x05C2UL),
  TA_UNIRANGE_REC(0x05C4UL, 0x05C5UL),
  TA_UNIRANGE_REC(0x05C7UL, 0x05C7UL),
  TA_UNIRANGE_REC(0xFB1EUL, 0xFB1EUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_lao_uniranges[] =
{
  TA_UNIRANGE_REC(0x0E80UL, 0x0EFFUL), /* Lao */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_lao_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0EB1UL, 0x0EB1UL),
  TA_UNIRANGE_REC(0x0EB4UL, 0x0EBCUL),
  TA_UNIRANGE_REC(0x0EC8UL, 0x0ECDUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_latn_uniranges[] =
{
  TA_UNIRANGE_REC( 0x0020UL,  0x007FUL), /* Basic Latin (no control chars) */
  TA_UNIRANGE_REC( 0x00A0UL,  0x00A9UL), /* Latin-1 Supplement (no control chars) */
  TA_UNIRANGE_REC( 0x00ABUL,  0x00B1UL),
  TA_UNIRANGE_REC( 0x00B4UL,  0x00B8UL),
  TA_UNIRANGE_REC( 0x00BBUL,  0x00FFUL),
  TA_UNIRANGE_REC( 0x0100UL,  0x017FUL), /* Latin Extended-A */
  TA_UNIRANGE_REC( 0x0180UL,  0x024FUL), /* Latin Extended-B */
  TA_UNIRANGE_REC( 0x0250UL,  0x02AFUL), /* IPA Extensions */
  TA_UNIRANGE_REC( 0x02B9UL,  0x02DFUL), /* Spacing Modifier Letters */
  TA_UNIRANGE_REC( 0x02E5UL,  0x02FFUL),
  TA_UNIRANGE_REC( 0x0300UL,  0x036FUL), /* Combining Diacritical Marks */
  TA_UNIRANGE_REC( 0x1AB0UL,  0x1ABEUL), /* Combining Diacritical Marks Extended */
  TA_UNIRANGE_REC( 0x1D00UL,  0x1D2BUL), /* Phonetic Extensions */
  TA_UNIRANGE_REC( 0x1D6BUL,  0x1D77UL),
  TA_UNIRANGE_REC( 0x1D79UL,  0x1D7FUL),
  TA_UNIRANGE_REC( 0x1D80UL,  0x1D9AUL), /* Phonetic Extensions Supplement */
  TA_UNIRANGE_REC( 0x1DC0UL,  0x1DFFUL), /* Combining Diacritical Marks Supplement */
  TA_UNIRANGE_REC( 0x1E00UL,  0x1EFFUL), /* Latin Extended Additional */
  TA_UNIRANGE_REC( 0x2000UL,  0x206FUL), /* General Punctuation */
  TA_UNIRANGE_REC( 0x20A0UL,  0x20B8UL), /* Currency Symbols ... */
  TA_UNIRANGE_REC( 0x20BAUL,  0x20CFUL), /* ... except new Rupee sign */
  TA_UNIRANGE_REC( 0x2150UL,  0x218FUL), /* Number Forms */
  TA_UNIRANGE_REC( 0x2C60UL,  0x2C7BUL), /* Latin Extended-C */
  TA_UNIRANGE_REC( 0x2C7EUL,  0x2C7FUL),
  TA_UNIRANGE_REC( 0x2E00UL,  0x2E7FUL), /* Supplemental Punctuation */
  TA_UNIRANGE_REC( 0xA720UL,  0xA76FUL), /* Latin Extended-D */
  TA_UNIRANGE_REC( 0xA771UL,  0xA7F7UL),
  TA_UNIRANGE_REC( 0xA7FAUL,  0xA7FFUL),
  TA_UNIRANGE_REC( 0xAB30UL,  0xAB5BUL), /* Latin Extended-E */
  TA_UNIRANGE_REC( 0xAB60UL,  0xAB6FUL),
  TA_UNIRANGE_REC( 0xFB00UL,  0xFB06UL), /* Alphab. Present. Forms (Latin Ligs) */
  TA_UNIRANGE_REC(0x1D400UL, 0x1D7FFUL), /* Mathematical Alphanumeric Symbols */
  TA_UNIRANGE_REC(      0UL,       0UL)
};

const TA_Script_UniRangeRec ta_latn_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x005EUL, 0x0060UL),
  TA_UNIRANGE_REC(0x007EUL, 0x007EUL),
  TA_UNIRANGE_REC(0x00A8UL, 0x00A9UL),
  TA_UNIRANGE_REC(0x00AEUL, 0x00B0UL),
  TA_UNIRANGE_REC(0x00B4UL, 0x00B4UL),
  TA_UNIRANGE_REC(0x00B8UL, 0x00B8UL),
  TA_UNIRANGE_REC(0x00BCUL, 0x00BEUL),
  TA_UNIRANGE_REC(0x02B9UL, 0x02DFUL),
  TA_UNIRANGE_REC(0x02E5UL, 0x02FFUL),
  TA_UNIRANGE_REC(0x0300UL, 0x036FUL),
  TA_UNIRANGE_REC(0x1AB0UL, 0x1ABEUL),
  TA_UNIRANGE_REC(0x1DC0UL, 0x1DFFUL),
  TA_UNIRANGE_REC(0x2017UL, 0x2017UL),
  TA_UNIRANGE_REC(0x203EUL, 0x203EUL),
  TA_UNIRANGE_REC(0xA788UL, 0xA788UL),
  TA_UNIRANGE_REC(0xA7F8UL, 0xA7FAUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_latb_uniranges[] =
{
  TA_UNIRANGE_REC(0x1D62UL, 0x1D6AUL),
  TA_UNIRANGE_REC(0x2080UL, 0x209CUL),
  TA_UNIRANGE_REC(0x2C7CUL, 0x2C7CUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_latb_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0UL, 0UL)
};


const TA_Script_UniRangeRec ta_latp_uniranges[] =
{
  TA_UNIRANGE_REC(0x00AAUL, 0x00AAUL),
  TA_UNIRANGE_REC(0x00B2UL, 0x00B3UL),
  TA_UNIRANGE_REC(0x00B9UL, 0x00BAUL),
  TA_UNIRANGE_REC(0x02B0UL, 0x02B8UL),
  TA_UNIRANGE_REC(0x02E0UL, 0x02E4UL),
  TA_UNIRANGE_REC(0x1D2CUL, 0x1D61UL),
  TA_UNIRANGE_REC(0x1D78UL, 0x1D78UL),
  TA_UNIRANGE_REC(0x1D9BUL, 0x1DBFUL),
  TA_UNIRANGE_REC(0x2070UL, 0x207FUL),
  TA_UNIRANGE_REC(0x2C7DUL, 0x2C7DUL),
  TA_UNIRANGE_REC(0xA770UL, 0xA770UL),
  TA_UNIRANGE_REC(0xA7F8UL, 0xA7F9UL),
  TA_UNIRANGE_REC(0xAB5CUL, 0xAB5FUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_latp_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0UL, 0UL)
};


const TA_Script_UniRangeRec ta_telu_uniranges[] =
{
  TA_UNIRANGE_REC(0x0C00UL, 0x0C7FUL), /* Telugu */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_telu_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0C00UL, 0x0C00UL),
  TA_UNIRANGE_REC(0x0C3EUL, 0x0C40UL),
  TA_UNIRANGE_REC(0x0C46UL, 0x0C56UL),
  TA_UNIRANGE_REC(0x0C62UL, 0x0C63UL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_thai_uniranges[] =
{
  TA_UNIRANGE_REC(0x0E00UL, 0x0E7FUL), /* Thai */
  TA_UNIRANGE_REC(     0UL,      0UL)
};

const TA_Script_UniRangeRec ta_thai_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0E31UL, 0x0E31UL),
  TA_UNIRANGE_REC(0x0E34UL, 0x0E3AUL),
  TA_UNIRANGE_REC(0x0E47UL, 0x0E4EUL),
  TA_UNIRANGE_REC(     0UL,      0UL)
};


const TA_Script_UniRangeRec ta_none_uniranges[] =
{
  TA_UNIRANGE_REC(0UL, 0UL)
};

const TA_Script_UniRangeRec ta_none_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0UL, 0UL)
};

/* end of taranges.c */
