/* tacontrol.c */

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
#include "tashaper.h"

#include <locale.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h> /* for llrb.h */

#include "llrb.h" /* a red-black tree implementation */
#include "tacontrol-bison.h"


Control*
TA_control_new(Control_Type type,
               long font_idx,
               long glyph_idx,
               number_range* point_set,
               double x_shift,
               double y_shift,
               number_range* ppem_set,
               int line_number)
{
  Control* control;


  control = (Control*)malloc(sizeof (Control));
  if (!control)
    return NULL;

  control->type = type;
  control->font_idx = font_idx;
  control->glyph_idx = glyph_idx;
  control->points = number_set_reverse(point_set);

  switch (control->type)
  {
  case Control_Delta_before_IUP:
  case Control_Delta_after_IUP:
    /* we round shift values to multiples of 1/(2^CONTROL_DELTA_SHIFT) */
    control->x_shift = (int)(x_shift * CONTROL_DELTA_FACTOR
                             + (x_shift > 0 ? 0.5 : -0.5));
    control->y_shift = (int)(y_shift * CONTROL_DELTA_FACTOR
                             + (y_shift > 0 ? 0.5 : -0.5));
    break;

  case Control_Single_Point_Segment_Left:
  case Control_Single_Point_Segment_Right:
    /* offsets */
    control->x_shift = (int)x_shift;
    control->y_shift = (int)y_shift;
    break;

  case Control_Single_Point_Segment_None:
    control->x_shift = 0;
    control->y_shift = 0;
    break;

  case Control_Script_Feature_Glyphs:
    /* the `glyph_idx' field holds the style; */
    /* the `points' field holds the glyph index set */
    break;

  case Control_Script_Feature_Widths:
    /* the `glyph_idx' field holds the style */
    /* (or a feature for all scripts); */
    /* the `points' field holds the width set; */
    break;
  }

  control->ppems = number_set_reverse(ppem_set);
  control->next = NULL;

  control->line_number = line_number;

  return control;
}


Control*
TA_control_prepend(Control* list,
                   Control* element)
{
  if (!element)
    return list;

  element->next = list;

  return element;
}


Control*
TA_control_reverse(Control* list)
{
  Control* cur;


  cur = list;
  list = NULL;

  while (cur)
  {
    Control* tmp;


    tmp = cur;
    cur = cur->next;
    tmp->next = list;
    list = tmp;
  }

  return list;
}


void
TA_control_free(Control* control)
{
  while (control)
  {
    Control* tmp;


    number_set_free(control->points);
    number_set_free(control->ppems);

    tmp = control;
    control = control->next;
    free(tmp);
  }
}


static sds
control_show_line(FONT* font,
                  Control* control)
{
  char glyph_name_buf[64];
  char* points_buf = NULL;
  char* ppems_buf = NULL;

  sds s;

  FT_Face face;


  s = sdsempty();

  if (!control)
    goto Exit;

  if (control->font_idx >= font->num_sfnts)
    goto Exit;

  face = font->sfnts[control->font_idx].face;
  glyph_name_buf[0] = '\0';
  if (!(control->type == Control_Script_Feature_Glyphs
        || control->type == Control_Script_Feature_Widths)
      && FT_HAS_GLYPH_NAMES(face))
    FT_Get_Glyph_Name(face, (FT_UInt)control->glyph_idx, glyph_name_buf, 64);

  points_buf = number_set_show(control->points, -1, -1);
  if (!points_buf)
    goto Exit;
  ppems_buf = number_set_show(control->ppems, -1, -1);
  if (!ppems_buf)
    goto Exit;

  switch (control->type)
  {
  case Control_Delta_before_IUP:
  case Control_Delta_after_IUP:
    /* display glyph index if we don't have a glyph name */
    if (*glyph_name_buf)
      s = sdscatprintf(s, "%ld %s %s %s xshift %.20g yshift %.20g @ %s",
                       control->font_idx,
                       glyph_name_buf,
                       control->type == Control_Delta_before_IUP ? "touch"
                                                                 : "point",
                       points_buf,
                       (double)control->x_shift / CONTROL_DELTA_FACTOR,
                       (double)control->y_shift / CONTROL_DELTA_FACTOR,
                       ppems_buf);
    else
      s = sdscatprintf(s, "%ld %ld point %s xshift %.20g yshift %.20g @ %s",
                       control->font_idx,
                       control->glyph_idx,
                       points_buf,
                       (double)control->x_shift / CONTROL_DELTA_FACTOR,
                       (double)control->y_shift / CONTROL_DELTA_FACTOR,
                       ppems_buf);
    break;

  case Control_Single_Point_Segment_Left:
  case Control_Single_Point_Segment_Right:
    /* display glyph index if we don't have a glyph name */
    if (*glyph_name_buf)
      s = sdscatprintf(s, "%ld %s %s %s",
                       control->font_idx,
                       glyph_name_buf,
                       control->type == Control_Single_Point_Segment_Left
                         ? "left" : "right",
                       points_buf);
    else
      s = sdscatprintf(s, "%ld %ld %s %s",
                       control->font_idx,
                       control->glyph_idx,
                       control->type == Control_Single_Point_Segment_Left
                         ? "left" : "right",
                       points_buf);

    if (control->x_shift || control->y_shift)
      s = sdscatprintf(s, " (%d,%d)", control->x_shift, control->y_shift);
    break;

  case Control_Single_Point_Segment_None:
    /* display glyph index if we don't have a glyph name */
    if (*glyph_name_buf)
      s = sdscatprintf(s, "%ld %s nodir %s",
                       control->font_idx,
                       glyph_name_buf,
                       points_buf);
    else
      s = sdscatprintf(s, "%ld %ld nodir %s",
                       control->font_idx,
                       control->glyph_idx,
                       points_buf);
    break;

  case Control_Script_Feature_Glyphs:
    {
      TA_StyleClass style_class = ta_style_classes[control->glyph_idx];
      char feature_name[5];


      feature_name[4] = '\0';
      hb_tag_to_string(feature_tags[style_class->coverage], feature_name);

      s = sdscatprintf(s, "%ld %s %s @ %s",
                       control->font_idx,
                       script_names[style_class->script],
                       feature_name,
                       points_buf);
    }
    break;

  case Control_Script_Feature_Widths:
    {
      char feature_name[5];
      const char* script_name;


      feature_name[4] = '\0';

      if (control->glyph_idx > 0)
      {
        TA_StyleClass  style_class = ta_style_classes[control->glyph_idx];


        script_name = script_names[style_class->script];
        hb_tag_to_string(feature_tags[style_class->coverage], feature_name);
      }
      else
      {
        script_name = "*";
        hb_tag_to_string(feature_tags[-control->glyph_idx], feature_name);
      }

      s = sdscatprintf(s, "%ld %s %s width %s",
                       control->font_idx,
                       script_name,
                       feature_name,
                       points_buf);
    }
  }

Exit:
  free(points_buf);
  free(ppems_buf);

  return s;
}


char*
TA_control_show(FONT* font)
{
  sds s;
  size_t len;
  char* res;

  Control* control = font->control;


  s = sdsempty();

  while (control)
  {
    sds d;


    /* append current line to buffer, followed by a newline character */
    d = control_show_line(font, control);
    if (!d)
    {
      sdsfree(s);
      return NULL;
    }
    s = sdscatsds(s, d);
    sdsfree(d);
    s = sdscat(s, "\n");

    control = control->next;
  }

  if (!s)
    return NULL;

  /* we return an empty string if there is no data */
  len = sdslen(s) + 1;
  res = (char*)malloc(len);
  if (res)
    memcpy(res, s, len);

  sdsfree(s);

  return res;
}


/* Parse control instructions in `font->control_buf'. */

TA_Error
TA_control_parse_buffer(FONT* font,
                        char** error_string_p,
                        unsigned int* errlinenum_p,
                        char** errline_p,
                        char** errpos_p)
{
  int bison_error;

  Control_Context context;


  /* nothing to do if no data */
  if (!font->control_buf)
  {
    font->control = NULL;
    return TA_Err_Ok;
  }

  TA_control_scanner_init(&context, font);
  if (context.error)
    goto Fail;
  /* this is `yyparse' in disguise */
  bison_error = TA_control_parse(&context);
  TA_control_scanner_done(&context);

  if (bison_error)
  {
    if (bison_error == 2)
      context.error = TA_Err_Control_Allocation_Error;

Fail:
    font->control = NULL;

    if (context.error == TA_Err_Control_Allocation_Error
        || context.error == TA_Err_Control_Flex_Error)
    {
      *errlinenum_p = 0;
      *errline_p = NULL;
      *errpos_p = NULL;
      if (*context.errmsg)
        *error_string_p = strdup(context.errmsg);
      else
        *error_string_p = strdup(TA_get_error_message(context.error));
    }
    else
    {
      int ret;
      char auxbuf[128];

      char* buf_end;
      char* p_start;
      char* p_end;


      /* construct data for `errline_p' */
      buf_end = font->control_buf + font->control_len;

      p_start = font->control_buf;
      if (context.errline_num > 1)
      {
        int i = 1;
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
      *errline_p = strndup(p_start, (size_t)(p_end - p_start));

      /* construct data for `error_string_p' */
      if (context.error == TA_Err_Control_Invalid_Font_Index)
        sprintf(auxbuf, " (valid range is [%ld;%ld])",
                0L,
                font->num_sfnts);
      else if (context.error == TA_Err_Control_Invalid_Glyph_Index)
        sprintf(auxbuf, " (valid range is [%ld;%ld])",
                0L,
                font->sfnts[context.font_idx].face->num_glyphs);
      else if (context.error == TA_Err_Control_Invalid_Shift)
        sprintf(auxbuf, " (valid interval is [%g;%g])",
                CONTROL_DELTA_SHIFT_MIN,
                CONTROL_DELTA_SHIFT_MAX);
      else if (context.error == TA_Err_Control_Invalid_Offset)
        sprintf(auxbuf, " (valid interval is [%d;%d])",
                SHRT_MIN,
                SHRT_MAX);
      else if (context.error == TA_Err_Control_Invalid_Range)
        sprintf(auxbuf, " (values must be within [%d;%d])",
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

      if (*errline_p)
        *errpos_p = *errline_p + context.errline_pos_left - 1;
      else
        *errpos_p = NULL;

      *errlinenum_p = (unsigned int)context.errline_num;
    }
  }
  else
    font->control = context.result;

  return context.error;
}


/* apply coverage data from the control instructions file */

void
TA_control_apply_coverage(SFNT* sfnt,
                          FONT* font)
{
  Control* control = font->control;
  TA_FaceGlobals globals = (TA_FaceGlobals)sfnt->face->autohint.data;
  FT_UShort* gstyles = globals->glyph_styles;


  while (control)
  {
    number_set_iter glyph_idx_iter;
    int glyph_idx;
    int style;


    if (control->type != Control_Script_Feature_Glyphs)
      goto Skip;
    if (control->font_idx != sfnt->face->face_index)
      goto Skip;

    /* `control->glyph_idx' holds the style; */
    /* `control->points' holds the glyph index set */
    style = (int)control->glyph_idx;
    glyph_idx_iter.range = control->points;
    glyph_idx = number_set_get_first(&glyph_idx_iter);

    while (glyph_idx >= 0)
    {
      /* assign new style but retain digit property */
      gstyles[glyph_idx] &= TA_DIGIT;
      gstyles[glyph_idx] |= style;

      glyph_idx = number_set_get_next(&glyph_idx_iter);
    }

  Skip:
    control = control->next;
  }
}


/* handle stem width data from the control instructions file; */

void
TA_control_set_stem_widths(TA_LatinMetrics metrics,
                           FONT* font)
{
  Control* control = font->control;
  FT_Face face = font->loader->face;
  TA_WidthRec* widths = metrics->axis[TA_DIMENSION_VERT].widths;


  while (control)
  {
    TA_StyleClass style_class = metrics->root.style_class;

    number_set_iter width_iter;
    int width;
    int i;


    if (control->type != Control_Script_Feature_Widths)
      goto Skip;
    if (control->font_idx != face->face_index)
      goto Skip;
    /* `control->glyph_idx' holds either a style */
    /* or a feature for all scripts */
    if (!(control->glyph_idx == style_class->style
          || -control->glyph_idx == style_class->coverage))
      goto Skip;

    /* `control->points' holds the width set */
    width_iter.range = control->points;
    width = number_set_get_first(&width_iter);

    i = 0;
    while (width >= 0)
    {
      widths[i++].org = width;
      width = number_set_get_next(&width_iter);
    }
    metrics->axis[TA_DIMENSION_VERT].width_count = i;

  Skip:
    control = control->next;
  }
}


/* node structure for control instruction data */

typedef struct Node Node;
struct Node
{
  LLRB_ENTRY(Node) entry;
  Ctrl ctrl;
};


/* comparison function for our red-black tree */

static int
nodecmp(Node* e1,
        Node* e2)
{
  long diff;


  /* sort by font index ... */
  diff = e1->ctrl.font_idx - e2->ctrl.font_idx;
  if (diff)
    goto Exit;

  /* ... then by glyph index ... */
  diff = e1->ctrl.glyph_idx - e2->ctrl.glyph_idx;
  if (diff)
    goto Exit;

  /* ... then by ppem ... */
  diff = e1->ctrl.ppem - e2->ctrl.ppem;
  if (diff)
    goto Exit;

  /* ... then by point index */
  diff = e1->ctrl.point_idx - e2->ctrl.point_idx;

Exit:
  /* https://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign */
  return (diff > 0) - (diff < 0);
}


/* the red-black tree function body */
typedef struct control_data control_data;

LLRB_HEAD(control_data, Node);

/* no trailing semicolon in the next line */
LLRB_GENERATE_STATIC(control_data, Node, entry, nodecmp)


void
TA_control_free_tree(FONT* font)
{
  control_data* control_data_head = (control_data*)font->control_data_head;
  Control* control_segment_dirs_head = (Control*)font->control_segment_dirs_head;

  Node* node;
  Node* next_node;


  if (!control_data_head)
    return;

  for (node = LLRB_MIN(control_data, control_data_head);
       node;
       node = next_node)
  {
    next_node = LLRB_NEXT(control_data, control_data_head, node);
    LLRB_REMOVE(control_data, control_data_head, node);
    free(node);
  }

  free(control_data_head);
  TA_control_free(control_segment_dirs_head);
}


TA_Error
TA_control_build_tree(FONT* font)
{
  Control* control = font->control;
  control_data* control_data_head;
  int emit_newline = 0;


  font->control_segment_dirs_head = NULL;
  font->control_segment_dirs_cur = NULL;

  /* nothing to do if no data */
  if (!control)
  {
    font->control_data_head = NULL;
    font->control_data_cur = NULL;
    return TA_Err_Ok;
  }

  control_data_head = (control_data*)malloc(sizeof (control_data));
  if (!control_data_head)
    return FT_Err_Out_Of_Memory;

  LLRB_INIT(control_data_head);

  while (control)
  {
    Control_Type type = control->type;
    long font_idx = control->font_idx;
    long glyph_idx = control->glyph_idx;
    int x_shift = control->x_shift;
    int y_shift = control->y_shift;
    int line_number = control->line_number;

    number_set_iter ppems_iter;
    int ppem;


    /* we don't store style information in the tree */
    if (type == Control_Script_Feature_Glyphs)
    {
      control = control->next;
      continue;
    }

    ppems_iter.range = control->ppems;
    ppem = number_set_get_first(&ppems_iter);

    /* ppem is always zero for one-point segments */
    if (type == Control_Single_Point_Segment_Left
        || type == Control_Single_Point_Segment_Right
        || type == Control_Single_Point_Segment_None)
      goto Points_Loop;

    while (ppem >= 0)
    {
      number_set_iter points_iter;
      int point_idx;


    Points_Loop:
      points_iter.range = control->points;
      point_idx = number_set_get_first(&points_iter);

      while (point_idx >= 0)
      {
        Node* node;
        Node* val;


        node = (Node*)malloc(sizeof (Node));
        if (!node)
          return FT_Err_Out_Of_Memory;

        node->ctrl.type = type;
        node->ctrl.font_idx = font_idx;
        node->ctrl.glyph_idx = glyph_idx;
        node->ctrl.ppem = ppem;
        node->ctrl.point_idx = point_idx;
        node->ctrl.x_shift = x_shift;
        node->ctrl.y_shift = y_shift;
        node->ctrl.line_number = line_number;

        val = LLRB_INSERT(control_data, control_data_head, node);
        if (val && font->debug)
        {
          /* entry is already present; we overwrite it */
          Control d;
          number_range ppems;
          number_range points;

          sds s;


          /* construct Control entry for debugging output */
          ppems.start = ppem;
          ppems.end = ppem;
          ppems.next = NULL;
          points.start = point_idx;
          points.end = point_idx;
          points.next = NULL;

          d.type = type;
          d.font_idx = font_idx;
          d.glyph_idx = glyph_idx;
          d.points = &points;
          d.x_shift = x_shift;
          d.y_shift = y_shift;
          d.ppems = &ppems;
          d.next = NULL;

          s = control_show_line(font, &d);
          if (s)
          {
            fprintf(stderr, "Control instruction `%s' (line %d)"
                            " overwrites data from line %d.\n",
                            s, line_number, val->ctrl.line_number);
            sdsfree(s);
          }

          emit_newline = 1;
        }
        if (val)
        {
          val->ctrl.type = type;
          val->ctrl.font_idx = font_idx;
          val->ctrl.glyph_idx = glyph_idx;
          val->ctrl.ppem = ppem;
          val->ctrl.point_idx = point_idx;
          val->ctrl.x_shift = x_shift;
          val->ctrl.y_shift = y_shift;
          val->ctrl.line_number = line_number;

          free(node);
        }

        point_idx = number_set_get_next(&points_iter);
      }

      ppem = number_set_get_next(&ppems_iter);
    }

    control = control->next;
  }

  if (font->debug && emit_newline)
    fprintf(stderr, "\n");

  font->control_data_head = control_data_head;
  font->control_data_cur = LLRB_MIN(control_data, control_data_head);

  return TA_Err_Ok;
}


/* the next functions are intended to restrict the use of LLRB stuff */
/* related to control instructions to this file, */
/* providing a means to access the data sequentially */

void
TA_control_get_next(FONT* font)
{
  Node* node = (Node*)font->control_data_cur;


  if (!node)
    return;

  node = LLRB_NEXT(control_data, /* unused */, node);

  font->control_data_cur = node;
}


const Ctrl*
TA_control_get_ctrl(FONT* font)
{
  Node* node = (Node*)font->control_data_cur;


  return node ? &node->ctrl : NULL;
}


TA_Error
TA_control_segment_dir_collect(FONT* font,
                               long font_idx,
                               long glyph_idx)
{
  Control* control_segment_dirs_head = (Control*)font->control_segment_dirs_head;


  /* nothing to do if no data */
  if (!font->control_data_head)
    return TA_Err_Ok;

  if (control_segment_dirs_head)
  {
    TA_control_free(control_segment_dirs_head);
    control_segment_dirs_head = NULL;
  }

  /*
   * The PPEM value for one-point segments is always zero; such control
   * instructions are thus sorted before other control instructions for the
   * same glyph index -- this fits nicely with the call to
   * `TA_control_get_next' in the loop of `TA_sfnt_build_delta_exceptions',
   * assuring proper sequential access to the red-black tree.
   */
  for (;;)
  {
    const Ctrl* ctrl = TA_control_get_ctrl(font);
    Control* elem;


    if (!ctrl)
      break;

    /* check type */
    if (!(ctrl->type == Control_Single_Point_Segment_Left
          || ctrl->type == Control_Single_Point_Segment_Right
          || ctrl->type == Control_Single_Point_Segment_None))
      break;

    /* too large values of font and glyph indices in `ctrl' */
    /* are handled by later calls of this function */
    if (font_idx < ctrl->font_idx
        || glyph_idx < ctrl->glyph_idx)
      break;

    /* we simply use the `Control' structure again, */
    /* abusing the `glyph_idx' field for the point index */
    elem = TA_control_new(ctrl->type,
                          0,
                          ctrl->point_idx,
                          NULL,
                          ctrl->x_shift,
                          ctrl->y_shift,
                          NULL,
                          ctrl->line_number);
    if (!elem)
    {
      TA_control_free(control_segment_dirs_head);
      return TA_Err_Control_Allocation_Error;
    }
    control_segment_dirs_head = TA_control_prepend(control_segment_dirs_head,
                                                   elem);

    TA_control_get_next(font);
  }

  font->control_segment_dirs_head = TA_control_reverse(control_segment_dirs_head);
  font->control_segment_dirs_cur = font->control_segment_dirs_head;

  return TA_Err_Ok;
}


int
TA_control_segment_dir_get_next(FONT* font,
                                int* point_idx,
                                TA_Direction* dir,
                                int* left_offset,
                                int* right_offset)
{
  Control* control_segment_dirs_head = (Control*)font->control_segment_dirs_head;
  Control* control_segment_dirs_cur = (Control*)font->control_segment_dirs_cur;


  /* nothing to do if no data */
  if (!control_segment_dirs_head)
    return 0;

  if (!control_segment_dirs_cur)
  {
    font->control_segment_dirs_cur = control_segment_dirs_head;
    return 0;
  }

  /* the `glyph_idx' field gets abused for `point_idx' */
  *point_idx = (int)control_segment_dirs_cur->glyph_idx;
  *dir = control_segment_dirs_cur->type == Control_Single_Point_Segment_Left
           ? TA_DIR_LEFT
           : control_segment_dirs_cur->type == Control_Single_Point_Segment_Right
             ? TA_DIR_RIGHT
             : TA_DIR_NONE;
  *left_offset = control_segment_dirs_cur->x_shift;
  *right_offset = control_segment_dirs_cur->y_shift;

  font->control_segment_dirs_cur = control_segment_dirs_cur->next;

  return 1;
}

/* end of tacontrol.c */
