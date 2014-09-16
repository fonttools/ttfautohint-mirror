/* tadeltas.c */

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

#include <locale.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h> /* for llrb.h */

#include "llrb.h" /* a red-black tree implementation */
#include "tadeltas-bison.h"

Deltas*
TA_deltas_new(long font_idx,
              long glyph_idx,
              number_range* point_set,
              double x_shift,
              double y_shift,
              number_range* ppem_set)
{
  Deltas* deltas;


  deltas = (Deltas*)malloc(sizeof (Deltas));
  if (!deltas)
    return NULL;

  deltas->font_idx = font_idx;
  deltas->glyph_idx = glyph_idx;
  deltas->points = number_set_reverse(point_set);

  /* we round shift values to multiples of 1/(2^DELTA_SHIFT) */
  deltas->x_shift = (char)(x_shift * DELTA_FACTOR
                           + (x_shift > 0 ? 0.5 : -0.5));
  deltas->y_shift = (char)(y_shift * DELTA_FACTOR
                           + (y_shift > 0 ? 0.5 : -0.5));

  deltas->ppems = number_set_reverse(ppem_set);
  deltas->next = NULL;

  return deltas;
}


Deltas*
TA_deltas_prepend(Deltas* list,
                  Deltas* element)
{
  if (!element)
    return list;

  element->next = list;

  return element;
}


Deltas*
TA_deltas_reverse(Deltas* list)
{
  Deltas* cur;


  cur = list;
  list = NULL;

  while (cur)
  {
    Deltas* tmp;


    tmp = cur;
    cur = cur->next;
    tmp->next = list;
    list = tmp;
  }

  return list;
}


void
TA_deltas_free(Deltas* deltas)
{
  while (deltas)
  {
    Deltas* tmp;


    number_set_free(deltas->points);
    number_set_free(deltas->ppems);

    tmp = deltas;
    deltas = deltas->next;
    free(tmp);
  }
}


sds
deltas_show_line(FONT* font,
                 Deltas* deltas)
{
  char glyph_name_buf[64];
  char* points_buf = NULL;
  char* ppems_buf = NULL;

  sds s;

  FT_Face face;


  s = sdsempty();
  if (!s)
    return NULL;

  if (!deltas)
    goto Exit;

  if (deltas->font_idx >= font->num_sfnts)
    goto Exit;

  face = font->sfnts[deltas->font_idx].face;
  glyph_name_buf[0] = '\0';
  if (FT_HAS_GLYPH_NAMES(face))
    FT_Get_Glyph_Name(face, deltas->glyph_idx, glyph_name_buf, 64);

  points_buf = number_set_show(deltas->points, -1, -1);
  if (!points_buf)
    goto Exit;
  ppems_buf = number_set_show(deltas->ppems, -1, -1);
  if (!ppems_buf)
    goto Exit;

  /* display glyph index if we don't have a glyph name */
  if (*glyph_name_buf)
    s = sdscatprintf(s, "%ld %s p %s x %.20g y %.20g @ %s",
                     deltas->font_idx,
                     glyph_name_buf,
                     points_buf,
                     (double)deltas->x_shift / DELTA_FACTOR,
                     (double)deltas->y_shift / DELTA_FACTOR,
                     ppems_buf);
  else
    s = sdscatprintf(s, "%ld %ld p %s x %.20g y %.20g @ %s",
                     deltas->font_idx,
                     deltas->glyph_idx,
                     points_buf,
                     (double)deltas->x_shift / DELTA_FACTOR,
                     (double)deltas->y_shift / DELTA_FACTOR,
                     ppems_buf);

Exit:
  free(points_buf);
  free(ppems_buf);

  return s;
}


char*
TA_deltas_show(FONT* font)
{
  sds s;
  size_t len;
  char* res;

  Deltas* deltas = font->deltas;


  s = sdsempty();
  if (!s)
    return NULL;

  while (deltas)
  {
    sds d;


    /* append current line to buffer, followed by a newline character */
    d = deltas_show_line(font, deltas);
    if (!d)
    {
      sdsfree(s);
      return NULL;
    }
    s = sdscatsds(s, d);
    sdsfree(d);
    if (!s)
      return NULL;
    s = sdscat(s, "\n");
    if (!s)
      return NULL;

    deltas = deltas->next;
  }

  /* we return an empty string if there is no data */
  len = sdslen(s) + 1;
  res = (char*)malloc(len);
  if (res)
    memcpy(res, s, len);

  sdsfree(s);

  return res;
}


/* Parse delta exceptions in `font->deltas_buf'. */

TA_Error
TA_deltas_parse_buffer(FONT* font,
                       char** error_string_p,
                       unsigned int* errlinenum_p,
                       char** errline_p,
                       char** errpos_p)
{
  int bison_error;

  Deltas_Context context;


  /* nothing to do if no data */
  if (!font->deltas_buf)
  {
    font->deltas = NULL;
    return TA_Err_Ok;
  }

  TA_deltas_scanner_init(&context, font);
  if (context.error)
    goto Fail;
  /* this is `yyparse' in disguise */
  bison_error = TA_deltas_parse(&context);
  TA_deltas_scanner_done(&context);

  if (bison_error)
  {
    if (bison_error == 2)
      context.error = TA_Err_Deltas_Allocation_Error;

Fail:
    font->deltas = NULL;

    if (context.error == TA_Err_Deltas_Allocation_Error
        || context.error == TA_Err_Deltas_Flex_Error)
    {
      *errlinenum_p = 0;
      *errline_p = NULL;
      *errpos_p = NULL;
      if (context.errmsg)
        *error_string_p = strdup(context.errmsg);
      else
        *error_string_p = strdup(TA_get_error_message(context.error));
    }
    else
    {
      int i, ret;
      char auxbuf[128];

      char* buf_end;
      char* p_start;
      char* p_end;


      /* construct data for `errline_p' */
      buf_end = font->deltas_buf + font->deltas_len;

      p_start = font->deltas_buf;
      if (context.errline_num > 1)
      {
        i = 1;
        while (p_start < buf_end)
        {
          if (*p_start++ == '\n')
          {
            i++;
            if (i == context.errline_num)
              break;
          }
        }
      }

      p_end = p_start;
      while (p_end < buf_end)
      {
        if (*p_end == '\n')
          break;
        p_end++;
      }
      *errline_p = strndup(p_start, p_end - p_start);

      /* construct data for `error_string_p' */
      if (context.error == TA_Err_Deltas_Invalid_Font_Index)
        sprintf(auxbuf, " (valid range is [%ld;%ld])",
                0L,
                font->num_sfnts);
      else if (context.error == TA_Err_Deltas_Invalid_Glyph_Index)
        sprintf(auxbuf, " (valid range is [%ld;%ld])",
                0L,
                font->sfnts[context.font_idx].face->num_glyphs);
      else if (context.error == TA_Err_Deltas_Invalid_Shift)
        sprintf(auxbuf, " (valid interval is [%g;%g])",
                DELTA_SHIFT_MIN,
                DELTA_SHIFT_MAX);
      else if (context.error == TA_Err_Deltas_Invalid_Range)
        sprintf(auxbuf, " (values must be within [%ld;%ld])",
                context.number_set_min,
                context.number_set_max);
      else
        auxbuf[0] = '\0';

      ret = asprintf(error_string_p, "%s%s",
                     *context.errmsg ? context.errmsg
                                     : TA_get_error_message(context.error),
                     auxbuf);
      if (ret == -1)
        *error_string_p = NULL;

      if (errline_p)
        *errpos_p = *errline_p + context.errline_pos_left - 1;
      else
        *errpos_p = NULL;

      *errlinenum_p = context.errline_num;
    }
  }
  else
    font->deltas = context.result;

  return context.error;
}


/* node structure for delta exception data */

typedef struct Node Node;
struct Node
{
  LLRB_ENTRY(Node) entry;
  Delta delta;
};


/* comparison function for our red-black tree */

static int
nodecmp(Node* e1,
        Node* e2)
{
  long diff;


  /* sort by font index ... */
  diff = e1->delta.font_idx - e2->delta.font_idx;
  if (diff)
    goto Exit;

  /* ... then by glyph index ... */
  diff = e1->delta.glyph_idx - e2->delta.glyph_idx;
  if (diff)
    goto Exit;

  /* ... then by ppem ... */
  diff = e1->delta.ppem - e2->delta.ppem;
  if (diff)
    goto Exit;

  /* ... then by point index */
  diff = e1->delta.point_idx - e2->delta.point_idx;

Exit:
  /* https://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign */
  return (diff > 0) - (diff < 0);
}


/* the red-black tree function body */
typedef struct deltas_data deltas_data;

LLRB_HEAD(deltas_data, Node);

/* no trailing semicolon in the next line */
LLRB_GENERATE_STATIC(deltas_data, Node, entry, nodecmp)


void
TA_deltas_free_tree(FONT* font)
{
  deltas_data* deltas_data_head = (deltas_data*)font->deltas_data_head;

  Node* node;
  Node* next_node;


  if (!deltas_data_head)
    return;

  for (node = LLRB_MIN(deltas_data, deltas_data_head);
       node;
       node = next_node)
  {
    next_node = LLRB_NEXT(deltas_data, deltas_data_head, node);
    LLRB_REMOVE(deltas_data, deltas_data_head, node);
    free(node);
  }

  free(deltas_data_head);
}


TA_Error
TA_deltas_build_tree(FONT* font)
{
  Deltas* deltas = font->deltas;
  deltas_data* deltas_data_head;
  int emit_newline = 0;


  /* nothing to do if no data */
  if (!deltas)
  {
    font->deltas_data_head = NULL;
    return TA_Err_Ok;
  }

  deltas_data_head = (deltas_data*)malloc(sizeof (deltas_data));
  if (!deltas_data_head)
    return FT_Err_Out_Of_Memory;

  LLRB_INIT(deltas_data_head);

  while (deltas)
  {
    long font_idx = deltas->font_idx;
    long glyph_idx = deltas->glyph_idx;
    char x_shift = deltas->x_shift;
    char y_shift = deltas->y_shift;

    number_set_iter ppems_iter;
    int ppem;


    ppems_iter.range = deltas->ppems;
    ppem = number_set_get_first(&ppems_iter);

    while (ppems_iter.range)
    {
      number_set_iter points_iter;
      int point_idx;


      points_iter.range = deltas->points;
      point_idx = number_set_get_first(&points_iter);

      while (points_iter.range)
      {
        Node* node;
        Node* val;


        node = (Node*)malloc(sizeof (Node));
        if (!node)
          return FT_Err_Out_Of_Memory;

        node->delta.font_idx = font_idx;
        node->delta.glyph_idx = glyph_idx;
        node->delta.ppem = ppem;
        node->delta.point_idx = point_idx;
        node->delta.x_shift = x_shift;
        node->delta.y_shift = y_shift;

        val = LLRB_INSERT(deltas_data, deltas_data_head, node);
        if (val)
          free(node);
        if (val && font->debug)
        {
          /* entry is already present; we ignore it */
          Deltas d;
          number_range ppems;
          number_range points;

          sds s;


          /* construct Deltas entry for debugging output */
          ppems.start = ppem;
          ppems.end = ppem;
          ppems.next = NULL;
          points.start = point_idx;
          points.end = point_idx;
          points.next = NULL;

          d.font_idx = font_idx;
          d.glyph_idx = glyph_idx;
          d.points = &points;
          d.x_shift = x_shift;
          d.y_shift = y_shift;
          d.ppems = &ppems;
          d.next = NULL;

          s = deltas_show_line(font, &d);
          if (s)
          {
            fprintf(stderr, "Delta exception %s ignored.\n", s);
            sdsfree(s);
          }

          emit_newline = 1;
        }

        point_idx = number_set_get_next(&points_iter);
      }

      ppem = number_set_get_next(&ppems_iter);
    }

    deltas = deltas->next;
  }

  if (font->debug && emit_newline)
    fprintf(stderr, "\n");

  font->deltas_data_head = deltas_data_head;
  font->deltas_data_cur = LLRB_MIN(deltas_data, deltas_data_head);

  return TA_Err_Ok;
}


/* the next functions are intended to restrict the use of LLRB stuff */
/* related to delta exceptions to this file, */
/* providing a means to access the data sequentially */

void
TA_deltas_get_next(FONT* font)
{
  Node* node = (Node*)font->deltas_data_cur;


  if (!node)
    return;

  node = LLRB_NEXT(deltas_data, /* unused */, node);

  font->deltas_data_cur = node;
}


const Delta*
TA_deltas_get_delta(FONT* font)
{
  Node* node = (Node*)font->deltas_data_cur;


  return node ? &node->delta : NULL;
}

/* end of tadeltas.c */
