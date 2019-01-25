/* tahints.c */

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


/* originally file `afhints.c' (2011-Mar-28) from FreeType */

/* heavily modified 2011 by Werner Lemberg <wl@gnu.org> */

#include "ta.h"

#include <string.h>
#include <stdlib.h>
#include "tahints.h"


/* get new segment for given axis */

FT_Error
ta_axis_hints_new_segment(TA_AxisHints axis,
                          TA_Segment* asegment)
{
  FT_Error error = FT_Err_Ok;
  TA_Segment segment = NULL;


  if (axis->num_segments < TA_SEGMENTS_EMBEDDED)
  {
    if (!axis->segments)
    {
      axis->segments = axis->embedded.segments;
      axis->max_segments = TA_SEGMENTS_EMBEDDED;
    }
  }
  else if (axis->num_segments >= axis->max_segments)
  {
    TA_Segment segments_new;

    FT_Int old_max = axis->max_segments;
    FT_Int new_max = old_max;
    FT_Int big_max = (FT_Int)(FT_INT_MAX / sizeof (*segment));


    if (old_max >= big_max)
    {
      error = FT_Err_Out_Of_Memory;
      goto Exit;
    }

    new_max += (new_max >> 2) + 4;
    if (new_max < old_max
        || new_max > big_max)
      new_max = big_max;

    if (axis->segments == axis->embedded.segments)
    {
      axis->segments = (TA_Segment)malloc(
                         (size_t)new_max * sizeof (TA_SegmentRec));
      if (!axis->segments)
        return FT_Err_Out_Of_Memory;

      memcpy(axis->segments, axis->embedded.segments,
             sizeof (axis->embedded.segments));
    }
    else
    {
      segments_new = (TA_Segment)realloc(
                       axis->segments,
                       (size_t)new_max * sizeof (TA_SegmentRec));
      if (!segments_new)
        return FT_Err_Out_Of_Memory;
      axis->segments = segments_new;
    }

    axis->max_segments = new_max;
  }

  segment = axis->segments + axis->num_segments++;

Exit:
  *asegment = segment;
  return error;
}


/* get new edge for given axis, direction, and position, */
/* without initializing the edge itself */

FT_Error
ta_axis_hints_new_edge(TA_AxisHints axis,
                       FT_Int fpos,
                       TA_Direction dir,
                       FT_Bool top_to_bottom_hinting,
                       TA_Edge* anedge)
{
  FT_Error error = FT_Err_Ok;
  TA_Edge edge = NULL;
  TA_Edge edges;


  if (axis->num_edges < TA_EDGES_EMBEDDED)
  {
    if (!axis->edges)
    {
      axis->edges = axis->embedded.edges;
      axis->max_edges = TA_EDGES_EMBEDDED;
    }
  }
  else if (axis->num_edges >= axis->max_edges)
  {
    TA_Edge edges_new;

    FT_Int old_max = axis->max_edges;
    FT_Int new_max = old_max;
    FT_Int big_max = (FT_Int)(FT_INT_MAX / sizeof (*edge));


    if (old_max >= big_max)
    {
      error = FT_Err_Out_Of_Memory;
      goto Exit;
    }

    new_max += (new_max >> 2) + 4;
    if (new_max < old_max
        || new_max > big_max)
      new_max = big_max;

    if (axis->edges == axis->embedded.edges)
    {
      axis->edges = (TA_Edge)malloc((size_t)new_max * sizeof (TA_EdgeRec));
      if (!axis->edges)
        return FT_Err_Out_Of_Memory;

      memcpy(axis->edges, axis->embedded.edges,
             sizeof (axis->embedded.edges));
    }
    else
    {
      edges_new = (TA_Edge)realloc(axis->edges,
                                   (size_t)new_max * sizeof (TA_EdgeRec));
      if (!edges_new)
        return FT_Err_Out_Of_Memory;
      axis->edges = edges_new;
    }

    axis->max_edges = new_max;
  }

  edges = axis->edges;
  edge = edges + axis->num_edges;

  while (edge > edges)
  {
    if (top_to_bottom_hinting ? (edge[-1].fpos > fpos)
                              : (edge[-1].fpos < fpos))
      break;

    /* we want the edge with same position and minor direction */
    /* to appear before those in the major one in the list */
    if (edge[-1].fpos == fpos
        && dir == axis->major_dir)
      break;

    edge[0] = edge[-1];
    edge--;
  }

  axis->num_edges++;

Exit:
  *anedge = edge;
  return error;
}


#ifdef TA_DEBUG

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


void
_ta_message(const char* format,
            ...)
{
  va_list ap;


  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}


static const char*
ta_dir_str(TA_Direction dir)
{
  const char* result;


  switch (dir)
  {
  case TA_DIR_UP:
    result = "up";
    break;
  case TA_DIR_DOWN:
    result = "down";
    break;
  case TA_DIR_LEFT:
    result = "left";
    break;
  case TA_DIR_RIGHT:
    result = "right";
    break;
  default:
    result = "none";
  }

  return result;
}


#define TA_INDEX_NUM(ptr, base) \
          (int)((ptr) ? ((ptr) - (base)) \
                      : -1)


static char*
ta_print_idx(char* p,
             int idx)
{
  if (idx == -1)
  {
    p[0] = '-';
    p[1] = '-';
    p[2] = '\0';
  }
  else
    sprintf(p, "%d", idx);

  return p;
}


static int
ta_get_segment_index(TA_GlyphHints hints,
                     int point_idx,
                     int dimension)
{
  TA_AxisHints axis = &hints->axis[dimension];
  TA_Point point = hints->points + point_idx;
  TA_Segment segments = axis->segments;
  TA_Segment limit = segments + axis->num_segments;
  TA_Segment segment;


  for (segment = segments; segment < limit; segment++)
  {
    if (segment->first <= segment->last)
    {
      if (point >= segment->first && point <= segment->last)
        break;
    }
    else
    {
      TA_Point p = segment->first;


      for (;;)
      {
        if (point == p)
          goto Exit;

        if (p == segment->last)
          break;

        p = p->next;
      }
    }
  }

Exit:
  if (segment == limit)
    return -1;

  return (int)(segment - segments);
}


static int
ta_get_edge_index(TA_GlyphHints hints,
                  int segment_idx,
                  int dimension)
{
  TA_AxisHints axis = &hints->axis[dimension];
  TA_Edge edges = axis->edges;
  TA_Segment segment = axis->segments + segment_idx;


  return segment_idx == -1 ? -1 : TA_INDEX_NUM(segment->edge, edges);
}


static int
ta_get_strong_edge_index(TA_GlyphHints hints,
                         TA_Edge* strong_edges,
                         int dimension)
{
  TA_AxisHints axis = &hints->axis[dimension];
  TA_Edge edges = axis->edges;


  return TA_INDEX_NUM(strong_edges[dimension], edges);
}


void
ta_glyph_hints_dump_points(TA_GlyphHints hints)
{
  TA_Point points = hints->points;
  TA_Point limit = points + hints->num_points;
  TA_Point* contour = hints->contours;
  TA_Point* climit = contour + hints->num_contours;
  TA_Point point;


  TA_LOG(("Table of points:\n"));

  if (hints->num_points)
  {
    TA_LOG(("  index  hedge  hseg  flags"
         /* "  XXXXX  XXXXX XXXXX   XXXX" */
            "  xorg  yorg  xscale  yscale   xfit    yfit "
         /* " XXXXX XXXXX XXXX.XX XXXX.XX XXXX.XX XXXX.XX" */
            "  hbef  haft"));
         /* " XXXXX XXXXX" */
  }
  else
    TA_LOG(("  (none)\n"));

  for (point = points; point < limit; point++)
  {
    int point_idx = TA_INDEX_NUM(point, points);
    int segment_idx_1 = ta_get_segment_index(hints, point_idx, 1);

    char buf1[16], buf2[16];
    char buf5[16], buf6[16];


    /* insert extra newline at the beginning of a contour */
    if (contour < climit && *contour == point)
    {
      TA_LOG(("\n"));
      contour++;
    }

    /* we don't show vertical edges since they are never used */
    TA_LOG(("  %5d  %5s %5s   %4s"
            " %5d %5d %7.2f %7.2f %7.2f %7.2f"
            " %5s %5s\n",
            point_idx,
            ta_print_idx(buf1,
                         ta_get_edge_index(hints, segment_idx_1, 1)),
            ta_print_idx(buf2, segment_idx_1),
            (point->flags & TA_FLAG_WEAK_INTERPOLATION) ? "weak" : " -- ",

            point->fx,
            point->fy,
            point->ox / 64.0,
            point->oy / 64.0,
            point->x / 64.0,
            point->y / 64.0,

            ta_print_idx(buf5, ta_get_strong_edge_index(hints,
                                                        point->before,
                                                        1)),
            ta_print_idx(buf6, ta_get_strong_edge_index(hints,
                                                        point->after,
                                                        1))));
  }
  TA_LOG(("\n"));
}


static const char*
ta_edge_flags_to_string(FT_Byte flags)
{
  static char temp[32];
  int pos = 0;


  if (flags & TA_EDGE_ROUND)
  {
    memcpy(temp + pos, "round", 5);
    pos += 5;
  }
  if (flags & TA_EDGE_SERIF)
  {
    if (pos > 0)
      temp[pos++] = ' ';
    memcpy(temp + pos, "serif", 5);
    pos += 5;
  }
  if (pos == 0)
    return "normal";

  temp[pos] = '\0';

  return temp;
}


/* dump the array of linked segments */

void
ta_glyph_hints_dump_segments(TA_GlyphHints hints)
{
  FT_Int dimension;


  for (dimension = TA_DEBUG_STARTDIM;
       dimension >= TA_DEBUG_ENDDIM;
       dimension--)
  {
    TA_AxisHints axis = &hints->axis[dimension];
    TA_Point points = hints->points;
    TA_Edge edges = axis->edges;
    TA_Segment segments = axis->segments;
    TA_Segment limit = segments + axis->num_segments;
    TA_Segment seg;

    char buf1[16], buf2[16], buf3[16];


    TA_LOG(("Table of %s segments:\n",
            dimension == TA_DIMENSION_HORZ ? "vertical"
                                           : "horizontal"));
    if (axis->num_segments)
    {
      TA_LOG(("  index   pos   delta   dir   from   to "
           /* "  XXXXX  XXXXX  XXXXX  XXXXX  XXXX  XXXX" */
              "  link  serif  edge"
           /* "  XXXX  XXXXX  XXXX" */
              "  height  extra     flags\n"));
           /* "  XXXXXX  XXXXX  XXXXXXXXXXX" */
    }
    else
      TA_LOG(("  (none)\n"));

    for (seg = segments; seg < limit; seg++)
      TA_LOG(("  %5d  %5d  %5d  %5s  %4d  %4d"
              "  %4s  %5s  %4s"
              "  %6d  %5d  %11s\n",
              TA_INDEX_NUM(seg, segments),
              seg->pos,
              seg->delta,
              ta_dir_str((TA_Direction)seg->dir),
              TA_INDEX_NUM(seg->first, points),
              TA_INDEX_NUM(seg->last, points),

              ta_print_idx(buf1, TA_INDEX_NUM(seg->link, segments)),
              ta_print_idx(buf2, TA_INDEX_NUM(seg->serif, segments)),
              ta_print_idx(buf3, TA_INDEX_NUM(seg->edge, edges)),

              seg->height,
              seg->height - (seg->max_coord - seg->min_coord),
              ta_edge_flags_to_string(seg->flags)));
    TA_LOG(("\n"));
  }
}


/* dump the array of linked edges */

void
ta_glyph_hints_dump_edges(TA_GlyphHints hints)
{
  FT_Int dimension;


  for (dimension = TA_DEBUG_STARTDIM;
       dimension >= TA_DEBUG_ENDDIM;
       dimension--)
  {
    TA_AxisHints axis = &hints->axis[dimension];
    TA_Edge edges = axis->edges;
    TA_Edge limit = edges + axis->num_edges;
    TA_Edge edge;

    char buf1[16], buf2[16];


    /* note that TA_DIMENSION_HORZ corresponds to _vertical_ edges */
    /* since they have a constant X coordinate */
    if (dimension == TA_DIMENSION_HORZ)
      TA_LOG(("Table of %s edges (1px=%.2fu, 10u=%.2fpx):\n",
              "vertical",
              65536.0 * 64.0 / hints->x_scale,
              10.0 * hints->x_scale / 65536.0 / 64.0));
    else
      TA_LOG(("Table of %s edges (1px=%.2fu, 10u=%.2fpx):\n",
              "horizontal",
              65536.0 * 64.0 / hints->y_scale,
              10.0 * hints->y_scale / 65536.0 / 64.0));

    if (axis->num_edges)
    {
      TA_LOG(("  index    pos     dir   link  serif"
           /* "  XXXXX  XXXX.XX  XXXXX  XXXX  XXXXX" */
              "  blue    opos     pos       flags\n"));
           /* "    X   XXXX.XX  XXXX.XX  XXXXXXXXXXX" */
    }
    else
      TA_LOG(("  (none)\n"));

    for (edge = edges; edge < limit; edge++)
      TA_LOG(("  %5d  %7.2f  %5s  %4s  %5s"
              "    %c   %7.2f  %7.2f  %11s\n",
              TA_INDEX_NUM(edge, edges),
              (int)edge->opos / 64.0,
              ta_dir_str((TA_Direction)edge->dir),
              ta_print_idx(buf1, TA_INDEX_NUM(edge->link, edges)),
              ta_print_idx(buf2, TA_INDEX_NUM(edge->serif, edges)),

              edge->blue_edge ? 'y' : 'n',
              edge->opos / 64.0,
              edge->pos / 64.0,
              ta_edge_flags_to_string(edge->flags)));
    TA_LOG(("\n"));
  }
}

#endif /* TA_DEBUG */


/* compute the direction value of a given vector */

TA_Direction
ta_direction_compute(FT_Pos dx,
                     FT_Pos dy)
{
  FT_Pos ll, ss; /* long and short arm lengths */
  TA_Direction dir; /* candidate direction */


  if (dy >= dx)
  {
    if (dy >= -dx)
    {
      dir = TA_DIR_UP;
      ll = dy;
      ss = dx;
    }
    else
    {
      dir = TA_DIR_LEFT;
      ll = -dx;
      ss = dy;
    }
  }
  else /* dy < dx */
  {
    if (dy >= -dx)
    {
      dir = TA_DIR_RIGHT;
      ll = dx;
      ss = dy;
    }
    else
    {
      dir = TA_DIR_DOWN;
      ll = -dy;
      ss = dx;
    }
  }

  /* return no direction if arm lengths do not differ enough */
  /* (value 14 is heuristic, corresponding to approx. 4.1 degrees); */
  /* the long arm is never negative */
  if (ll <= 14 * TA_ABS(ss))
    dir = TA_DIR_NONE;

  return dir;
}


void
ta_glyph_hints_init(TA_GlyphHints hints)
{
  /* no need to initialize the embedded items */
  memset(hints, 0, sizeof (*hints) - sizeof (hints->embedded));
}


void
ta_glyph_hints_done(TA_GlyphHints hints)
{
  int dim;


  if (!hints)
    return;

  /* we don't need to free the segment and edge buffers */
  /* since they are really within the hints->points array */
  for (dim = 0; dim < TA_DIMENSION_MAX; dim++)
  {
    TA_AxisHints axis = &hints->axis[dim];


    axis->num_segments = 0;
    axis->max_segments = 0;
    if (axis->segments != axis->embedded.segments)
    {
      free(axis->segments);
      axis->segments = NULL;
    }

    axis->num_edges = 0;
    axis->max_edges = 0;
    if (axis->edges != axis->embedded.edges)
    {
      free(axis->edges);
      axis->edges = NULL;
    }
  }

  if (hints->contours != hints->embedded.contours)
  {
    free(hints->contours);
    hints->contours = NULL;
  }
  hints->max_contours = 0;
  hints->num_contours = 0;

  if (hints->points != hints->embedded.points)
  {
    free(hints->points);
    hints->points = NULL;
  }
  hints->max_points = 0;
  hints->num_points = 0;
}


/* reset metrics */

void
ta_glyph_hints_rescale(TA_GlyphHints hints,
                       TA_StyleMetrics metrics)
{
  hints->metrics = metrics;
  hints->scaler_flags = metrics->scaler.flags;
}


/* from FreeType's ftcalc.c */

static FT_Int
ta_corner_is_flat(FT_Pos in_x,
                  FT_Pos in_y,
                  FT_Pos out_x,
                  FT_Pos out_y)
{
  FT_Pos ax = in_x;
  FT_Pos ay = in_y;

  FT_Pos d_in, d_out, d_corner;


  if (ax < 0)
    ax = -ax;
  if (ay < 0)
    ay = -ay;
  d_in = ax + ay;

  ax = out_x;
  if (ax < 0)
    ax = -ax;
  ay = out_y;
  if (ay < 0)
    ay = -ay;
  d_out = ax + ay;

  ax = out_x + in_x;
  if (ax < 0)
    ax = -ax;
  ay = out_y + in_y;
  if (ay < 0)
    ay = -ay;
  d_corner = ax + ay;

  return (d_in + d_out - d_corner) < (d_corner >> 4);
}


/* recompute all TA_Point in TA_GlyphHints */
/* from the definitions in a source outline */

FT_Error
ta_glyph_hints_reload(TA_GlyphHints hints,
                      FT_Outline* outline)
{
  FT_Error error = FT_Err_Ok;
  TA_Point points;
  FT_UInt old_max, new_max;

  FT_Fixed x_scale = hints->x_scale;
  FT_Fixed y_scale = hints->y_scale;
  FT_Pos x_delta = hints->x_delta;
  FT_Pos y_delta = hints->y_delta;


  hints->num_points = 0;
  hints->num_contours = 0;

  hints->axis[0].num_segments = 0;
  hints->axis[0].num_edges = 0;
  hints->axis[1].num_segments = 0;
  hints->axis[1].num_edges = 0;

  /* first of all, reallocate the contours array if necessary */
  new_max = (FT_UInt)outline->n_contours;
  old_max = (FT_UInt)hints->max_contours;

  if (new_max <= TA_CONTOURS_EMBEDDED)
  {
    if (!hints->contours)
    {
      hints->contours = hints->embedded.contours;
      hints->max_contours = TA_CONTOURS_EMBEDDED;
    }
  }
  else if (new_max > old_max)
  {
    TA_Point* contours_new;


    if (hints->contours == hints->embedded.contours)
      hints->contours = NULL;

    new_max = (new_max + 3) & ~3U; /* round up to a multiple of 4 */

    contours_new = (TA_Point*)realloc(hints->contours,
                                      new_max * sizeof (TA_Point));
    if (!contours_new)
      return FT_Err_Out_Of_Memory;

    hints->contours = contours_new;
    hints->max_contours = (FT_Int)new_max;
  }

  /* reallocate the points arrays if necessary -- we reserve */
  /* two additional point positions, used to hint metrics appropriately */
  new_max = (FT_UInt)(outline->n_points + 2);
  old_max = (FT_UInt)hints->max_points;

  if (new_max <= TA_POINTS_EMBEDDED)
  {
    if (!hints->points)
    {
      hints->points = hints->embedded.points;
      hints->max_points = TA_POINTS_EMBEDDED;
    }
  }
  else if (new_max > old_max)
  {
    TA_Point points_new;


    if (hints->points == hints->embedded.points)
      hints->points = NULL;

    new_max = (new_max + 2 + 7) & ~7U; /* round up to a multiple of 8 */

    points_new = (TA_Point)realloc(hints->points,
                                   new_max * sizeof (TA_PointRec));
    if (!points_new)
      return FT_Err_Out_Of_Memory;

    hints->points = points_new;
    hints->max_points = (FT_Int)new_max;
  }

  hints->num_points = outline->n_points;
  hints->num_contours = outline->n_contours;

  /* we can't rely on the value of `FT_Outline.flags' to know the fill */
  /* direction used for a glyph, given that some fonts are broken */
  /* (e.g. the Arphic ones); we thus recompute it each time we need to */

  hints->axis[TA_DIMENSION_HORZ].major_dir = TA_DIR_UP;
  hints->axis[TA_DIMENSION_VERT].major_dir = TA_DIR_LEFT;

  if (FT_Outline_Get_Orientation(outline) == FT_ORIENTATION_POSTSCRIPT)
  {
    hints->axis[TA_DIMENSION_HORZ].major_dir = TA_DIR_DOWN;
    hints->axis[TA_DIMENSION_VERT].major_dir = TA_DIR_RIGHT;
  }

  hints->x_scale = x_scale;
  hints->y_scale = y_scale;
  hints->x_delta = x_delta;
  hints->y_delta = y_delta;

  hints->xmin_delta = 0;
  hints->xmax_delta = 0;

  points = hints->points;
  if (hints->num_points == 0)
    goto Exit;

  {
    TA_Point point;
    TA_Point point_limit = points + hints->num_points;


    /* compute coordinates & Bezier flags, next and prev */
    {
      FT_Vector* vec = outline->points;
      char* tag = outline->tags;

      TA_Point end = points + outline->contours[0];
      TA_Point prev = end;

      FT_Int contour_index = 0;


      for (point = points; point < point_limit; point++, vec++, tag++)
      {
        point->in_dir = (FT_Char)TA_DIR_NONE;
        point->out_dir = (FT_Char)TA_DIR_NONE;

        point->fx = (FT_Short)vec->x;
        point->fy = (FT_Short)vec->y;
        point->ox = point->x = FT_MulFix(vec->x, x_scale) + x_delta;
        point->oy = point->y = FT_MulFix(vec->y, y_scale) + y_delta;

        switch (FT_CURVE_TAG(*tag))
        {
        case FT_CURVE_TAG_CONIC:
          point->flags = TA_FLAG_CONIC;
          break;
        case FT_CURVE_TAG_CUBIC:
          point->flags = TA_FLAG_CUBIC;
          break;
        default:
          point->flags = TA_FLAG_NONE;
        }

        point->prev = prev;
        prev->next = point;
        prev = point;

        if (point == end)
        {
          if (++contour_index < outline->n_contours)
          {
            end = points + outline->contours[contour_index];
            prev = end;
          }
        }

#ifdef TA_DEBUG
        point->before[0] = NULL;
        point->before[1] = NULL;
        point->after[0] = NULL;
        point->after[1] = NULL;
#endif
      }
    }

    /* set up the contours array */
    {
      TA_Point* contour = hints->contours;
      TA_Point* contour_limit = contour + hints->num_contours;

      short* end = outline->contours;
      short idx = 0;


      for (; contour < contour_limit; contour++, end++)
      {
        contour[0] = points + idx;
        idx = (short)(end[0] + 1);
      }
    }

    {
      /*
       *  Compute directions of `in' and `out' vectors.
       *
       *  Note that distances between points that are very near to each
       *  other are accumulated.  In other words, the auto-hinter
       *  prepends the small vectors between near points to the first
       *  non-near vector.  All intermediate points are tagged as
       *  weak; the directions are adjusted also to be equal to the
       *  accumulated one.
       */

      /* value 20 in `near_limit' is heuristic */
      FT_UInt units_per_em = hints->metrics->scaler.face->units_per_EM;
      FT_Int near_limit = 20 * units_per_em / 2048;
      FT_Int near_limit2 = 2 * near_limit - 1;

      TA_Point* contour;
      TA_Point* contour_limit = hints->contours + hints->num_contours;


      for (contour = hints->contours; contour < contour_limit; contour++)
      {
        TA_Point first = *contour;
        TA_Point next, prev, curr;

        FT_Pos out_x, out_y;


        /* since the first point of a contour could be part of a */
        /* series of near points, go backwards to find the first */
        /* non-near point and adjust `first' */

        point = first;
        prev = first->prev;

        while (prev != first)
        {
          out_x = point->fx - prev->fx;
          out_y = point->fy - prev->fy;

          /*
           *  We use Taxicab metrics to measure the vector length.
           *
           *  Note that the accumulated distances so far could have the
           *  opposite direction of the distance measured here.  For this
           *  reason we use `near_limit2' for the comparison to get a
           *  non-near point even in the worst case.
           */
          if (TA_ABS(out_x) + TA_ABS(out_y) >= near_limit2)
            break;

          point = prev;
          prev = prev->prev;
        }

        /* adjust first point */
        first = point;

        /* now loop over all points of the contour to get */
        /* `in' and `out' vector directions */

        curr = first;

        /*
         *  We abuse the `u' and `v' fields to store index deltas to the
         *  next and previous non-near point, respectively.
         *
         *  To avoid problems with not having non-near points, we point to
         *  `first' by default as the next non-near point.
         */
        curr->u = (FT_Pos)(first - curr);
        first->v = -curr->u;

        out_x = 0;
        out_y = 0;

        next = first;
        do
        {
          TA_Direction out_dir;


          point = next;
          next = point->next;

          out_x += next->fx - point->fx;
          out_y += next->fy - point->fy;

          if (TA_ABS(out_x) + TA_ABS(out_y) < near_limit)
          {
            next->flags |= TA_FLAG_WEAK_INTERPOLATION;
            continue;
          }

          curr->u = (FT_Pos)(next - curr);
          next->v = -curr->u;

          out_dir = ta_direction_compute(out_x, out_y);

          /* adjust directions for all points inbetween; */
          /* the loop also updates position of `curr' */
          curr->out_dir = (FT_Char)out_dir;
          for (curr = curr->next; curr != next; curr = curr->next)
          {
            curr->in_dir = (FT_Char)out_dir;
            curr->out_dir = (FT_Char)out_dir;
          }
          next->in_dir = (FT_Char)out_dir;

          curr->u = (FT_Pos)(first - curr);
          first->v = -curr->u;

          out_x = 0;
          out_y = 0;

        } while (next != first);
      }

      /*
       *  The next step is to `simplify' an outline's topology so that we
       *  can identify local extrema more reliably: A series of
       *  non-horizontal or non-vertical vectors pointing into the same
       *  quadrant are handled as a single, long vector.  From a
       *  topological point of the view, the intermediate points are of no
       *  interest and thus tagged as weak.
       */

      for (point = points; point < point_limit; point++)
      {
        if (point->flags & TA_FLAG_WEAK_INTERPOLATION)
          continue;

        if (point->in_dir == TA_DIR_NONE
            && point->out_dir == TA_DIR_NONE)
        {
          /* check whether both vectors point into the same quadrant */

          FT_Pos in_x, in_y;
          FT_Pos out_x, out_y;

          TA_Point next_u = point + point->u;
          TA_Point prev_v = point + point->v;


          in_x = point->fx - prev_v->fx;
          in_y = point->fy - prev_v->fy;

          out_x = next_u->fx - point->fx;
          out_y = next_u->fy - point->fy;

          if ((in_x ^ out_x) >= 0 && (in_y ^ out_y) >= 0)
          {
            /* yes, so tag current point as weak */
            /* and update index deltas */

            point->flags |= TA_FLAG_WEAK_INTERPOLATION;

            prev_v->u = (FT_Pos)(next_u - prev_v);
            next_u->v = -prev_v->u;
          }
        }
      }

      /*
       *  Finally, check for remaining weak points.  Everything else not
       *  collected in edges so far is then implicitly classified as strong
       *  points.
       */

      for (point = points; point < point_limit; point++)
      {
        if (point->flags & TA_FLAG_WEAK_INTERPOLATION)
          continue;

        if (point->flags & TA_FLAG_CONTROL)
        {
          /* control points are always weak */
        Is_Weak_Point:
          point->flags |= TA_FLAG_WEAK_INTERPOLATION;
        }
        else if (point->out_dir == point->in_dir)
        {
          if (point->out_dir != TA_DIR_NONE)
          {
            /* current point lies on a horizontal or */
            /* vertical segment (but doesn't start or end it) */
            goto Is_Weak_Point;
          }

          {
            TA_Point next_u = point + point->u;
            TA_Point prev_v = point + point->v;


            if (ta_corner_is_flat(point->fx - prev_v->fx,
                                  point->fy - prev_v->fy,
                                  next_u->fx - point->fx,
                                  next_u->fy - point->fy))
            {
              /* either the `in' or the `out' vector is much more */
              /* dominant than the other one, so tag current point */
              /* as weak and update index deltas */

              prev_v->u = (FT_Pos)(next_u - prev_v);
              next_u->v = -prev_v->u;

              goto Is_Weak_Point;
            }
          }
        }
        else if (point->in_dir == -point->out_dir)
        {
          /* current point forms a spike */
          goto Is_Weak_Point;
        }
      }
    }
  }

  /* change some directions at the user's request */
  /* to make ttfautohint insert one-point segments */
  /* or remove points from segments */
  {
    FONT* font;
    FT_Int idx;
    TA_Direction dir;
    int left_offset;
    int right_offset;


    /* `globals' is not set up while initializing metrics, */
    /* so exit early in this case */
    if (!hints->metrics->globals)
      goto Exit;

    font = hints->metrics->globals->font;

    /* start conditions are set with `TA_control_segment_dir_collect' */
    while (TA_control_segment_dir_get_next(font, &idx, &dir,
                                           &left_offset, &right_offset))
    {
      TA_Point point = &points[idx];


      point->out_dir = dir;
      if (dir == TA_DIR_NONE)
        point->flags |= TA_FLAG_WEAK_INTERPOLATION;
      else
        point->flags &= ~TA_FLAG_WEAK_INTERPOLATION;
      point->left_offset = (FT_Short)left_offset;
      point->right_offset = (FT_Short)right_offset;
    }
  }

Exit:
  return error;
}


/* store the hinted outline in an FT_Outline structure */

void
ta_glyph_hints_save(TA_GlyphHints hints,
                    FT_Outline* outline)
{
  TA_Point point = hints->points;
  TA_Point limit = point + hints->num_points;

  FT_Vector* vec = outline->points;
  char* tag = outline->tags;


  for (; point < limit; point++, vec++, tag++)
  {
    vec->x = point->x;
    vec->y = point->y;

    if (point->flags & TA_FLAG_CONIC)
      tag[0] = FT_CURVE_TAG_CONIC;
    else if (point->flags & TA_FLAG_CUBIC)
      tag[0] = FT_CURVE_TAG_CUBIC;
    else
      tag[0] = FT_CURVE_TAG_ON;
  }
}


/****************************************************************
 *
 *                     EDGE POINT GRID-FITTING
 *
 ****************************************************************/


/* align all points of an edge to the same coordinate value, */
/* either horizontally or vertically */

void
ta_glyph_hints_align_edge_points(TA_GlyphHints hints,
                                 TA_Dimension dim)
{
  TA_AxisHints axis = &hints->axis[dim];
  TA_Segment segments = axis->segments;
  TA_Segment segment_limit = segments + axis->num_segments;
  TA_Segment seg;


  if (dim == TA_DIMENSION_HORZ)
  {
    for (seg = segments; seg < segment_limit; seg++)
    {
      TA_Edge edge = seg->edge;
      TA_Point point, first, last;


      if (!edge)
        continue;

      first = seg->first;
      last = seg->last;
      point = first;
      for (;;)
      {
        point->x = edge->pos;
        point->flags |= TA_FLAG_TOUCH_X;

        if (point == last)
          break;

        point = point->next;
      }
    }
  }
  else
  {
    for (seg = segments; seg < segment_limit; seg++)
    {
      TA_Edge edge = seg->edge;
      TA_Point point, first, last;


      if (!edge)
        continue;

      first = seg->first;
      last = seg->last;
      point = first;
      for (;;)
      {
        point->y = edge->pos;
        point->flags |= TA_FLAG_TOUCH_Y;

        if (point == last)
          break;

        point = point->next;
      }
    }
  }
}


/****************************************************************
 *
 *                    STRONG POINT INTERPOLATION
 *
 ****************************************************************/


/* hint the strong points -- */
/* this is equivalent to the TrueType `IP' hinting instruction */

void
ta_glyph_hints_align_strong_points(TA_GlyphHints hints,
                                   TA_Dimension dim)
{
  TA_Point points = hints->points;
  TA_Point point_limit = points + hints->num_points;

  TA_AxisHints axis = &hints->axis[dim];

  TA_Edge edges = axis->edges;
  TA_Edge edge_limit = edges + axis->num_edges;

  FT_UShort touch_flag;


  if (dim == TA_DIMENSION_HORZ)
    touch_flag = TA_FLAG_TOUCH_X;
  else
    touch_flag = TA_FLAG_TOUCH_Y;

  if (edges < edge_limit)
  {
    TA_Point point;
    TA_Edge edge;


    for (point = points; point < point_limit; point++)
    {
      FT_Pos u, ou, fu; /* point position */
      FT_Pos delta;


      if (point->flags & touch_flag)
        continue;

      /* if this point is candidate to weak interpolation, we */
      /* interpolate it after all strong points have been processed */

      if ((point->flags & TA_FLAG_WEAK_INTERPOLATION))
        continue;

      if (dim == TA_DIMENSION_VERT)
      {
        u = point->fy;
        ou = point->oy;
      }
      else
      {
        u = point->fx;
        ou = point->ox;
      }

      fu = u;

      /* is the point before the first edge? */
      edge = edges;
      delta = edge->fpos - u;
      if (delta >= 0)
      {
        u = edge->pos - (edge->opos - ou);

#ifdef TA_DEBUG
        point->before[dim] = edge;
        point->after[dim] = NULL;
#endif

        if (hints->recorder)
          hints->recorder(ta_ip_before, hints, dim,
                          point, NULL, NULL, NULL, NULL);

        goto Store_Point;
      }

      /* is the point after the last edge? */
      edge = edge_limit - 1;
      delta = u - edge->fpos;
      if (delta >= 0)
      {
        u = edge->pos + (ou - edge->opos);

#ifdef TA_DEBUG
        point->before[dim] = NULL;
        point->after[dim] = edge;
#endif

        if (hints->recorder)
          hints->recorder(ta_ip_after, hints, dim,
                          point, NULL, NULL, NULL, NULL);

        goto Store_Point;
      }

      {
        FT_PtrDist min, max, mid;
        FT_Pos fpos;


        /* find enclosing edges */
        min = 0;
        max = edge_limit - edges;

        /* for a small number of edges, a linear search is better */
        if (max <= 8)
        {
          FT_PtrDist nn;


          for (nn = 0; nn < max; nn++)
            if (edges[nn].fpos >= u)
              break;

          if (edges[nn].fpos == u)
          {
            u = edges[nn].pos;

            if (hints->recorder)
              hints->recorder(ta_ip_on, hints, dim,
                              point, &edges[nn], NULL, NULL, NULL);

            goto Store_Point;
          }
          min = nn;
        }
        else
          while (min < max)
          {
            mid = (max + min) >> 1;
            edge = edges + mid;
            fpos = edge->fpos;

            if (u < fpos)
              max = mid;
            else if (u > fpos)
              min = mid + 1;
            else
            {
              /* we are on the edge */
              u = edge->pos;

#ifdef TA_DEBUG
              point->before[dim] = NULL;
              point->after[dim] = NULL;
#endif

              if (hints->recorder)
                hints->recorder(ta_ip_on, hints, dim,
                                point, edge, NULL, NULL, NULL);

              goto Store_Point;
            }
          }

        /* point is not on an edge */
        {
          TA_Edge before = edges + min - 1;
          TA_Edge after = edges + min + 0;


#ifdef TA_DEBUG
          point->before[dim] = before;
          point->after[dim] = after;
#endif

          /* assert(before && after && before != after) */
          if (before->scale == 0)
            before->scale = FT_DivFix(after->pos - before->pos,
                                      after->fpos - before->fpos);

          u = before->pos + FT_MulFix(fu - before->fpos,
                                      before->scale);

          if (hints->recorder)
            hints->recorder(ta_ip_between, hints, dim,
                            point, before, after, NULL, NULL);
        }
      }

    Store_Point:
      /* save the point position */
      if (dim == TA_DIMENSION_HORZ)
        point->x = u;
      else
        point->y = u;

      point->flags |= touch_flag;
    }
  }
}


/****************************************************************
 *
 *                    WEAK POINT INTERPOLATION
 *
 ****************************************************************/


/* shift the original coordinates of all points between `p1' and */
/* `p2' to get hinted coordinates, using the same difference as */
/* given by `ref' */

static void
ta_iup_shift(TA_Point p1,
             TA_Point p2,
             TA_Point ref)
{
  TA_Point p;
  FT_Pos delta = ref->u - ref->v;


  if (delta == 0)
    return;

  for (p = p1; p < ref; p++)
    p->u = p->v + delta;

  for (p = ref + 1; p <= p2; p++)
    p->u = p->v + delta;
}


/* interpolate the original coordinates of all points between `p1' and */
/* `p2' to get hinted coordinates, using `ref1' and `ref2' as the */
/* reference points; the `u' and `v' members are the current and */
/* original coordinate values, respectively. */

/* details can be found in the TrueType bytecode specification */

static void
ta_iup_interp(TA_Point p1,
              TA_Point p2,
              TA_Point ref1,
              TA_Point ref2)
{
  TA_Point p;
  FT_Pos u, v1, v2, u1, u2, d1, d2;


  if (p1 > p2)
    return;

  if (ref1->v > ref2->v)
  {
    p = ref1;
    ref1 = ref2;
    ref2 = p;
  }

  v1 = ref1->v;
  v2 = ref2->v;
  u1 = ref1->u;
  u2 = ref2->u;
  d1 = u1 - v1;
  d2 = u2 - v2;

  if (u1 == u2 || v1 == v2)
  {
    for (p = p1; p <= p2; p++)
    {
      u = p->v;

      if (u <= v1)
        u += d1;
      else if (u >= v2)
        u += d2;
      else
        u = u1;

      p->u = u;
    }
  }
  else
  {
    FT_Fixed scale = FT_DivFix(u2 - u1, v2 - v1);


    for (p = p1; p <= p2; p++)
    {
      u = p->v;

      if (u <= v1)
        u += d1;
      else if (u >= v2)
        u += d2;
      else
        u = u1 + FT_MulFix(u - v1, scale);

      p->u = u;
    }
  }
}


/* hint the weak points -- */
/* this is equivalent to the TrueType `IUP' hinting instruction */

void
ta_glyph_hints_align_weak_points(TA_GlyphHints hints,
                                 TA_Dimension dim)
{
  TA_Point points = hints->points;
  TA_Point point_limit = points + hints->num_points;

  TA_Point* contour = hints->contours;
  TA_Point* contour_limit = contour + hints->num_contours;

  FT_UShort touch_flag;
  TA_Point point;
  TA_Point end_point;
  TA_Point first_point;


  /* pass 1: move segment points to edge positions */

  if (dim == TA_DIMENSION_HORZ)
  {
    touch_flag = TA_FLAG_TOUCH_X;

    for (point = points; point < point_limit; point++)
    {
      point->u = point->x;
      point->v = point->ox;
    }
  }
  else
  {
    touch_flag = TA_FLAG_TOUCH_Y;

    for (point = points; point < point_limit; point++)
    {
      point->u = point->y;
      point->v = point->oy;
    }
  }

  for (; contour < contour_limit; contour++)
  {
    TA_Point first_touched, last_touched;


    point = *contour;
    end_point = point->prev;
    first_point = point;

    /* find first touched point */
    for (;;)
    {
      if (point > end_point) /* no touched point in contour */
        goto NextContour;

      if (point->flags & touch_flag)
        break;

      point++;
    }

    first_touched = point;

    for (;;)
    {
      /* skip any touched neighbours */
      while (point < end_point
             && (point[1].flags & touch_flag) != 0)
        point++;

      last_touched = point;

      /* find the next touched point, if any */
      point++;
      for (;;)
      {
        if (point > end_point)
          goto EndContour;

        if ((point->flags & touch_flag) != 0)
          break;

        point++;
      }

      /* interpolate between last_touched and point */
      ta_iup_interp(last_touched + 1, point - 1,
                    last_touched, point);
    }

  EndContour:
    /* special case: only one point was touched */
    if (last_touched == first_touched)
      ta_iup_shift(first_point, end_point, first_touched);

    else /* interpolate the last part */
    {
      if (last_touched < end_point)
        ta_iup_interp(last_touched + 1, end_point,
                      last_touched, first_touched);

      if (first_touched > points)
        ta_iup_interp(first_point, first_touched - 1,
                      last_touched, first_touched);
    }

  NextContour:
    ;
  }

  /* now save the interpolated values back to x/y */
  if (dim == TA_DIMENSION_HORZ)
  {
    for (point = points; point < point_limit; point++)
      point->x = point->u;
  }
  else
  {
    for (point = points; point < point_limit; point++)
      point->y = point->u;
  }
}


#ifdef TA_CONFIG_OPTION_USE_WARPER

/* apply (small) warp scale and warp delta for given dimension */

static void
ta_glyph_hints_scale_dim(TA_GlyphHints hints,
                         TA_Dimension dim,
                         FT_Fixed scale,
                         FT_Pos delta)
{
  TA_Point points = hints->points;
  TA_Point points_limit = points + hints->num_points;
  TA_Point point;


  if (dim == TA_DIMENSION_HORZ)
  {
    for (point = points; point < points_limit; point++)
      point->x = FT_MulFix(point->fx, scale) + delta;
  }
  else
  {
    for (point = points; point < points_limit; point++)
      point->y = FT_MulFix(point->fy, scale) + delta;
  }
}

#endif /* TA_CONFIG_OPTION_USE_WARPER */

/* end of tahints.c */
