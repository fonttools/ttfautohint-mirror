/* tadump.c */

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

#include <stdarg.h>


#define DUMPVAL(str, arg) \
          do \
          { \
            s = sdscatprintf(s, \
                             "%*s = %ld\n", \
                             width, (str), (FT_Long)(arg)); \
          } while (0)
#define DUMPSTR(str, arg) \
          do \
          { \
            s = sdscatprintf(s, \
                             "%*s = %s%s", \
                             width, (str), (arg), eol); \
          } while (0)
#define DUMPSTRX(arg) \
          do \
          { \
            s = sdscatprintf(s, \
                             "%s%*s   %s%s", \
                             prev_eol, width, "", (arg), eol); \
          } while (0)



/* if `format' is set, we present the data in a more friendly format */

char*
TA_font_dump_parameters(FONT* font,
                        FT_Bool format)
{
  sds s;
  size_t len;
  char* res;

  char* ns = NULL;
  char* ds = NULL;

  int width = 0;
  const char* eol = "\n";
  const char* prev_eol = "";


  s = sdsempty();

  if (format)
  {
    s = sdscat(s, "TTF_autohint parameters\n"
                  "=======================\n");
    width = 33;
  }

  s = sdscat(s, "\n");

  DUMPSTR("ttfautohint version",
          VERSION);

  s = sdscat(s, "\n");

  if (font->dehint)
  {
    if (format)
      DUMPVAL("dehint",
              font->dehint);
    goto Exit;
  }

  DUMPVAL("adjust-subglyphs",
          font->adjust_subglyphs);
  DUMPSTR("default-script",
          script_names[font->default_script]);
  DUMPSTR("dw-cleartype-stem-width-mode",
          (font->dw_cleartype_stem_width_mode == TA_STEM_WIDTH_MODE_NATURAL)
            ? "natural"
            : (font->dw_cleartype_stem_width_mode == TA_STEM_WIDTH_MODE_QUANTIZED)
                ? "quantized"
                : "strong");
  DUMPVAL("fallback-scaling",
          font->fallback_scaling);
  DUMPSTR("fallback-script",
          script_names[ta_style_classes[font->fallback_style]->script]);
  DUMPVAL("fallback-stem-width",
          font->fallback_stem_width);
  DUMPSTR("gdi-cleartype-stem-width-mode",
          (font->gdi_cleartype_stem_width_mode == TA_STEM_WIDTH_MODE_NATURAL)
            ? "natural"
            : (font->gdi_cleartype_stem_width_mode == TA_STEM_WIDTH_MODE_QUANTIZED)
                ? "quantized"
                : "strong");
  DUMPSTR("gray-stem-width-mode",
          (font->gray_stem_width_mode == TA_STEM_WIDTH_MODE_NATURAL)
            ? "natural"
            : (font->gray_stem_width_mode == TA_STEM_WIDTH_MODE_QUANTIZED)
                ? "quantized"
                : "strong");
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
  if (font->reference_name)
    DUMPSTR("reference",
            font->reference_name);
  else if (font->reference_buf)
    DUMPSTR("reference",
            "<yes>");
  else
    DUMPSTR("reference",
            "");
  DUMPVAL("reference-index",
          font->reference_index);
  DUMPVAL("symbol",
          font->symbol);
  DUMPVAL("TTFA-info",
          font->TTFA_info);
  DUMPVAL("windows-compatibility",
          font->windows_compatibility);

  ns = number_set_show(font->x_height_snapping_exceptions,
                       TA_PROP_INCREASE_X_HEIGHT_MIN, 0x7FFF);
  if (!ns)
  {
    sdsfree(s);
    s = NULL;
    goto Exit;
  }

  DUMPSTR("x-height-snapping-exceptions", ns);

  ds = TA_control_show(font);
  if (!ds)
  {
    sdsfree(s);
    s = NULL;
    goto Exit;
  }

  if (*ds)
  {
    char* token;
    char* saveptr;


    token = strtok_r(ds, "\n", &saveptr);
    if (format)
      DUMPSTR("control-instructions", token);
    else
    {
      DUMPSTR("control-instructions", "\\");
      eol = "";
      /* show control instructions line by line */
      DUMPSTRX(token);
      prev_eol = "; \\\n";
    }

    for (;;)
    {
      token = strtok_r(NULL, "\n", &saveptr);
      if (!token)
        break;

      DUMPSTRX(token);
    }
  }
  else
    DUMPSTR("control-instructions", "");

  if (!format)
    s = sdscat(s, "\n");
  s = sdscat(s, "\n");

Exit:
  free(ns);
  free(ds);

  if (!s)
    return NULL;

  len = sdslen(s) + 1;
  res = (char*)malloc(len);
  if (res)
    memcpy(res, s, len);

  sdsfree(s);

  return res;
}

/* end of tadump.c */
