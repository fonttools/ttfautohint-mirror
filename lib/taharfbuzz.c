/* taharfbuzz.c */

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


/* originally file `hbshim.c' (2014-Jan-11) from FreeType */

/* heavily modified 2014 by Werner Lemberg <wl@gnu.org> */

#include "taharfbuzz.h"


/*
 * We use `sets' (in the HarfBuzz sense, which comes quite near to the
 * usual mathematical meaning) to manage both lookups and glyph indices.
 *
 * 1. For each coverage, collect lookup IDs in a set.  Note that an
 *    auto-hinter `coverage' is represented by one `feature', and a
 *    feature consists of an arbitrary number of (font specific) `lookup's
 *    that actually do the mapping job.  Please check the OpenType
 *    specification for more details on features and lookups.
 *
 * 2. Create glyph ID sets from the corresponding lookup sets.
 *
 * 3. The glyph set corresponding to TA_COVERAGE_DEFAULT is computed
 *    with all lookups specific to the OpenType script activated.  It
 *    relies on the order of ta_style_classes entries so that
 *    special coverages (like `oldstyle figures') don't get overwritten.
 */


/* load coverage tags */
#undef COVERAGE
#define COVERAGE(name, NAME, description, \
                 tag1, tag2, tag3, tag4) \
          static const hb_tag_t name ## _coverage[] = \
          { \
            HB_TAG(tag1, tag2, tag3, tag4), \
            HB_TAG_NONE \
          };

#include "tacover.h"


/* define mapping between coverage tags and TA_Coverage */
#undef COVERAGE
#define COVERAGE(name, NAME, description, \
                 tag1, tag2, tag3, tag4) \
          name ## _coverage,

static const hb_tag_t* coverages[] =
{
#include "tacover.h"

  NULL /* TA_COVERAGE_DEFAULT */
};


/* load HarfBuzz script tags */
#undef SCRIPT
#define SCRIPT(s, S, d, h, sc1, sc2, sc3) h,

static const hb_script_t scripts[] =
{
#include "ttfautohint-scripts.h"
};


FT_Error
ta_get_coverage(TA_FaceGlobals globals,
                TA_StyleClass style_class,
                FT_Byte* gstyles)
{
  hb_face_t* face;

  hb_set_t* gsub_lookups; /* GSUB lookups for a given script */
  hb_set_t* gsub_glyphs_in; /* glyphs covered by GSUB lookups */
  hb_set_t* gsub_glyphs_out;

  hb_set_t* gpos_lookups; /* GPOS lookups for a given script */
  hb_set_t* gpos_glyphs; /* glyphs covered by GPOS lookups */

  hb_script_t script;
  const hb_tag_t*  coverage_tags;
  hb_tag_t script_tags[] = { HB_TAG_NONE,
                             HB_TAG_NONE,
                             HB_TAG_NONE,
                             HB_TAG_NONE };

  hb_codepoint_t idx;
#ifdef TA_DEBUG
  int count;
#endif


  if (!globals || !style_class || !gstyles)
    return FT_Err_Invalid_Argument;

  face = hb_font_get_face(globals->hb_font);

  gsub_lookups = hb_set_create();
  gsub_glyphs_in = hb_set_create();
  gsub_glyphs_out = hb_set_create();
  gpos_lookups = hb_set_create();
  gpos_glyphs = hb_set_create();

  coverage_tags = coverages[style_class->coverage];
  script = scripts[style_class->script];

  /* convert a HarfBuzz script tag into the corresponding OpenType */
  /* tag or tags -- some Indic scripts like Devanagari have an old */
  /* and a new set of features */
  hb_ot_tags_from_script(script,
                         &script_tags[0],
                         &script_tags[1]);

  /* `hb_ot_tags_from_script' usually returns HB_OT_TAG_DEFAULT_SCRIPT */
  /* as the second tag; we change that to HB_TAG_NONE except for the */
  /* default script */
  if (style_class->script == globals->font->default_script
      && style_class->coverage == TA_COVERAGE_DEFAULT)
  {
    if (script_tags[0] == HB_TAG_NONE)
      script_tags[0] = HB_OT_TAG_DEFAULT_SCRIPT;
    else
    {
      if (script_tags[1] == HB_TAG_NONE)
        script_tags[1] = HB_OT_TAG_DEFAULT_SCRIPT;
      else if (script_tags[1] != HB_OT_TAG_DEFAULT_SCRIPT)
        script_tags[2] = HB_OT_TAG_DEFAULT_SCRIPT;
    }
  }
  else
  {
    if (script_tags[1] == HB_OT_TAG_DEFAULT_SCRIPT)
      script_tags[1] = HB_TAG_NONE;
  }

  hb_ot_layout_collect_lookups(face,
                               HB_OT_TAG_GSUB,
                               script_tags,
                               NULL,
                               coverage_tags,
                               gsub_lookups);

  if (hb_set_is_empty(gsub_lookups))
    goto Exit; /* nothing to do */

  hb_ot_layout_collect_lookups(face,
                               HB_OT_TAG_GPOS,
                               script_tags,
                               NULL,
                               coverage_tags,
                               gpos_lookups);

  TA_LOG_GLOBAL(("GSUB lookups (style `%s'):\n"
                 " ",
                 ta_style_names[style_class->style]));

#ifdef TA_DEBUG
  count = 0;
#endif

  for (idx = -1; hb_set_next(gsub_lookups, &idx);)
  {
#ifdef TA_DEBUG
    TA_LOG_GLOBAL((" %d", idx));
    count++;
#endif

    /* get output coverage of GSUB feature */
    hb_ot_layout_lookup_collect_glyphs(face,
                                       HB_OT_TAG_GSUB,
                                       idx,
                                       NULL,
                                       gsub_glyphs_in,
                                       NULL,
                                       gsub_glyphs_out);
  }

#ifdef TA_DEBUG
  if (!count)
    TA_LOG_GLOBAL((" (none)"));
  TA_LOG_GLOBAL(("\n\n"));
#endif

  TA_LOG_GLOBAL(("GPOS lookups (style `%s'):\n"
                 " ",
                 ta_style_names[style_class->style]));

#ifdef TA_DEBUG
  count = 0;
#endif

  for (idx = -1; hb_set_next(gpos_lookups, &idx);)
  {
#ifdef TA_DEBUG
    TA_LOG_GLOBAL((" %d", idx));
    count++;
#endif

    /* get input coverage of GPOS feature */
    hb_ot_layout_lookup_collect_glyphs(face,
                                       HB_OT_TAG_GPOS,
                                       idx,
                                       NULL,
                                       gpos_glyphs,
                                       NULL,
                                       NULL);
  }

#ifdef TA_DEBUG
  if (!count)
    TA_LOG_GLOBAL((" (none)"));
  TA_LOG_GLOBAL(("\n\n"));
#endif

  /*
   * we now check whether we can construct blue zones, using glyphs
   * covered by the feature only -- in case there is not a single zone
   * (this is, not a single character is covered), we skip this coverage
   */
  if (style_class->coverage != TA_COVERAGE_DEFAULT)
  {
    TA_Blue_Stringset bss = style_class->blue_stringset;
    const TA_Blue_StringRec* bs = &ta_blue_stringsets[bss];

    FT_Bool found = 0;


    for (; bs->string != TA_BLUE_STRING_MAX; bs++)
    {
      const char* p = &ta_blue_strings[bs->string];


      while (*p)
      {
        hb_codepoint_t ch;


        GET_UTF8_CHAR(ch, p);

        for (idx = -1; hb_set_next(gsub_lookups, &idx);)
        {
          hb_codepoint_t gidx = FT_Get_Char_Index(globals->face, ch);


          if (hb_ot_layout_lookup_would_substitute(face, idx,
                                                   &gidx, 1, 1))
          {
            found = 1;
            break;
          }
        }
      }
    }

    if (!found)
    {
      TA_LOG_GLOBAL(("  no blue characters found; style skipped\n"));
      goto Exit;
    }
  }

  /* merge in and out glyphs */
  hb_set_union(gsub_glyphs_out, gsub_glyphs_in);

  /*
   * Various OpenType features might use the same glyphs at different
   * vertical positions; for example, superscript and subscript glyphs
   * could be the same.  However, the auto-hinter is completely
   * agnostic of OpenType features after the feature analysis has been
   * completed: The engine then simply receives a glyph index and returns a
   * hinted and usually rendered glyph.
   *
   * Consider the superscript feature of font `pala.ttf': Some of the
   * glyphs are `real', this is, they have a zero vertical offset, but
   * most of them are small caps glyphs shifted up to the superscript
   * position (this is, the `sups' feature is present in both the GSUB and
   * GPOS tables).  The code for blue zones computation actually uses a
   * feature's y offset so that the `real' glyphs get correct hints.  But
   * later on it is impossible to decide whether a glyph index belongs to,
   * say, the small caps or superscript feature.
   *
   * For this reason, we don't assign a style to a glyph if the current
   * feature covers the glyph in both the GSUB and the GPOS tables.  This
   * is quite a broad condition, assuming that
   *
   *   (a) glyphs that get used in multiple features are present in a
   *       feature without vertical shift,
   *
   * and
   *
   *   (b) a feature's GPOS data really moves the glyph vertically.
   *
   * Not fulfilling condition (a) makes a font larger; it would also
   * reduce the number of glyphs that could be addressed directly without
   * using OpenType features, so this assumption is rather strong.
   *
   * Condition (b) is much weaker, and there might be glyphs which get
   * missed.  However, the OpenType features we are going to handle are
   * primarily located in GSUB, and HarfBuzz doesn't provide an API to
   * directly get the necessary information from the GPOS table.  A
   * possible solution might be to directly parse the GPOS table to find
   * out whether a glyph gets shifted vertically, but this is something I
   * would like to avoid if not really necessary.
   *
   * Note that we don't follow this logic for the default coverage. 
   * Complex scripts like Devanagari have mandatory GPOS features to
   * position many glyph elements, using mark-to-base or mark-to-ligature
   * tables; the number of glyphs missed due to condition (b) would be far
   * too large.
   */
  if (style_class->coverage != TA_COVERAGE_DEFAULT)
    hb_set_subtract(gsub_glyphs_out, gpos_glyphs);

#ifdef TA_DEBUG
  TA_LOG_GLOBAL(("  glyphs without GPOS data (`*' means already assigned)"));
  count = 0;
#endif

  for (idx = -1; hb_set_next(gsub_glyphs_out, &idx);)
  {
#ifdef TA_DEBUG
    if (!(count % 10))
      TA_LOG_GLOBAL(("\n"
                     "   "));

    TA_LOG_GLOBAL((" %d", idx));
    count++;
#endif

    /* HarfBuzz 0.9.26 and older doesn't validate glyph indices */
    /* returned by `hb_ot_layout_lookup_collect_glyphs'...      */
    if (idx >= (hb_codepoint_t)globals->glyph_count)
      continue;

    if (gstyles[idx] == TA_STYLE_UNASSIGNED)
      gstyles[idx] = (FT_Byte)style_class->style;
#ifdef TA_DEBUG
    else
      TA_LOG_GLOBAL(("*"));
#endif
  }

#ifdef TA_DEBUG
    if (!count)
      TA_LOG_GLOBAL(("\n"
                     "    (none)"));
    TA_LOG_GLOBAL(("\n\n"));
#endif

Exit:
  hb_set_destroy(gsub_lookups);
  hb_set_destroy(gsub_glyphs_in);
  hb_set_destroy(gsub_glyphs_out);
  hb_set_destroy(gpos_lookups);
  hb_set_destroy(gpos_glyphs);

  return FT_Err_Ok;
}


/* construct HarfBuzz features */
#undef COVERAGE
#define COVERAGE(name, NAME, description, \
                 tag1, tag2, tag3, tag4) \
          static const hb_feature_t name ## _feature[] = \
          { \
            { \
              HB_TAG(tag1, tag2, tag3, tag4), \
              1, 0, (unsigned int)-1 \
            } \
          };

#include "tacover.h"


/* define mapping between HarfBuzz features and TA_Coverage */
#undef COVERAGE
#define COVERAGE(name, NAME, description, \
                 tag1, tag2, tag3, tag4) \
          name ## _feature,

static const hb_feature_t* features[] =
{
#include "tacover.h"

  NULL /* TA_COVERAGE_DEFAULT */
};


FT_Error
ta_get_char_index(TA_StyleMetrics metrics,
                  FT_ULong charcode,
                  FT_ULong *codepoint,
                  FT_Long  *y_offset)
{
  TA_StyleClass style_class;

  const hb_feature_t* feature;

  FT_ULong in_idx, out_idx;


  if (!metrics)
    return FT_Err_Invalid_Argument;

  in_idx = FT_Get_Char_Index(metrics->globals->face, charcode);

  style_class = metrics->style_class;

  feature = features[style_class->coverage];

  if (feature)
  {
    FT_UInt upem = metrics->globals->face->units_per_EM;

    hb_font_t* font = metrics->globals->hb_font;
    hb_buffer_t* buf = hb_buffer_create();

    uint32_t c = (uint32_t)charcode;

    hb_glyph_info_t* ginfo;
    hb_glyph_position_t* gpos;
    unsigned int gcount;


    /* we shape at a size of units per EM; this means font units */
    hb_font_set_scale(font, upem, upem);

    /* XXX: is this sufficient for a single character of any script? */
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, scripts[style_class->script]);

    /* we add one character to `buf' ... */
    hb_buffer_add_utf32(buf, &c, 1, 0, 1);

    /* ... and apply one feature */
    hb_shape(font, buf, feature, 1);

    ginfo = hb_buffer_get_glyph_infos(buf, &gcount);
    gpos = hb_buffer_get_glyph_positions(buf, &gcount);

    out_idx = ginfo[0].codepoint;

    /* getting the same index indicates no substitution, */
    /* which means that the glyph isn't available in the feature */
    if (in_idx == out_idx)
    {
      *codepoint = 0;
      *y_offset = 0;
    }
    else
    {
      *codepoint = out_idx;
      *y_offset = gpos[0].y_offset;
    }

    hb_buffer_destroy(buf);

#ifdef TA_DEBUG
    if (gcount > 1)
      TA_LOG(("ta_get_char_index:"
              " input character mapped to multiple glyphs\n"));
#endif
  }
  else
  {
    *codepoint = in_idx;
    *y_offset = 0;
  }

  return FT_Err_Ok;
}

/* end of taharfbuzz.c */
