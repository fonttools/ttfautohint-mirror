/* ttfautohint.c */

/*
 * Copyright (C) 2011 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* This file needs FreeType 2.4.5 or newer. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ta.h"


/* If `do_complete' is 0, only return `header_len'. */

static FT_Error
TA_sfnt_build_TTF_header(SFNT* sfnt,
                         FONT* font,
                         FT_Byte** header_buf,
                         FT_ULong* header_len,
                         FT_Int do_complete)
{
  SFNT_Table* tables = font->tables;

  SFNT_Table_Info* table_infos = sfnt->table_infos;
  FT_ULong num_table_infos = sfnt->num_table_infos;

  FT_Byte* buf;
  FT_ULong len;

  FT_Byte* table_record;

  FT_Byte* head_buf = NULL; /* pointer to `head' table */
  FT_ULong head_checksum; /* checksum in `head' table */

  FT_ULong num_tables_in_header;
  FT_ULong i;


  num_tables_in_header = 0;
  for (i = 0; i < num_table_infos; i++)
  {
    /* ignore empty tables */
    if (table_infos[i] != MISSING)
      num_tables_in_header++;
  }

  len = 12 + 16 * num_tables_in_header;
  if (!do_complete)
  {
    *header_len = len;
    return TA_Err_Ok;
  }
  buf = (FT_Byte*)malloc(len);
  if (!buf)
    return FT_Err_Out_Of_Memory;

  /* SFNT version */
  buf[0] = 0x00;
  buf[1] = 0x01;
  buf[2] = 0x00;
  buf[3] = 0x00;

  /* number of tables */
  buf[4] = HIGH(num_tables_in_header);
  buf[5] = LOW(num_tables_in_header);

  /* auxiliary data */
  {
    FT_ULong search_range, entry_selector, range_shift;
    FT_ULong i, j;


    for (i = 1, j = 2; j <= num_tables_in_header; i++, j <<= 1)
      ;

    entry_selector = i - 1;
    search_range = 0x10 << entry_selector;
    range_shift = (num_tables_in_header << 4) - search_range;

    buf[6] = HIGH(search_range);
    buf[7] = LOW(search_range);
    buf[8] = HIGH(entry_selector);
    buf[9] = LOW(entry_selector);
    buf[10] = HIGH(range_shift);
    buf[11] = LOW(range_shift);
  }

  /* location of the first table info record */
  table_record = &buf[12];

  head_checksum = 0;

  /* loop over all tables */
  for (i = 0; i < num_table_infos; i++)
  {
    SFNT_Table_Info table_info = table_infos[i];
    SFNT_Table* table;


    /* ignore empty slots */
    if (table_info == MISSING)
      continue;

    table = &tables[table_info];

    if (table->tag == TTAG_head)
    {
      FT_ULong date_high;
      FT_ULong date_low;


      /* we always reach this IF clause since FreeType would */
      /* have aborted already if the `head' table were missing */

      head_buf = table->buf;

      /* reset checksum in `head' table for recalculation */
      head_buf[8] = 0x00;
      head_buf[9] = 0x00;
      head_buf[10] = 0x00;
      head_buf[11] = 0x00;

      /* update modification time */
      TA_get_current_time(&date_high, &date_low);

      head_buf[28] = BYTE1(date_high);
      head_buf[29] = BYTE2(date_high);
      head_buf[30] = BYTE3(date_high);
      head_buf[31] = BYTE4(date_high);

      head_buf[32] = BYTE1(date_low);
      head_buf[33] = BYTE2(date_low);
      head_buf[34] = BYTE3(date_low);
      head_buf[35] = BYTE4(date_low);

      table->checksum = TA_table_compute_checksum(table->buf, table->len);
    }

    head_checksum += table->checksum;

    table_record[0] = BYTE1(table->tag);
    table_record[1] = BYTE2(table->tag);
    table_record[2] = BYTE3(table->tag);
    table_record[3] = BYTE4(table->tag);

    table_record[4] = BYTE1(table->checksum);
    table_record[5] = BYTE2(table->checksum);
    table_record[6] = BYTE3(table->checksum);
    table_record[7] = BYTE4(table->checksum);

    table_record[8] = BYTE1(table->offset);
    table_record[9] = BYTE2(table->offset);
    table_record[10] = BYTE3(table->offset);
    table_record[11] = BYTE4(table->offset);

    table_record[12] = BYTE1(table->len);
    table_record[13] = BYTE2(table->len);
    table_record[14] = BYTE3(table->len);
    table_record[15] = BYTE4(table->len);

    table_record += 16;
  }

  /* the font header is complete; compute `head' checksum */
  head_checksum += TA_table_compute_checksum(buf, len);
  head_checksum = 0xB1B0AFBAUL - head_checksum;

  /* store checksum in `head' table; */
  head_buf[8] = BYTE1(head_checksum);
  head_buf[9] = BYTE2(head_checksum);
  head_buf[10] = BYTE3(head_checksum);
  head_buf[11] = BYTE4(head_checksum);

  *header_buf = buf;
  *header_len = len;

  return TA_Err_Ok;
}


static FT_Error
TA_font_build_TTF(FONT* font)
{
  SFNT* sfnt = &font->sfnts[0];

  SFNT_Table* tables;
  FT_ULong num_tables;

  FT_ULong SFNT_offset;

  FT_Byte* DSIG_buf;

  FT_Byte* header_buf;
  FT_ULong header_len;

  FT_ULong i;
  FT_Error error;


  /* replace an existing `DSIG' table with a dummy */

  if (font->have_DSIG)
  {
    error = TA_sfnt_add_table_info(sfnt);
    if (error)
      return error;

    error = TA_table_build_DSIG(&DSIG_buf);
    if (error)
      return error;

    /* in case of success, `DSIG_buf' gets linked */
    /* and is eventually freed in `TA_font_unload' */
    error = TA_font_add_table(font,
                              &sfnt->table_infos[sfnt->num_table_infos - 1],
                              TTAG_DSIG, DSIG_LEN, DSIG_buf);
    if (error)
    {
      free(DSIG_buf);
      return error;
    }
  }

  TA_sfnt_sort_table_info(sfnt, font);

  /* the first SFNT table immediately follows the header */
  (void)TA_sfnt_build_TTF_header(sfnt, font, NULL, &SFNT_offset, 0);
  TA_font_compute_table_offsets(font, SFNT_offset);

  error = TA_sfnt_build_TTF_header(sfnt, font,
                                   &header_buf, &header_len, 1);
  if (error)
    return error;

  /* build font */

  tables = font->tables;
  num_tables = font->num_tables;

  /* get font length from last SFNT table array element */
  font->out_len = tables[num_tables - 1].offset
                  + ((tables[num_tables - 1].len + 3) & ~3);
  font->out_buf = (FT_Byte*)malloc(font->out_len);
  if (!font->out_buf)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err;
  }

  memcpy(font->out_buf, header_buf, header_len);

  for (i = 0; i < num_tables; i++)
  {
    SFNT_Table* table = &tables[i];


    /* buffer length is a multiple of 4 */
    memcpy(font->out_buf + table->offset,
           table->buf, (table->len + 3) & ~3);
  }

  error = TA_Err_Ok;

Err:
  free(header_buf);

  return error;
}


static FT_Error
TA_font_build_TTC_header(FONT* font,
                         FT_Byte** header_buf,
                         FT_ULong* header_len)
{
  SFNT* sfnts = font->sfnts;
  FT_Long num_sfnts = font->num_sfnts;

  SFNT_Table* tables = font->tables;
  FT_ULong num_tables = font->num_tables;

  FT_ULong TTF_offset;
  FT_ULong DSIG_offset;

  FT_Byte* buf;
  FT_ULong len;

  FT_Long i;
  FT_Byte* p;


  len = 24 + 4 * num_sfnts;
  buf = (FT_Byte*)malloc(len);
  if (!buf)
    return FT_Err_Out_Of_Memory;

  p = buf;

  /* TTC ID string */
  *(p++) = 't';
  *(p++) = 't';
  *(p++) = 'c';
  *(p++) = 'f';

  /* TTC header version */
  *(p++) = 0x00;
  *(p++) = font->have_DSIG ? 0x02 : 0x01;
  *(p++) = 0x00;
  *(p++) = 0x00;

  /* number of subfonts */
  *(p++) = BYTE1(num_sfnts);
  *(p++) = BYTE2(num_sfnts);
  *(p++) = BYTE3(num_sfnts);
  *(p++) = BYTE4(num_sfnts);

  /* the first TTF subfont header immediately follows the TTC header */
  TTF_offset = len;

  /* loop over all subfonts */
  for (i = 0; i < num_sfnts; i++)
  {
    SFNT* sfnt = &sfnts[i];
    FT_ULong l;


    TA_sfnt_sort_table_info(sfnt, font);
    /* only get header length */
    (void)TA_sfnt_build_TTF_header(sfnt, font, NULL, &l, 0);

    *(p++) = BYTE1(TTF_offset);
    *(p++) = BYTE2(TTF_offset);
    *(p++) = BYTE3(TTF_offset);
    *(p++) = BYTE4(TTF_offset);

    TTF_offset += l;
  }

  /* the first SFNT table immediately follows the subfont TTF headers */
  TA_font_compute_table_offsets(font, TTF_offset);

  if (font->have_DSIG)
  {
    /* DSIG tag */
    *(p++) = 'D';
    *(p++) = 'S';
    *(p++) = 'I';
    *(p++) = 'G';

    /* DSIG length */
    *(p++) = 0x00;
    *(p++) = 0x00;
    *(p++) = 0x00;
    *(p++) = 0x08;

    /* DSIG offset; in a TTC this is always the last SFNT table */
    DSIG_offset = tables[num_tables - 1].offset;

    *(p++) = BYTE1(DSIG_offset);
    *(p++) = BYTE2(DSIG_offset);
    *(p++) = BYTE3(DSIG_offset);
    *(p++) = BYTE4(DSIG_offset);
  }

  *header_buf = buf;
  *header_len = len;

  return TA_Err_Ok;
}


static FT_Error
TA_font_build_TTC(FONT* font)
{
  SFNT* sfnts = font->sfnts;
  FT_Long num_sfnts = font->num_sfnts;

  SFNT_Table* tables;
  FT_ULong num_tables;

  FT_Byte* DSIG_buf;
  SFNT_Table_Info dummy;

  FT_Byte* TTC_header_buf;
  FT_ULong TTC_header_len;

  FT_Byte** TTF_header_bufs = NULL;
  FT_ULong* TTF_header_lens = NULL;

  FT_ULong offset;
  FT_Long i;
  FT_ULong j;
  FT_Error error;


  /* replace an existing `DSIG' table with a dummy */

  if (font->have_DSIG)
  {
    error = TA_table_build_DSIG(&DSIG_buf);
    if (error)
      return error;

    /* in case of success, `DSIG_buf' gets linked */
    /* and is eventually freed in `TA_font_unload' */
    error = TA_font_add_table(font, &dummy, TTAG_DSIG, DSIG_LEN, DSIG_buf);
    if (error)
    {
      free(DSIG_buf);
      return error;
    }
  }

  /* this also computes the SFNT table offsets */
  error = TA_font_build_TTC_header(font,
                                   &TTC_header_buf, &TTC_header_len);
  if (error)
    return error;

  TTF_header_bufs = (FT_Byte**)calloc(1, num_sfnts * sizeof (FT_Byte*));
  if (!TTF_header_bufs)
    goto Err;

  TTF_header_lens = (FT_ULong*)malloc(num_sfnts * sizeof (FT_ULong));
  if (!TTF_header_lens)
    goto Err;

  for (i = 0; i < num_sfnts; i++)
  {
    error = TA_sfnt_build_TTF_header(&sfnts[i], font,
                                     &TTF_header_bufs[i],
                                     &TTF_header_lens[i], 1);
    if (error)
      goto Err;
  }

  /* build font */

  tables = font->tables;
  num_tables = font->num_tables;

  /* get font length from last SFNT table array element */
  font->out_len = tables[num_tables - 1].offset
                  + ((tables[num_tables - 1].len + 3) & ~3);
  font->out_buf = (FT_Byte*)malloc(font->out_len);
  if (!font->out_buf)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err;
  }

  memcpy(font->out_buf, TTC_header_buf, TTC_header_len);

  offset = TTC_header_len;

  for (i = 0; i < num_sfnts; i++)
  {
    memcpy(font->out_buf + offset,
           TTF_header_bufs[i], TTF_header_lens[i]);

    offset += TTF_header_lens[i];
  }

  for (j = 0; j < num_tables; j++)
  {
    SFNT_Table* table = &tables[j];


    /* buffer length is a multiple of 4 */
    memcpy(font->out_buf + table->offset,
           table->buf, (table->len + 3) & ~3);
  }

  error = TA_Err_Ok;

Err:
  free(TTC_header_buf);
  if (TTF_header_bufs)
  {
    for (i = 0; i < font->num_sfnts; i++)
      free(TTF_header_bufs[i]);
    free(TTF_header_bufs);
  }
  free(TTF_header_lens);

  return error;
}


#define COMPARE(str) (len == (sizeof (str) - 1) \
                      && !strncmp(start, str, sizeof (str) - 1))


TA_Error
TTF_autohint(const char* options,
             ...)
{
  va_list ap;

  FONT* font;
  FT_Error error;
  FT_Long i;

  FILE* in_file = NULL;
  FILE* out_file = NULL;

  const char* in_buf = NULL;
  size_t in_len = 0;
  char** out_bufp = NULL;
  size_t* out_lenp = NULL;

  const unsigned char** error_stringp = NULL;

  FT_Long hinting_range_min = -1;
  FT_Long hinting_range_max = -1;

  TA_Progress_Func progress;
  void* progress_data;

  FT_Bool ignore_permissions = 0;
  FT_UInt fallback_script = 0;

  const char *op;


  if (!options || !*options)
  {
    error = FT_Err_Invalid_Argument;
    goto Err1;
  }

  /* XXX */
  va_start(ap, options);

  op = options;

  for(;;)
  {
    const char* start;
    size_t len;


    start = op;

    /* search comma */
    while (*op && *op != ',')
      op++;

    /* remove leading whitespace */
    while (isspace(*start))
      start++;

    /* check for empty option */
    if (start == op)
      goto End;

    len = op - start;

    /* the `COMPARE' macro uses `len' and `start' */

    /* handle option */
    if (COMPARE("in-file"))
    {
      in_file = va_arg(ap, FILE*);
      in_buf = NULL;
      in_len = 0;
    }
    else if (COMPARE("in-buffer"))
    {
      in_file = NULL;
      in_buf = va_arg(ap, const char*);
    }
    else if (COMPARE("in-buffer-len"))
    {
      in_file = NULL;
      in_len = va_arg(ap, size_t);
    }
    else if (COMPARE("out-file"))
    {
      out_file = va_arg(ap, FILE*);
      out_bufp = NULL;
      out_lenp = NULL;
    }
    else if (COMPARE("out-buffer"))
    {
      out_file = NULL;
      out_bufp = va_arg(ap, char**);
    }
    else if (COMPARE("out-buffer-len"))
    {
      out_file = NULL;
      out_lenp = va_arg(ap, size_t*);
    }
    else if (COMPARE("error-string"))
      error_stringp = va_arg(ap, const unsigned char**);
    else if (COMPARE("hinting-range-min"))
      hinting_range_min = (FT_Long)va_arg(ap, FT_UInt);
    else if (COMPARE("hinting-range-max"))
      hinting_range_max = (FT_Long)va_arg(ap, FT_UInt);
    else if (COMPARE("progress-callback"))
      progress = va_arg(ap, TA_Progress_Func);
    else if (COMPARE("progress-callback-data"))
      progress_data = va_arg(ap, void*);
    else if (COMPARE("ignore-permissions"))
      ignore_permissions = (FT_Bool)va_arg(ap, FT_Int);
    else if (COMPARE("fallback-script"))
      fallback_script = va_arg(ap, FT_UInt);

    /*
      pre-hinting
      x-height-snapping-exceptions
     */

  End:
    if (!*op)
      break;
    op++;
  }

  va_end(ap);

  /* check options */

  if (!(in_file
        || (in_buf && in_len)))
  {
    error = FT_Err_Invalid_Argument;
    goto Err1;
  }

  if (!(out_file
        || (out_bufp && out_lenp)))
  {
    error = FT_Err_Invalid_Argument;
    goto Err1;
  }

  font = (FONT*)calloc(1, sizeof (FONT));
  if (!font)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err1;
  }

  if (hinting_range_min >= 0 && hinting_range_min < 2)
  {
    error = FT_Err_Invalid_Argument;
    goto Err1;
  }
  if (hinting_range_min < 0)
    hinting_range_min = 8;

  if (hinting_range_max >= 0 && hinting_range_max < hinting_range_min)
  {
    error = FT_Err_Invalid_Argument;
    goto Err1;
  }
  if (hinting_range_max < 0)
    hinting_range_max = 1000;

  font->hinting_range_min = (FT_UInt)hinting_range_min;
  font->hinting_range_max = (FT_UInt)hinting_range_max;

  font->progress = progress;
  font->progress_data = progress_data;

  font->ignore_permissions = ignore_permissions;
  /* restrict value to two bits */
  font->fallback_script = fallback_script & 3;

  /* now start with processing the data */

  if (in_file)
  {
    error = TA_font_file_read(font, in_file);
    if (error)
      goto Err;
  }
  else
  {
    /* a valid TTF can never be that small */
    if (in_len < 100)
    {
      error = FT_Err_Invalid_Argument;
      goto Err1;
    }
    font->in_buf = (FT_Byte*)in_buf;
    font->in_len = in_len;
  }

  error = TA_font_init(font);
  if (error)
    goto Err;

  /* loop over subfonts */
  for (i = 0; i < font->num_sfnts; i++)
  {
    SFNT* sfnt = &font->sfnts[i];


    error = FT_New_Memory_Face(font->lib, font->in_buf, font->in_len,
                               i, &sfnt->face);
    if (error)
      goto Err;

    error = TA_sfnt_split_into_SFNT_tables(sfnt, font);
    if (error)
      goto Err;

    error = TA_sfnt_split_glyf_table(sfnt, font);
    if (error)
      goto Err;

    /* check permission */
    if (sfnt->OS2_idx != MISSING)
    {
      SFNT_Table* OS2_table = &font->tables[sfnt->OS2_idx];


      /* check lower byte of the `fsType' field */
      if (OS2_table->buf[OS2_FSTYPE_OFFSET + 1] == 0x02
          && !font->ignore_permissions)
      {
        error = TA_Err_Missing_Legal_Permission;
        goto Err;
      }
    }
  }

  /* build `gasp' table */
  error = TA_sfnt_build_gasp_table(&font->sfnts[0], font);
  if (error)
    goto Err;

  /* XXX handle subfonts for bytecode tables */

  /* build `cvt ' table */
  error = TA_sfnt_build_cvt_table(&font->sfnts[0], font);
  if (error)
    goto Err;

  /* build `fpgm' table */
  error = TA_sfnt_build_fpgm_table(&font->sfnts[0], font);
  if (error)
    goto Err;

  /* build `prep' table */
  error = TA_sfnt_build_prep_table(&font->sfnts[0], font);
  if (error)
    goto Err;

  /* hint the glyphs and build bytecode */
  error = TA_sfnt_build_glyf_hints(&font->sfnts[0], font);
  if (error)
    goto Err;

  /* loop again over subfonts */
  for (i = 0; i < font->num_sfnts; i++)
  {
    SFNT* sfnt = &font->sfnts[i];


    error = TA_sfnt_build_glyf_table(sfnt, font);
    if (error)
      goto Err;
    error = TA_sfnt_build_loca_table(sfnt, font);
    if (error)
      goto Err;
    error = TA_sfnt_update_maxp_table(sfnt, font);
    if (error)
      goto Err;
  }

  if (font->num_sfnts == 1)
    error = TA_font_build_TTF(font);
  else
    error = TA_font_build_TTC(font);
  if (error)
    goto Err;

  if (out_file)
  {
    error = TA_font_file_write(font, out_file);
    if (error)
      goto Err;
  }
  else
  {
    *out_bufp = (char*)font->out_buf;
    *out_lenp = font->out_len;
  }

  error = TA_Err_Ok;

Err:
  TA_font_unload(font, in_buf, out_bufp);

Err1:
  if (error_stringp)
    *error_stringp = (const unsigned char*)TA_get_error_message(error);

  return error;
}

/* end of ttfautohint.c */
