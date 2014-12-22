// info.cpp

// Copyright (C) 2012-2014 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// the next header file is from gnulib defining function `base_name',
// which is a replacement for `basename' that works on Windows also
#include "dirname.h"

#include "info.h"
#include <sds.h>
#include <numberset.h>


#define TTFAUTOHINT_STRING "; ttfautohint"
#define TTFAUTOHINT_STRING_WIDE "\0;\0 \0t\0t\0f\0a\0u\0t\0o\0h\0i\0n\0t"

// build string that gets appended to the `Version' field(s)

extern "C" {

// return value 1 means allocation error, value 2 too long a string

int
build_version_string(Info_Data* idata)
{
  // since we use `goto' we have to initialize variables before the jumps
  unsigned char* data;
  unsigned char* data_wide;
  unsigned char* dt;
  unsigned char* dtw;
  char* s = NULL;
  char strong[4];
  int count;
  int ret = 0;
  sds d;

  d = sdsempty();

  d = sdscatprintf(d, TTFAUTOHINT_STRING " (v%s)", VERSION);
  if (!idata->detailed)
    goto Skip;

  if (idata->dehint)
  {
    d = sdscat(d, " -d");
    goto Skip;
  }
  d = sdscatprintf(d, " -l %d", idata->hinting_range_min);
  d = sdscatprintf(d, " -r %d", idata->hinting_range_max);
  d = sdscatprintf(d, " -G %d", idata->hinting_limit);
  d = sdscatprintf(d, " -x %d", idata->increase_x_height);
  if (idata->fallback_stem_width)
    d = sdscatprintf(d, " -H %d", idata->fallback_stem_width);
  d = sdscatprintf(d, " -D %s", idata->default_script);
  d = sdscatprintf(d, " -f %s", idata->fallback_script);
  if (idata->control_name)
  {
    char* bn = base_name(idata->control_name);
    d = sdscatprintf(d, " -m \"%s\"", bn ? bn : idata->control_name);
    free(bn);
  }

  count = 0;
  strong[0] = '\0';
  strong[1] = '\0';
  strong[2] = '\0';
  strong[3] = '\0';
  if (idata->gray_strong_stem_width)
    strong[count++] = 'g';
  if (idata->gdi_cleartype_strong_stem_width)
    strong[count++] = 'G';
  if (idata->dw_cleartype_strong_stem_width)
    strong[count++] = 'D';
  if (*strong)
    d = sdscatprintf(d, " -w %s", strong);
  else
    d = sdscat(d, " -w \"\"");

  if (idata->windows_compatibility)
    d = sdscat(d, " -W");
  if (idata->adjust_subglyphs)
    d = sdscat(d, " -p");
  if (idata->hint_composites)
    d = sdscat(d, " -c");
  if (idata->symbol)
    d = sdscat(d, " -s");
  if (idata->TTFA_info)
    d = sdscat(d, " -t");

  if (idata->x_height_snapping_exceptions_string)
  {
    // only set specific value of `ret' for an allocation error,
    // since syntax errors are handled in TTF_autohint
    number_range* x_height_snapping_exceptions;
    const char* pos = number_set_parse(
                        idata->x_height_snapping_exceptions_string,
                        &x_height_snapping_exceptions,
                        6, 0x7FFF);
    if (*pos)
    {
      if (x_height_snapping_exceptions == NUMBERSET_ALLOCATION_ERROR)
        ret = 1;
      goto Fail;
    }

    s = number_set_show(x_height_snapping_exceptions, 6, 0x7FFF);
    number_set_free(x_height_snapping_exceptions);

    // ensure UTF16-BE version doesn't get too long
    if (strlen(s) > 0xFFFF / 2 - sdslen(d))
    {
      ret = 2;
      goto Fail;
    }

    d = sdscatprintf(d, " -X \"%s\"", s);
  }

Skip:
  if (!d)
  {
    ret = 1;
    goto Fail;
  }

  data = (unsigned char*)malloc(sdslen(d) + 1);
  if (!data)
  {
    ret = 1;
    goto Fail;
  }
  memcpy(data, d, sdslen(d) + 1);

  idata->data = data;
  idata->data_len = (unsigned short)sdslen(d);

  // prepare UTF16-BE version data
  idata->data_wide_len = 2 * idata->data_len;
  data_wide = (unsigned char*)realloc(idata->data_wide,
                                      idata->data_wide_len);
  if (!data_wide)
  {
    ret = 1;
    goto Fail;
  }
  idata->data_wide = data_wide;

  dt = idata->data;
  dtw = idata->data_wide;
  for (unsigned short i = 0; i < idata->data_len; i++)
  {
    *(dtw++) = '\0';
    *(dtw++) = *(dt++);
  }

Exit:
  free(s);
  sdsfree(d);

  return ret;

Fail:
  free(idata->data);
  free(idata->data_wide);

  idata->data = NULL;
  idata->data_wide = NULL;
  idata->data_len = 0;
  idata->data_wide_len = 0;

  goto Exit;
}


int
info(unsigned short platform_id,
     unsigned short encoding_id,
     unsigned short /* language_id */,
     unsigned short name_id,
     unsigned short* len,
     unsigned char** str,
     void* user)
{
  Info_Data* idata = (Info_Data*)user;
  unsigned char ttfautohint_string[] = TTFAUTOHINT_STRING;
  unsigned char ttfautohint_string_wide[] = TTFAUTOHINT_STRING_WIDE;

  // we use memmem, so don't count the trailing \0 character
  size_t ttfautohint_string_len = sizeof (TTFAUTOHINT_STRING) - 1;
  size_t ttfautohint_string_wide_len = sizeof (TTFAUTOHINT_STRING_WIDE) - 1;

  unsigned char* v;
  unsigned short v_len;
  unsigned char* s;
  size_t s_len;
  size_t offset;

  // if it is a version string, append our data
  if (name_id != 5)
    return 0;

  if (platform_id == 1
      || (platform_id == 3 && !(encoding_id == 1
                                || encoding_id == 10)))
  {
    // one-byte or multi-byte encodings
    v = idata->data;
    v_len = idata->data_len;
    s = ttfautohint_string;
    s_len = ttfautohint_string_len;
    offset = 2;
  }
  else
  {
    // (two-byte) UTF-16BE for everything else
    v = idata->data_wide;
    v_len = idata->data_wide_len;
    s = ttfautohint_string_wide;
    s_len = ttfautohint_string_wide_len;
    offset = 4;
  }

  // if we already have an ttfautohint info string,
  // remove it up to a following `;' character (or end of string)
  unsigned char* s_start = (unsigned char*)memmem(*str, *len, s, s_len);
  if (s_start)
  {
    unsigned char* s_end = s_start + offset;
    unsigned char* limit = *str + *len;

    while (s_end < limit)
    {
      if (*s_end == ';')
      {
        if (offset == 2)
          break;
        else
        {
          if (*(s_end - 1) == '\0') // UTF-16BE
          {
            s_end--;
            break;
          }
        }
      }

      s_end++;
    }

    while (s_end < limit)
      *s_start++ = *s_end++;

    *len -= s_end - s_start;
  }

  // do nothing if the string would become too long
  if (*len > 0xFFFF - v_len)
    return 0;

  unsigned short len_new = *len + v_len;
  unsigned char* str_new = (unsigned char*)realloc(*str, len_new);
  if (!str_new)
    return 1;

  *str = str_new;
  memcpy(*str + *len, v, v_len);
  *len = len_new;

  return 0;
}

} // extern "C"

// end of info.cpp
