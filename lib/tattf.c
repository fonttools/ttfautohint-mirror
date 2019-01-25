/* tattf.c */

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


/* If `do_complete' is 0, only return `header_len'. */

FT_Error
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
    FT_ULong j;


    for (i = 1, j = 2; j <= num_tables_in_header; i++, j <<= 1)
      ;

    entry_selector = i - 1;
    search_range = 0x10U << entry_selector;
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

      /* update flags; we have to */
      /* set bit 2 (`instructions may depend on point size') and to */
      /* clear bit 4 (`instructions may alter advance width') */
      head_buf[17] |= 0x04U;
      head_buf[17] &= ~0x10U;

      /* update modification time */
      TA_get_current_time(font, &date_high, &date_low);

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


FT_Error
TA_font_build_TTF(FONT* font)
{
  SFNT* sfnt = &font->sfnts[0];

  SFNT_Table* tables;
  FT_ULong num_tables;

  FT_ULong SFNT_offset;

  FT_Byte* header_buf;
  FT_ULong header_len;

  FT_ULong i;
  FT_Error error;


  /* add our information table */

  if (font->TTFA_info)
  {
    FT_Byte* TTFA_buf;
    FT_ULong TTFA_len;


    error = TA_sfnt_add_table_info(sfnt);
    if (error)
      return error;

    error = TA_table_build_TTFA(&TTFA_buf, &TTFA_len, font);
    if (error)
      return error;

    /* in case of success, `TTFA_buf' gets linked */
    /* and is eventually freed in `TA_font_unload' */
    error = TA_font_add_table(font,
                              &sfnt->table_infos[sfnt->num_table_infos - 1],
                              TTAG_TTFA, TTFA_len, TTFA_buf);
    if (error)
    {
      free(TTFA_buf);
      return error;
    }
  }

  /* replace an existing `DSIG' table with a dummy */

  if (font->have_DSIG)
  {
    FT_Byte* DSIG_buf;


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
                  + ((tables[num_tables - 1].len + 3) & ~3U);
  /* if `out-buffer' is set, this buffer gets returned to the user, */
  /* thus we use the customized allocator function */
  font->out_buf = (FT_Byte*)font->allocate(font->out_len);
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
           table->buf, (table->len + 3) & ~3U);
  }

  error = TA_Err_Ok;

Err:
  free(header_buf);

  return error;
}

/* end of tattf.c */
