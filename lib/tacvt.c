/* tacvt.c */

/*
 * Copyright (C) 2011-2013 by Werner Lemberg.
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


static FT_Error
TA_sfnt_compute_global_hints(SFNT* sfnt,
                             FONT* font,
                             FT_UInt script_idx)
{
  FT_Error error;
  FT_Face face = sfnt->face;
  FT_UInt idx;
  FT_Int32 load_flags;


  error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
  if (error)
  {
    if (font->symbol)
    {
      error = FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL);
      if (error)
        return TA_Err_Missing_Symbol_CMap;
    }
    else
      return TA_Err_Missing_Unicode_CMap;
  }

  if (font->symbol)
    idx = 0;
  else
  {
    /* load standard character to trigger script initializations */
    /* XXX make this configurable to use a different letter */
    idx = FT_Get_Char_Index(face,
                            ta_script_classes[script_idx]->standard_char);
    if (!idx)
      return TA_Err_Missing_Glyph;
  }

  load_flags = 1 << 29; /* vertical hinting only */
  error = ta_loader_load_glyph(font, face, idx, load_flags);

  return error;
}


static FT_Error
TA_table_build_cvt(FT_Byte** cvt,
                   FT_ULong* cvt_len,
                   SFNT* sfnt,
                   FONT* font)
{
  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;

  TA_LatinAxis haxis;
  TA_LatinAxis vaxis;

  FT_UInt hwidth_count;
  FT_UInt vwidth_count;
  FT_UInt blue_count;

  FT_UInt i, j, i_max;
  FT_UInt buf_len;
  FT_UInt len;
  FT_Byte* buf;
  FT_Byte* bufp;
  FT_UInt cvt_offset;

  FT_Error error;


  /* checking multiple scripts doesn't make sense for symbol fonts */
  i_max = font->symbol ? 1 : TA_SCRIPT_MAX;

  /* loop over all scripts and collect the relevant CVT data */
  /* to compute the necessary array sizes and meta-information */
  hwidth_count = 0;
  vwidth_count = 0;
  blue_count = 0;

  data->num_used_scripts = 0;

  for (i = 0; i < i_max; i++)
  {
    error = TA_sfnt_compute_global_hints(sfnt, font, i);
    if (error == TA_Err_Missing_Glyph)
    {
      data->script_ids[i] = 0xFFFFU;
      continue;
    }
    if (error)
      return error;

    data->script_ids[i] = data->num_used_scripts++;

    if (font->loader->hints.metrics->script_class->script == TA_SCRIPT_DFLT)
      continue;
    else
    {
      /* XXX: generalize this to handle other metrics also */
      haxis = &((TA_LatinMetrics)font->loader->hints.metrics)->axis[0];
      vaxis = &((TA_LatinMetrics)font->loader->hints.metrics)->axis[1];

      hwidth_count += haxis->width_count;
      vwidth_count += vaxis->width_count;
      /* there are two artificial blue zones at the end of the array */
      /* that are not part of `vaxis->blue_count' */
      blue_count += vaxis->blue_count + 2;
    }
  }

  /* exit if the font doesn't contain a single supported script */
  if (!data->num_used_scripts)
    return TA_Err_Missing_Glyph;

  buf_len = cvtl_max_runtime /* runtime values 1 */
            + data->num_used_scripts /* runtime values 2 (for scaling) */
            + 2 * data->num_used_scripts /* runtime values 3 (blue data) */
            + 2 * data->num_used_scripts /* vert. and horiz. std. widths */
            + hwidth_count
            + vwidth_count
            + 2 * blue_count; /* round and flat blue zones */
  buf_len <<= 1; /* we have 16bit values */

  /* buffer length must be a multiple of four */
  len = (buf_len + 3) & ~3;
  buf = (FT_Byte*)malloc(len);
  if (!buf)
    return FT_Err_Out_Of_Memory;

  /* pad end of buffer with zeros */
  buf[len - 1] = 0x00;
  buf[len - 2] = 0x00;
  buf[len - 3] = 0x00;

  bufp = buf;

  /*
   * some CVT values are initialized (and modified) at runtime:
   *
   *   (1) the `cvtl_xxx' values (see `tabytecode.h')
   *   (2) a scaling value for each script
   *   (3) offset and size of the vertical widths array
   *       (needed by `bci_{smooth,strong}_stem_width') for each script
   */
  for (i = 0; i < (cvtl_max_runtime
                   + data->num_used_scripts
                   + 2 * data->num_used_scripts) * 2; i++)
    *(bufp++) = 0;

  cvt_offset = bufp - buf;

  /* loop again over all scripts and copy CVT data */
  for (i = 0; i < i_max; i++)
  {
    /* collect offsets */
    data->cvt_offsets[i] = ((FT_UInt)(bufp - buf) - cvt_offset) >> 1;

    error = TA_sfnt_compute_global_hints(sfnt, font, i);
    if (error == TA_Err_Missing_Glyph)
      continue;
    if (error)
      return error;

    if (font->loader->hints.metrics->script_class->script == TA_SCRIPT_DFLT)
    {
      haxis = NULL;
      vaxis = NULL;

      hwidth_count = 0;
      vwidth_count = 0;
      blue_count = 0;
    }
    else
    {
      haxis = &((TA_LatinMetrics)font->loader->hints.metrics)->axis[0];
      vaxis = &((TA_LatinMetrics)font->loader->hints.metrics)->axis[1];

      hwidth_count = haxis->width_count;
      vwidth_count = vaxis->width_count;
      blue_count = vaxis->blue_count + 2; /* with artificial blue zones */
    }

    /* horizontal standard width */
    if (hwidth_count > 0)
    {
      *(bufp++) = HIGH(haxis->widths[0].org);
      *(bufp++) = LOW(haxis->widths[0].org);
    }
    else
    {
      *(bufp++) = 0;
      *(bufp++) = 50;
    }

    for (j = 0; j < hwidth_count; j++)
    {
      if (haxis->widths[j].org > 0xFFFF)
        goto Err;
      *(bufp++) = HIGH(haxis->widths[j].org);
      *(bufp++) = LOW(haxis->widths[j].org);
    }

    /* vertical standard width */
    if (vwidth_count > 0)
    {
      *(bufp++) = HIGH(vaxis->widths[0].org);
      *(bufp++) = LOW(vaxis->widths[0].org);
    }
    else
    {
      *(bufp++) = 0;
      *(bufp++) = 50;
    }

    for (j = 0; j < vwidth_count; j++)
    {
      if (vaxis->widths[j].org > 0xFFFF)
        goto Err;
      *(bufp++) = HIGH(vaxis->widths[j].org);
      *(bufp++) = LOW(vaxis->widths[j].org);
    }

    data->cvt_blue_adjustment_offsets[i] = 0xFFFFU;

    for (j = 0; j < blue_count; j++)
    {
      if (vaxis->blues[j].ref.org > 0xFFFF)
        goto Err;
      *(bufp++) = HIGH(vaxis->blues[j].ref.org);
      *(bufp++) = LOW(vaxis->blues[j].ref.org);
    }

    for (j = 0; j < blue_count; j++)
    {
      if (vaxis->blues[j].shoot.org > 0xFFFF)
        goto Err;
      *(bufp++) = HIGH(vaxis->blues[j].shoot.org);
      *(bufp++) = LOW(vaxis->blues[j].shoot.org);

      if (vaxis->blues[j].flags & TA_LATIN_BLUE_ADJUSTMENT)
        data->cvt_blue_adjustment_offsets[i] = j;
    }

    data->cvt_horz_width_sizes[i] = hwidth_count;
    data->cvt_vert_width_sizes[i] = vwidth_count;
    data->cvt_blue_zone_sizes[i] = blue_count;
  }

  *cvt = buf;
  *cvt_len = buf_len;

  return FT_Err_Ok;

Err:
  free(buf);
  return TA_Err_Hinter_Overflow;
}


FT_Error
TA_sfnt_build_cvt_table(SFNT* sfnt,
                        FONT* font)
{
  FT_Error error;

  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;

  FT_Byte* cvt_buf;
  FT_ULong cvt_len;


  error = TA_sfnt_add_table_info(sfnt);
  if (error)
    goto Exit;

  /* `glyf', `cvt', `fpgm', and `prep' are always used in parallel */
  if (glyf_table->processed)
  {
    sfnt->table_infos[sfnt->num_table_infos - 1] = data->cvt_idx;
    goto Exit;
  }

  error = TA_table_build_cvt(&cvt_buf, &cvt_len, sfnt, font);
  if (error)
    goto Exit;

  /* in case of success, `cvt_buf' gets linked */
  /* and is eventually freed in `TA_font_unload' */
  error = TA_font_add_table(font,
                            &sfnt->table_infos[sfnt->num_table_infos - 1],
                            TTAG_cvt, cvt_len, cvt_buf);
  if (error)
    free(cvt_buf);
  else
    data->cvt_idx = sfnt->table_infos[sfnt->num_table_infos - 1];

Exit:
  return error;
}

/* end of tacvt.c */
