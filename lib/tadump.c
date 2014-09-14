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

#include "ta.h"

#include <stdarg.h>


typedef struct elem_
{
  char* s;
  int len;

  struct elem_* next;
} elem;



#define DUMPVAL(str, arg) \
          do \
          { \
            list = list_prepend(list, \
                                "%*s = %ld\n", \
                                width, (str), (FT_Long)(arg)); \
            if (!list) \
              goto Fail; \
          } while (0)
#define DUMPSTR(str, arg) \
          do \
          { \
            list = list_prepend(list, \
                                "%*s = %s%s", \
                                width, (str), (arg), eol); \
            if (!list) \
              goto Fail; \
          } while (0)
#define DUMPSTRX(arg) \
          do \
          { \
            list = list_prepend(list, \
                                "%s%*s   %s%s", \
                                prev_eol, width, "", (arg), eol); \
            if (!list) \
              goto Fail; \
          } while (0)


void
list_free(elem* list)
{
  while (list)
  {
    elem* tmp;


    tmp = list;
    list = list->next;
    free(tmp->s);
    free(tmp);
  }
}


elem*
list_prepend(elem* list,
             const char* fmt,
             ...)
{
  va_list ap;
  char* s;
  int len;

  elem* e;


  va_start(ap, fmt);
  len = vasprintf(&s, fmt, ap);
  va_end(ap);

  if (len == -1)
  {
    list_free(list);
    return NULL;
  }

  e = (elem*)malloc(sizeof (elem));
  if (!e)
  {
    free(s);
    list_free(list);
    return NULL;
  }

  e->s = s;
  e->len = len;
  e->next = list;

  return e;
}


elem*
list_reverse(elem* list)
{
  elem* cur;


  cur = list;
  list = NULL;

  while (cur)
  {
    elem* tmp;


    tmp = cur;
    cur = cur->next;
    tmp->next = list;
    list = tmp;
  }

  return list;
}


char*
list_concat(elem* list)
{
  elem* e;
  int len;

  char* buf;
  char* bufp;


  e = list;
  len = 0;
  while (e)
  {
    len += e->len;
    e = e->next;
  }

  buf = (char*)malloc(len + 1);
  if (!buf)
    return NULL;

  e = list;
  bufp = buf;
  while (e)
  {
    strcpy(bufp, e->s);
    bufp += e->len;
    e = e->next;
  }

  return buf;
}


/* if `format' is set, we present the data in a more friendly format */

char*
TA_font_dump_parameters(FONT* font,
                        FT_Bool format)
{
  char* buf;
  elem* list = NULL;

  char* ns = NULL;
  char* ds = NULL;

  int width = 0;
  const char* eol = "\n";
  const char* prev_eol = "";


  if (format)
  {
    list = list_prepend(list, "TTF_autohint parameters\n"
                              "=======================\n"
                              "\n");
    if (!list)
      goto Fail;

    width = 33;
  }

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
  DUMPVAL("TTFA-info",
          font->TTFA_info);
  DUMPVAL("windows-compatibility",
          font->windows_compatibility);

  ns = number_set_show(font->x_height_snapping_exceptions,
                       TA_PROP_INCREASE_X_HEIGHT_MIN, 0x7FFF);
  if (!ns)
    goto Fail;

  DUMPSTR("x-height-snapping-exceptions", ns);

  ds = TA_deltas_show(font);
  if (!ds)
    goto Fail;

  /* show delta exceptions data line by line */
  if (!format)
  {
    eol = "";
    prev_eol = "; \\\n";
  }

  if (*ds)
  {
    char* token;
    char* saveptr;


    token = strtok_r(ds, "\n", &saveptr);
    DUMPSTR("delta exceptions", token);

    for (;;)
    {
      token = strtok_r(NULL, "\n", &saveptr);
      if (!token)
        break;

      DUMPSTRX(token);
    }
  }
  else
    DUMPSTR("delta exceptions", "");

  if (!format)
  {
    list = list_prepend(list, "\n");
    if (!list)
      goto Fail;
  }

  list = list_prepend(list, "\n");
  if (!list)
    goto Fail;

Exit:
  list = list_reverse(list);
  buf = list_concat(list);

Fail:
  free(ns);
  free(ds);
  list_free(list);

  return buf;
}

/* end of tadump.c */
