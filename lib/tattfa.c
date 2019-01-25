/* tattfa.c */

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


#include "ta.h"


FT_Error
TA_table_build_TTFA(FT_Byte** TTFA,
                    FT_ULong* TTFA_len,
                    FONT* font)
{
  FT_Byte* buf;
  FT_UInt buf_len;

  FT_UInt len;
  FT_Byte* buf_new;
  FT_Byte* p;


  buf = (FT_Byte*)TA_font_dump_parameters(font, 0);
  if (!buf)
    return FT_Err_Out_Of_Memory;
  buf_len = (FT_UInt)strlen((char*)buf);

  /* buffer length must be a multiple of four */
  len = (buf_len + 3) & ~3U;
  buf_new = (FT_Byte*)realloc(buf, len);
  if (!buf_new)
  {
    free(buf);
    return FT_Err_Out_Of_Memory;
  }
  buf = buf_new;

  /* pad end of buffer with zeros */
  p = buf + buf_len;
  switch (buf_len % 4)
  {
  case 1:
    *(p++) = 0x00;
  case 2:
    *(p++) = 0x00;
  case 3:
    *(p++) = 0x00;
  default:
    break;
  }

  *TTFA = buf;
  *TTFA_len = buf_len;

  return TA_Err_Ok;
}


/* end of tattfa.c */
