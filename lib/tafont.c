/* tafont.c */

/*
 * Copyright (C) 2011-2019 by Werner Lemberg.
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


FT_Error
TA_font_init(FONT* font)
{
  FT_Error error;
  FT_Face f;
  FT_Int major, minor, patch;


  error = FT_Init_FreeType(&font->lib);
  if (error)
    return error;

  /* assure correct FreeType version to avoid using the wrong DLL */
  FT_Library_Version(font->lib, &major, &minor, &patch);
  if (((major*1000 + minor)*1000 + patch) < 2004005)
    return TA_Err_Invalid_FreeType_Version;

  /* get number of faces (i.e. subfonts) */
  error = FT_New_Memory_Face(font->lib,
                             font->in_buf,
                             (FT_Long)font->in_len,
                             -1,
                             &f);
  if (error)
    return error;
  font->num_sfnts = f->num_faces;
  FT_Done_Face(f);

  /* it is a TTC if we have more than a single subfont */
  font->sfnts = (SFNT*)calloc(1, (size_t)font->num_sfnts * sizeof (SFNT));
  if (!font->sfnts)
    return FT_Err_Out_Of_Memory;

  if (font->reference_buf)
  {
    error = FT_New_Memory_Face(font->lib,
                               font->reference_buf,
                               (FT_Long)font->reference_len,
                               font->reference_index,
                               &font->reference);
    if (error)
      return error + 0x300;
  }

  return TA_Err_Ok;
}


void
TA_font_unload(FONT* font,
               const char* in_buf,
               char** out_bufp,
               const char* control_buf,
               const char* reference_buf)
{
  /* in case of error it is expected that unallocated pointers */
  /* are NULL (and counters are zero) */

  if (!font)
    return;

  ta_loader_done(font);

  if (font->tables)
  {
    FT_ULong i;


    for (i = 0; i < font->num_tables; i++)
    {
      free(font->tables[i].buf);
      if (font->tables[i].data)
      {
        if (font->tables[i].tag == TTAG_glyf)
        {
          glyf_Data* data = (glyf_Data*)font->tables[i].data;
          FT_UShort j;


          for (j = 0; j < data->num_glyphs; j++)
          {
            free(data->glyphs[j].buf);
            free(data->glyphs[j].ins_buf);
            free(data->glyphs[j].ins_extra_buf);
            free(data->glyphs[j].components);
            free(data->glyphs[j].pointsums);
          }
          free(data->glyphs);
          free(data);
        }
      }
    }
    free(font->tables);
  }

  if (font->sfnts)
  {
    FT_Long i;


    for (i = 0; i < font->num_sfnts; i++)
    {
      FT_Done_Face(font->sfnts[i].face);
      free(font->sfnts[i].table_infos);
    }
    free(font->sfnts);
  }

  FT_Done_Face(font->reference);

  number_set_free(font->x_height_snapping_exceptions);

  FT_Done_FreeType(font->lib);

  /* in case the user provided file handles, */
  /* free the allocated buffers for the file contents */
  if (!in_buf)
    free(font->in_buf);
  if (!out_bufp)
    font->deallocate(font->out_buf);
  if (!control_buf)
    free(font->control_buf);
  if (!reference_buf)
    free(font->reference_buf);

  free(font);
}

/* end of tafont.c */
