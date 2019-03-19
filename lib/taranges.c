/* taranges.c */

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


/* originally file `afranges.c' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */

#include "taranges.h"

/*
 * This file gets also processed with the `taranges.sed' script to produce
 * documentation of character ranges.  To make this simple approach work,
 * don't change the formatting in this file!
 *
 *   - Everything before the first `const' keyword (starting a line) gets
 *     removed.
 *
 *   - The line containing `none_uniranges' and everything after it gets
 *     removed, too.
 *
 *   - Comments after a `TA_UNIRANGE_REC' entry are used for character range
 *     documentation within a table.
 *
 *   - Comments indented by two spaces are also used within a table.
 *
 *   - Other comments are inserted into the documentation as-is (after
 *     stripping off the comment characters).
 */


/*
 * The algorithm for assigning properties and styles to the `glyph_styles'
 * array is as follows (cf. the implementation in
 * `ta_face_globals_compute_style_coverage').
 *
 *   Walk over all scripts (as listed in `tascript.h').
 *
 *   For a given script, walk over all styles (as listed in `tastyles.h').
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


const TA_Script_UniRangeRec ta_adlm_uniranges[] =
{
  TA_UNIRANGE_REC(0x1E900, 0x1E95F), /* Adlam */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_adlm_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x1D944, 0x1E94A),
  TA_UNIRANGE_REC(      0,       0)
};


const TA_Script_UniRangeRec ta_arab_uniranges[] =
{
  TA_UNIRANGE_REC( 0x0600,  0x06FF), /* Arabic */
  TA_UNIRANGE_REC( 0x0750,  0x07FF), /* Arabic Supplement */
  TA_UNIRANGE_REC( 0x08A0,  0x08FF), /* Arabic Extended-A */
  TA_UNIRANGE_REC( 0xFB50,  0xFDFF), /* Arabic Presentation Forms-A */
  TA_UNIRANGE_REC( 0xFE70,  0xFEFF), /* Arabic Presentation Forms-B */
  TA_UNIRANGE_REC(0x1EE00, 0x1EEFF), /* Arabic Mathematical Alphabetic Symbols */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_arab_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0600, 0x0605),
  TA_UNIRANGE_REC(0x0610, 0x061A),
  TA_UNIRANGE_REC(0x064B, 0x065F),
  TA_UNIRANGE_REC(0x0670, 0x0670),
  TA_UNIRANGE_REC(0x06D6, 0x06DC),
  TA_UNIRANGE_REC(0x06DF, 0x06E4),
  TA_UNIRANGE_REC(0x06E7, 0x06E8),
  TA_UNIRANGE_REC(0x06EA, 0x06ED),
  TA_UNIRANGE_REC(0x08D3, 0x08FF),
  TA_UNIRANGE_REC(0xFBB2, 0xFBC1),
  TA_UNIRANGE_REC(0xFE70, 0xFE70),
  TA_UNIRANGE_REC(0xFE72, 0xFE72),
  TA_UNIRANGE_REC(0xFE74, 0xFE74),
  TA_UNIRANGE_REC(0xFE76, 0xFE76),
  TA_UNIRANGE_REC(0xFE78, 0xFE78),
  TA_UNIRANGE_REC(0xFE7A, 0xFE7A),
  TA_UNIRANGE_REC(0xFE7C, 0xFE7C),
  TA_UNIRANGE_REC(0xFE7E, 0xFE7E),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_armn_uniranges[] =
{
  TA_UNIRANGE_REC(0x0530, 0x058F), /* Armenian */
  TA_UNIRANGE_REC(0xFB13, 0xFB17), /* Alphab. Present. Forms (Armenian) */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_armn_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0559, 0x055F),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_avst_uniranges[] =
{
  TA_UNIRANGE_REC(0x10B00, 0x10B3F), /* Avestan */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_avst_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x10B39, 0x10B3F),
  TA_UNIRANGE_REC(      0,       0)
};


const TA_Script_UniRangeRec ta_bamu_uniranges[] =
{
  TA_UNIRANGE_REC( 0xA6A0,  0xA6FF), /* Bamum */
#if 0
  /* The characters in the Bamum supplement are pictograms, */
  /* not (directly) related to the syllabic Bamum script */
  TA_UNIRANGE_REC(0x16800, 0x16A3F), /* Bamum Supplement */
#endif
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_bamu_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0xA6F0,  0xA6F1),
  TA_UNIRANGE_REC(     0,       0)
};


const TA_Script_UniRangeRec ta_beng_uniranges[] =
{
  TA_UNIRANGE_REC(0x0980, 0x09FF), /* Bengali */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_beng_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0981, 0x0981),
  TA_UNIRANGE_REC(0x09BC, 0x09BC),
  TA_UNIRANGE_REC(0x09C1, 0x09C4),
  TA_UNIRANGE_REC(0x09CD, 0x09CD),
  TA_UNIRANGE_REC(0x09E2, 0x09E3),
  TA_UNIRANGE_REC(0x09FE, 0x09FE),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_buhd_uniranges[] =
{
  TA_UNIRANGE_REC(0x1740,  0x175F), /* Buhid */
  TA_UNIRANGE_REC(     0,       0)
};

const TA_Script_UniRangeRec ta_buhd_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x1752,  0x1753),
  TA_UNIRANGE_REC(     0,       0)
};


const TA_Script_UniRangeRec ta_cakm_uniranges[] =
{
  TA_UNIRANGE_REC(0x11100, 0x1114F), /* Chakma */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_cakm_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x11100, 0x11102),
  TA_UNIRANGE_REC(0x11127, 0x11134),
  TA_UNIRANGE_REC(0x11146, 0x11146),
  TA_UNIRANGE_REC(      0,       0)
};


const TA_Script_UniRangeRec ta_cans_uniranges[] =
{
  TA_UNIRANGE_REC(0x1400,  0x167F), /* Unified Canadian Aboriginal Syllabics */
  TA_UNIRANGE_REC(0x18B0,  0x18FF), /* Unified Canadian Aboriginal Syllabics Extended */
  TA_UNIRANGE_REC(     0,       0)
};

const TA_Script_UniRangeRec ta_cans_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_cari_uniranges[] =
{
  TA_UNIRANGE_REC(0x102A0, 0x102DF), /* Carian */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_cari_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_cher_uniranges[] =
{
  TA_UNIRANGE_REC(0x13A0, 0x13FF), /* Cherokee */
  TA_UNIRANGE_REC(0xAB70, 0xABBF), /* Cherokee Supplement */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_cher_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_copt_uniranges[] =
{
  TA_UNIRANGE_REC(0x2C80, 0x2CFF), /* Coptic */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_copt_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x2CEF, 0x2CF1),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_cprt_uniranges[] =
{
  TA_UNIRANGE_REC(0x10800, 0x1083F), /* Cypriot */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_cprt_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_cyrl_uniranges[] =
{
  TA_UNIRANGE_REC(0x0400, 0x04FF), /* Cyrillic */
  TA_UNIRANGE_REC(0x0500, 0x052F), /* Cyrillic Supplement */
  TA_UNIRANGE_REC(0x2DE0, 0x2DFF), /* Cyrillic Extended-A */
  TA_UNIRANGE_REC(0xA640, 0xA69F), /* Cyrillic Extended-B */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_cyrl_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0483, 0x0489),
  TA_UNIRANGE_REC(0x2DE0, 0x2DFF),
  TA_UNIRANGE_REC(0xA66F, 0xA67F),
  TA_UNIRANGE_REC(0xA69E, 0xA69F),
  TA_UNIRANGE_REC(     0,      0)
};


/* There are some characters in the Devanagari Unicode block that are */
/* generic to Indic scripts; we omit them so that their presence doesn't */
/* trigger Devanagari. */

const TA_Script_UniRangeRec ta_deva_uniranges[] =
{
  TA_UNIRANGE_REC(0x0900, 0x093B), /* Devanagari */
  /* omitting U+093C nukta */
  TA_UNIRANGE_REC(0x093D, 0x0950), /* ... continued */
  /* omitting U+0951 udatta, U+0952 anudatta */
  TA_UNIRANGE_REC(0x0953, 0x0963), /* ... continued */
  /* omitting U+0964 danda, U+0965 double danda */
  TA_UNIRANGE_REC(0x0966, 0x097F), /* ... continued */
  TA_UNIRANGE_REC(0x20B9, 0x20B9), /* (new) Rupee sign */
  TA_UNIRANGE_REC(0xA8E0, 0xA8FF), /* Devanagari Extended */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_deva_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0900, 0x0902),
  TA_UNIRANGE_REC(0x093A, 0x093A),
  TA_UNIRANGE_REC(0x0941, 0x0948),
  TA_UNIRANGE_REC(0x094D, 0x094D),
  TA_UNIRANGE_REC(0x0953, 0x0957),
  TA_UNIRANGE_REC(0x0962, 0x0963),
  TA_UNIRANGE_REC(0xA8E0, 0xA8F1),
  TA_UNIRANGE_REC(0xA8FF, 0xA8FF),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_dsrt_uniranges[] =
{
  TA_UNIRANGE_REC(0x10400, 0x1044F), /* Deseret */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_dsrt_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_ethi_uniranges[] =
{
  TA_UNIRANGE_REC(0x1200, 0x137F), /* Ethiopic */
  TA_UNIRANGE_REC(0x1380, 0x139F), /* Ethiopic Supplement */
  TA_UNIRANGE_REC(0x2D80, 0x2DDF), /* Ethiopic Extended */
  TA_UNIRANGE_REC(0xAB00, 0xAB2F), /* Ethiopic Extended-A */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_ethi_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x135D, 0x135F),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_geor_uniranges[] =
{
  TA_UNIRANGE_REC(0x10D0, 0x10FF), /* Georgian (Mkhedruli) */
  TA_UNIRANGE_REC(0x1C90, 0x1CBF), /* Georgian Extended (Mtavruli) */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_geor_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_geok_uniranges[] =
{
  /* Khutsuri */
  TA_UNIRANGE_REC(0x10A0, 0x10CD), /* Georgian (Asomtavruli) */
  TA_UNIRANGE_REC(0x2D00, 0x2D2D), /* Georgian Supplement (Nuskhuri) */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_geok_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_glag_uniranges[] =
{
  TA_UNIRANGE_REC( 0x2C00,  0x2C5F), /* Glagolitic */
  TA_UNIRANGE_REC(0x1E000, 0x1E02F), /* Glagolitic Supplement */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_glag_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x1E000, 0x1E02F),
  TA_UNIRANGE_REC(      0,       0)
};


const TA_Script_UniRangeRec ta_goth_uniranges[] =
{
  TA_UNIRANGE_REC(0x10330, 0x1034F), /* Gothic */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_goth_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_grek_uniranges[] =
{
  TA_UNIRANGE_REC(0x0370, 0x03FF), /* Greek and Coptic */
  TA_UNIRANGE_REC(0x1F00, 0x1FFF), /* Greek Extended */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_grek_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x037A, 0x037A),
  TA_UNIRANGE_REC(0x0384, 0x0385),
  TA_UNIRANGE_REC(0x1FBD, 0x1FC1),
  TA_UNIRANGE_REC(0x1FCD, 0x1FCF),
  TA_UNIRANGE_REC(0x1FDD, 0x1FDF),
  TA_UNIRANGE_REC(0x1FED, 0x1FEF),
  TA_UNIRANGE_REC(0x1FFD, 0x1FFE),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_gujr_uniranges[] =
{
  TA_UNIRANGE_REC(0x0A80, 0x0AFF), /* Gujarati */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_gujr_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0A81, 0x0A82),
  TA_UNIRANGE_REC(0x0ABC, 0x0ABC),
  TA_UNIRANGE_REC(0x0AC1, 0x0AC8),
  TA_UNIRANGE_REC(0x0ACD, 0x0ACD),
  TA_UNIRANGE_REC(0x0AE2, 0x0AE3),
  TA_UNIRANGE_REC(0x0AFA, 0x0AFF),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_guru_uniranges[] =
{
  TA_UNIRANGE_REC(0x0A00, 0x0A7F), /* Gurmukhi */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_guru_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0A01, 0x0A02),
  TA_UNIRANGE_REC(0x0A3C, 0x0A3C),
  TA_UNIRANGE_REC(0x0A41, 0x0A51),
  TA_UNIRANGE_REC(0x0A70, 0x0A71),
  TA_UNIRANGE_REC(0x0A75, 0x0A75),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_hebr_uniranges[] =
{
  TA_UNIRANGE_REC(0x0590, 0x05FF), /* Hebrew */
  TA_UNIRANGE_REC(0xFB1D, 0xFB4F), /* Alphab. Present. Forms (Hebrew) */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_hebr_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0591, 0x05BF),
  TA_UNIRANGE_REC(0x05C1, 0x05C2),
  TA_UNIRANGE_REC(0x05C4, 0x05C5),
  TA_UNIRANGE_REC(0x05C7, 0x05C7),
  TA_UNIRANGE_REC(0xFB1E, 0xFB1E),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_kali_uniranges[] =
{
  TA_UNIRANGE_REC(0xA900, 0xA92F), /* Kayah Li */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_kali_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0xA926, 0xA92D),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_knda_uniranges[] =
{
  TA_UNIRANGE_REC(0x0C80, 0x0CFF), /* Kannada */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_knda_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0C81, 0x0C81),
  TA_UNIRANGE_REC(0x0CBC, 0x0CBC),
  TA_UNIRANGE_REC(0x0CBF, 0x0CBF),
  TA_UNIRANGE_REC(0x0CC6, 0x0CC6),
  TA_UNIRANGE_REC(0x0CCC, 0x0CCD),
  TA_UNIRANGE_REC(0x0CE2, 0x0CE3),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_khmr_uniranges[] =
{
  TA_UNIRANGE_REC(0x1780, 0x17FF), /* Khmer */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_khmr_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x17B7, 0x17BD),
  TA_UNIRANGE_REC(0x17C6, 0x17C6),
  TA_UNIRANGE_REC(0x17C9, 0x17D3),
  TA_UNIRANGE_REC(0x17DD, 0x17DD),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_khms_uniranges[] =
{
  TA_UNIRANGE_REC(0x19E0, 0x19FF), /* Khmer Symbols */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_khms_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_lao_uniranges[] =
{
  TA_UNIRANGE_REC(0x0E80, 0x0EFF), /* Lao */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_lao_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0EB1, 0x0EB1),
  TA_UNIRANGE_REC(0x0EB4, 0x0EBC),
  TA_UNIRANGE_REC(0x0EC8, 0x0ECD),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_latn_uniranges[] =
{
  TA_UNIRANGE_REC( 0x0020,  0x007F), /* Basic Latin (no control chars) */
  TA_UNIRANGE_REC( 0x00A0,  0x00A9), /* Latin-1 Supplement (no control chars) */
  TA_UNIRANGE_REC( 0x00AB,  0x00B1), /* ... continued */
  TA_UNIRANGE_REC( 0x00B4,  0x00B8), /* ... continued */
  TA_UNIRANGE_REC( 0x00BB,  0x00FF), /* ... continued */
  TA_UNIRANGE_REC( 0x0100,  0x017F), /* Latin Extended-A */
  TA_UNIRANGE_REC( 0x0180,  0x024F), /* Latin Extended-B */
  TA_UNIRANGE_REC( 0x0250,  0x02AF), /* IPA Extensions */
  TA_UNIRANGE_REC( 0x02B9,  0x02DF), /* Spacing Modifier Letters */
  TA_UNIRANGE_REC( 0x02E5,  0x02FF), /* ... continued */
  TA_UNIRANGE_REC( 0x0300,  0x036F), /* Combining Diacritical Marks */
  TA_UNIRANGE_REC( 0x1AB0,  0x1ABE), /* Combining Diacritical Marks Extended */
  TA_UNIRANGE_REC( 0x1D00,  0x1D2B), /* Phonetic Extensions */
  TA_UNIRANGE_REC( 0x1D6B,  0x1D77), /* ... continued */
  TA_UNIRANGE_REC( 0x1D79,  0x1D7F), /* ... continued */
  TA_UNIRANGE_REC( 0x1D80,  0x1D9A), /* Phonetic Extensions Supplement */
  TA_UNIRANGE_REC( 0x1DC0,  0x1DFF), /* Combining Diacritical Marks Supplement */
  TA_UNIRANGE_REC( 0x1E00,  0x1EFF), /* Latin Extended Additional */
  TA_UNIRANGE_REC( 0x2000,  0x206F), /* General Punctuation */
  TA_UNIRANGE_REC( 0x20A0,  0x20B8), /* Currency Symbols ... */
  TA_UNIRANGE_REC( 0x20BA,  0x20CF), /* ... except new Rupee sign */
  TA_UNIRANGE_REC( 0x2150,  0x218F), /* Number Forms */
  TA_UNIRANGE_REC( 0x2C60,  0x2C7B), /* Latin Extended-C */
  TA_UNIRANGE_REC( 0x2C7E,  0x2C7F), /* ... continued */
  TA_UNIRANGE_REC( 0x2E00,  0x2E7F), /* Supplemental Punctuation */
  TA_UNIRANGE_REC( 0xA720,  0xA76F), /* Latin Extended-D */
  TA_UNIRANGE_REC( 0xA771,  0xA7F7), /* ... continued */
  TA_UNIRANGE_REC( 0xA7FA,  0xA7FF), /* ... continued */
  TA_UNIRANGE_REC( 0xAB30,  0xAB5B), /* Latin Extended-E */
  TA_UNIRANGE_REC( 0xAB60,  0xAB6F), /* ... continued */
  TA_UNIRANGE_REC( 0xFB00,  0xFB06), /* Alphab. Present. Forms (Latin Ligs) */
  TA_UNIRANGE_REC(0x1D400, 0x1D7FF), /* Mathematical Alphanumeric Symbols */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_latn_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x005E, 0x0060),
  TA_UNIRANGE_REC(0x007E, 0x007E),
  TA_UNIRANGE_REC(0x00A8, 0x00A9),
  TA_UNIRANGE_REC(0x00AE, 0x00B0),
  TA_UNIRANGE_REC(0x00B4, 0x00B4),
  TA_UNIRANGE_REC(0x00B8, 0x00B8),
  TA_UNIRANGE_REC(0x00BC, 0x00BE),
  TA_UNIRANGE_REC(0x02B9, 0x02DF),
  TA_UNIRANGE_REC(0x02E5, 0x02FF),
  TA_UNIRANGE_REC(0x0300, 0x036F),
  TA_UNIRANGE_REC(0x1AB0, 0x1ABE),
  TA_UNIRANGE_REC(0x1DC0, 0x1DFF),
  TA_UNIRANGE_REC(0x2017, 0x2017),
  TA_UNIRANGE_REC(0x203E, 0x203E),
  TA_UNIRANGE_REC(0xA788, 0xA788),
  TA_UNIRANGE_REC(0xA7F8, 0xA7FA),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_latb_uniranges[] =
{
  TA_UNIRANGE_REC(0x1D62, 0x1D6A), /* some small subscript letters */
  TA_UNIRANGE_REC(0x2080, 0x209C), /* subscript digits and letters */
  TA_UNIRANGE_REC(0x2C7C, 0x2C7C), /* latin subscript small letter j */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_latb_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_latp_uniranges[] =
{
  TA_UNIRANGE_REC(0x00AA, 0x00AA), /* feminine ordinal indicator */
  TA_UNIRANGE_REC(0x00B2, 0x00B3), /* superscript two and three */
  TA_UNIRANGE_REC(0x00B9, 0x00BA), /* superscript one, masc. ord. indic. */
  TA_UNIRANGE_REC(0x02B0, 0x02B8), /* some latin superscript mod. letters */
  TA_UNIRANGE_REC(0x02E0, 0x02E4), /* some IPA modifier letters */
  TA_UNIRANGE_REC(0x1D2C, 0x1D61), /* latin superscript modifier letters */
  TA_UNIRANGE_REC(0x1D78, 0x1D78), /* modifier letter cyrillic en */
  TA_UNIRANGE_REC(0x1D9B, 0x1DBF), /* more modifier letters */
  TA_UNIRANGE_REC(0x2070, 0x207F), /* superscript digits and letters */
  TA_UNIRANGE_REC(0x2C7D, 0x2C7D), /* modifier letter capital v */
  TA_UNIRANGE_REC(0xA770, 0xA770), /* modifier letter us */
  TA_UNIRANGE_REC(0xA7F8, 0xA7F9), /* more modifier letters */
  TA_UNIRANGE_REC(0xAB5C, 0xAB5F), /* more modifier letters */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_latp_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_lisu_uniranges[] =
{
  TA_UNIRANGE_REC(0xA4D0, 0xA4FF), /* Lisu */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_lisu_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_mlym_uniranges[] =
{
  TA_UNIRANGE_REC(0x0D00, 0x0D7F), /* Malayalam */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_mlym_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0D00, 0x0D01),
  TA_UNIRANGE_REC(0x0D3B, 0x0D3C),
  TA_UNIRANGE_REC(0x0D4D, 0x0D4E),
  TA_UNIRANGE_REC(0x0D62, 0x0D63),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_mong_uniranges[] =
{
  TA_UNIRANGE_REC( 0x1800,  0x18AF), /* Mongolian */
  TA_UNIRANGE_REC(0x11660, 0x1167F), /* Mongolian Supplement */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_mong_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x1885, 0x1886),
  TA_UNIRANGE_REC(0x18A9, 0x18A9),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_mymr_uniranges[] =
{
  TA_UNIRANGE_REC(0x1000, 0x109F), /* Myanmar */
  TA_UNIRANGE_REC(0xA9E0, 0xA9FF), /* Myanmar Extended-B */
  TA_UNIRANGE_REC(0xAA60, 0xAA7F), /* Myanmar Extended-A */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_mymr_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x102D, 0x1030),
  TA_UNIRANGE_REC(0x1032, 0x1037),
  TA_UNIRANGE_REC(0x103A, 0x103A),
  TA_UNIRANGE_REC(0x103D, 0x103E),
  TA_UNIRANGE_REC(0x1058, 0x1059),
  TA_UNIRANGE_REC(0x105E, 0x1060),
  TA_UNIRANGE_REC(0x1071, 0x1074),
  TA_UNIRANGE_REC(0x1082, 0x1082),
  TA_UNIRANGE_REC(0x1085, 0x1086),
  TA_UNIRANGE_REC(0x108D, 0x108D),
  TA_UNIRANGE_REC(0xA9E5, 0xA9E5),
  TA_UNIRANGE_REC(0xAA7C, 0xAA7C),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_nkoo_uniranges[] =
{
  TA_UNIRANGE_REC(0x07C0, 0x07FF), /* N'Ko */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_nkoo_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x07EB, 0x07F5),
  TA_UNIRANGE_REC(0x07FD, 0x07FD),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_olck_uniranges[] =
{
  TA_UNIRANGE_REC(0x1C50, 0x1C7F), /* Ol Chiki */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_olck_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_orkh_uniranges[] =
{
  TA_UNIRANGE_REC(0x10C00, 0x10C4F), /* Old Turkic */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_orkh_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_osge_uniranges[] =
{
  TA_UNIRANGE_REC(0x104B0, 0x104FF), /* Osage */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_osge_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_osma_uniranges[] =
{
  TA_UNIRANGE_REC(0x10480, 0x104AF), /* Osmanya */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_osma_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_saur_uniranges[] =
{
  TA_UNIRANGE_REC(0xA880, 0xA8DF), /* Saurashtra */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_saur_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0xA880, 0xA881),
  TA_UNIRANGE_REC(0xA8B4, 0xA8C5),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_shaw_uniranges[] =
{
  TA_UNIRANGE_REC(0x10450, 0x1047F), /* Shavian */
  TA_UNIRANGE_REC(      0,       0)
};

const TA_Script_UniRangeRec ta_shaw_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_sinh_uniranges[] =
{
  TA_UNIRANGE_REC(0x0D80, 0x0DFF), /* Sinhala */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_sinh_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0DCA, 0x0DCA),
  TA_UNIRANGE_REC(0x0DD2, 0x0DD6),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_sund_uniranges[] =
{
  TA_UNIRANGE_REC(0x1B80, 0x1BBF), /* Sundanese */
  TA_UNIRANGE_REC(0x1CC0, 0x1CCF), /* Sundanese Supplement */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_sund_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x1B80, 0x1B82),
  TA_UNIRANGE_REC(0x1BA1, 0x1BAD),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_taml_uniranges[] =
{
  TA_UNIRANGE_REC(0x0B80, 0x0BFF), /* Tamil */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_taml_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0B82, 0x0B82),
  TA_UNIRANGE_REC(0x0BC0, 0x0BC2),
  TA_UNIRANGE_REC(0x0BCD, 0x0BCD),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_tavt_uniranges[] =
{
  TA_UNIRANGE_REC(0xAA80, 0xAADF), /* Tai Viet */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_tavt_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0xAAB0, 0xAAB0),
  TA_UNIRANGE_REC(0xAAB2, 0xAAB4),
  TA_UNIRANGE_REC(0xAAB7, 0xAAB8),
  TA_UNIRANGE_REC(0xAABE, 0xAABF),
  TA_UNIRANGE_REC(0xAAC1, 0xAAC1),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_telu_uniranges[] =
{
  TA_UNIRANGE_REC(0x0C00, 0x0C7F), /* Telugu */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_telu_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0C00, 0x0C00),
  TA_UNIRANGE_REC(0x0C04, 0x0C04),
  TA_UNIRANGE_REC(0x0C3E, 0x0C40),
  TA_UNIRANGE_REC(0x0C46, 0x0C56),
  TA_UNIRANGE_REC(0x0C62, 0x0C63),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_thai_uniranges[] =
{
  TA_UNIRANGE_REC(0x0E00, 0x0E7F), /* Thai */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_thai_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0x0E31, 0x0E31),
  TA_UNIRANGE_REC(0x0E34, 0x0E3A),
  TA_UNIRANGE_REC(0x0E47, 0x0E4E),
  TA_UNIRANGE_REC(     0,      0)
};


const TA_Script_UniRangeRec ta_tfng_uniranges[] =
{
  TA_UNIRANGE_REC(0x2D30, 0x2D7F), /* Tifinagh */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_tfng_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_vaii_uniranges[] =
{
  TA_UNIRANGE_REC(0xA500, 0xA63F), /* Vai */
  TA_UNIRANGE_REC(     0,      0)
};

const TA_Script_UniRangeRec ta_vaii_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};


const TA_Script_UniRangeRec ta_none_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};

const TA_Script_UniRangeRec ta_none_nonbase_uniranges[] =
{
  TA_UNIRANGE_REC(0, 0)
};

/* end of taranges.c */
