/* tadump.c */

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

#define _POSIX_SOURCE /* to access `strtok_r' with glibc */

#include "ta.h"
#include <string.h>


#define DUMP_COLUMN "33"

#define DUMPVAL(str, arg) \
          fprintf(stderr, "%" DUMP_COLUMN "s = %ld\n", \
                          (str), \
                          (FT_Long)(arg))
#define DUMPSTR(str, arg) \
          fprintf(stderr, "%" DUMP_COLUMN "s = %s\n", \
                          (str), \
                          (arg))
#define DUMPSTRX(arg) \
          fprintf(stderr, "%" DUMP_COLUMN "s   %s\n", \
                          "", \
                          (arg))


TA_Error
TA_font_dump_parameters(FONT* font,
                        Deltas* deltas,
                        FT_Bool dehint)
{
  char* s;
  char* token;
  char* saveptr;


  fprintf(stderr, "TTF_autohint parameters\n"
                  "=======================\n"
                  "\n");

  if (dehint)
  {
    DUMPVAL("dehint",
            font->dehint);
    return TA_Err_Ok;
  }

  DUMPVAL("adjust-subglyphs",
          font->adjust_subglyphs);
  DUMPSTR("default-script",
          script_names[font->default_script]);
  DUMPVAL("dw-cleartype-strong-stem-width",
          font->dw_cleartype_strong_stem_width);
  DUMPSTR("fallback-script",
          script_names[ta_style_classes[font->fallback_style]->script]);
  DUMPVAL("fallback-stem-width",
          font->fallback_stem_width);
  DUMPVAL("gdi-cleartype-strong-stem-width",
          font->gdi_cleartype_strong_stem_width);
  DUMPVAL("gray-strong-stem-width",
          font->gray_strong_stem_width);
  DUMPVAL("hinting-limit",
          font->hinting_limit);
  DUMPVAL("hinting-range-max",
          font->hinting_range_max);
  DUMPVAL("hinting-range-min",
          font->hinting_range_min);
  DUMPVAL("hint-composites",
          font->hint_composites);
  DUMPVAL("ignore-restrictions",
          font->ignore_restrictions);
  DUMPVAL("increase-x-height",
          font->increase_x_height);
  DUMPVAL("symbol",
          font->symbol);
  DUMPVAL("windows-compatibility",
          font->windows_compatibility);

  s = number_set_show(font->x_height_snapping_exceptions,
                      TA_PROP_INCREASE_X_HEIGHT_MIN, 0x7FFF);
  if (!s)
    return FT_Err_Out_Of_Memory;

  DUMPSTR("x-height-snapping-exceptions", s);
  free(s);

  s = TA_deltas_show(font, deltas);
  if (!s)
    return FT_Err_Out_Of_Memory;

  /* show delta exceptions data line by line */
  token = strtok_r(s, "\n", &saveptr);
  DUMPSTR("delta exceptions", token);

  for (;;)
  {
    token = strtok_r(NULL, "\n", &saveptr);
    if (!token)
      break;

    DUMPSTRX(token);
  }

  free(s);

  fprintf(stderr, "\n");

  return TA_Err_Ok;
}

/* end of tadump.c */
