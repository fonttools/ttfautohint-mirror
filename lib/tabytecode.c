/* tabytecode.c */

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

#include <stdbool.h> /* for llrb.h */

#include "llrb.h" /* a red-black tree implementation */
#include "tahints.h"


#define DEBUGGING


#ifdef TA_DEBUG
int _ta_debug = 0;
int _ta_debug_global = 0;
int _ta_debug_disable_horz_hints;
int _ta_debug_disable_vert_hints;
int _ta_debug_disable_blue_hints;
void* _ta_debug_hints;
#endif


/* node structures for point hints */

typedef struct Node1 Node1;
struct Node1
{
  LLRB_ENTRY(Node1) entry1;
  FT_UShort point;
};

typedef struct Node2 Node2;
struct Node2
{
  LLRB_ENTRY(Node2) entry2;
  FT_UShort edge;
  FT_UShort point;
};

typedef struct Node3 Node3;
struct Node3
{
  LLRB_ENTRY(Node3) entry3;
  FT_UShort before_edge;
  FT_UShort after_edge;
  FT_UShort point;
};


/* comparison functions for our red-black trees */

static int
node1cmp(Node1* e1,
         Node1* e2)
{
  /* sort by points */
  return e1->point - e2->point;
}

static int
node2cmp(Node2* e1,
         Node2* e2)
{
  FT_Int delta;


  /* sort by edges ... */
  delta = (FT_Int)e1->edge - (FT_Int)e2->edge;
  if (delta)
    return delta;

  /* ... then by points */
  return (FT_Int)e1->point - (FT_Int)e2->point;
}

static int
node3cmp(Node3* e1,
         Node3* e2)
{
  FT_Int delta;


  /* sort by `before' edges ... */
  delta = (FT_Int)e1->before_edge - (FT_Int)e2->before_edge;
  if (delta)
    return delta;

  /* ... then by `after' edges ... */
  delta = (FT_Int)e1->after_edge - (FT_Int)e2->after_edge;
  if (delta)
    return delta;

  /* ... then by points */
  return (FT_Int)e1->point - (FT_Int)e2->point;
}


/* the red-black tree function bodies */
typedef struct ip_before_points ip_before_points;
typedef struct ip_after_points ip_after_points;
typedef struct ip_on_points ip_on_points;
typedef struct ip_between_points ip_between_points;

LLRB_HEAD(ip_before_points, Node1);
LLRB_HEAD(ip_after_points, Node1);
LLRB_HEAD(ip_on_points, Node2);
LLRB_HEAD(ip_between_points, Node3);

/* no trailing semicolon in the next four lines */
LLRB_GENERATE_STATIC(ip_before_points, Node1, entry1, node1cmp)
LLRB_GENERATE_STATIC(ip_after_points, Node1, entry1, node1cmp)
LLRB_GENERATE_STATIC(ip_on_points, Node2, entry2, node2cmp)
LLRB_GENERATE_STATIC(ip_between_points, Node3, entry3, node3cmp)


typedef struct Hints_Record_
{
  FT_UInt size;
  FT_UInt num_actions;
  FT_Byte* buf;
  FT_UInt buf_len;
} Hints_Record;

typedef struct Recorder_
{
  SFNT* sfnt;
  FONT* font;
  GLYPH* glyph; /* the current glyph */
  Hints_Record hints_record;

  /* some segments can `wrap around' */
  /* a contour's start point like 24-25-26-0-1-2 */
  /* (there can be at most one such segment per contour); */
  /* later on we append additional records */
  /* to split them into 24-26 and 0-2 */
  FT_UShort* wrap_around_segments;
  FT_UShort num_wrap_around_segments;
  FT_Bool wrap_around_segments_initialized;

  FT_UShort num_stack_elements; /* the necessary stack depth so far */

  /* data necessary for strong point interpolation */
  ip_before_points ip_before_points_head;
  ip_after_points ip_after_points_head;
  ip_on_points ip_on_points_head;
  ip_between_points ip_between_points_head;

  /* we omit one-point segments not part of an edge, */
  /* thus we have to adjust indices into the `segments' array */
  FT_UShort* segment_map;
  FT_Bool segment_map_initialized;
} Recorder;


/* this is the bytecode of the `.ttfautohint' glyph */

FT_Byte ttfautohint_glyph_bytecode[7] =
{

  /* increment `cvtl_is_subglyph' counter */
  PUSHB_3,
    cvtl_is_subglyph,
    100,
    cvtl_is_subglyph,
  RCVT,
  ADD,
  WCVTP,

};


/* if we have y delta exceptions before IUP_y, this code gets inserted */
static FT_Byte ins_extra_delta_exceptions[4] =
{

  /* tell bci_{scale,scale_composite,hint}_glyph to not call IUP_y */
  PUSHB_2,
    cvtl_do_iup_y,
    0,
  WCVTP,

};


/* if we have a non-base glyph, this code gets inserted */
static FT_Byte ins_extra_ignore_std_width[4] =
{

  /* tell bci_{smooth,strong}_stem_width to ignore std_width */
  PUSHB_2,
    cvtl_ignore_std_width,
    100,
  WCVTP,

};


/*
 * convert array `args' into a sequence of NPUSHB, NPUSHW, PUSHB_X, and
 * PUSHW_X instructions to be stored in `bufp' (the latter two instructions
 * only if `optimize' is not set); if `need_words' is set, NPUSHW and
 * PUSHW_X gets used
 */

FT_Byte*
TA_build_push(FT_Byte* bufp,
              FT_UInt* args,
              FT_UInt num_args,
              FT_Bool need_words,
              FT_Bool optimize)
{
  FT_UInt* arg = args;
  FT_UInt i, j, nargs;


  if (need_words)
  {
    for (i = 0; i < num_args; i += 255)
    {
      nargs = (num_args - i > 255) ? 255 : num_args - i;

      if (optimize && nargs <= 8)
        BCI(PUSHW_1 - 1 + nargs);
      else
      {
        BCI(NPUSHW);
        BCI(nargs);
      }
      for (j = 0; j < nargs; j++)
      {
        BCI(HIGH(*arg));
        BCI(LOW(*arg));
        arg++;
      }
    }
  }
  else
  {
    for (i = 0; i < num_args; i += 255)
    {
      nargs = (num_args - i > 255) ? 255 : num_args - i;

      if (optimize && nargs <= 8)
        BCI(PUSHB_1 - 1 + nargs);
      else
      {
        BCI(NPUSHB);
        BCI(nargs);
      }
      for (j = 0; j < nargs; j++)
      {
        BCI(*arg);
        arg++;
      }
    }
  }

  return bufp;
}


/*
 * We optimize two common cases, replacing
 *
 *   NPUSHB A ... NPUSHB B [... NPUSHB C] ... CALL
 *
 * with
 *
 *   NPUSHB (A+B[+C]) ... CALL
 *
 * if possible
 */

static FT_Byte*
TA_optimize_push(FT_Byte* buf,
                 FT_Byte** pos)
{
  FT_Byte sizes[3];
  FT_Byte new_size1;
  FT_Byte new_size2;

  FT_UInt sum;
  FT_UInt i;
  FT_UInt pos_idx;

  FT_Byte* p;
  FT_Byte* bufp;


  /* XXX improve handling of NPUSHW */
  if (*(pos[0]) == NPUSHW
      || *(pos[1]) == NPUSHW
      || *(pos[2]) == NPUSHW)
    return buf;

  /* the point hints records block can be missing */
  if (pos[0] == pos[1])
  {
    pos[1] = pos[2];
    pos[2] = NULL;
  }

  /* only needed for the algorithm loops below */
  pos[3] = NULL;

  /* there are at least two NPUSHB instructions */
  /* (one of them directly at the start) */
  sizes[0] = *(pos[0] + 1);
  sizes[1] = *(pos[1] + 1);
  sizes[2] = pos[2] ? *(pos[2] + 1) : 0;

  sum = sizes[0] + sizes[1] + sizes[2];

  if (sum > 2 * 0xFF)
    return buf; /* nothing to do since we need three NPUSHB */
  else if (!sizes[2] && (sum > 0xFF))
    return buf; /* nothing to do since we need two NPUSHB */

  if (sum > 0xFF)
  {
    /* reduce three NPUSHB to two */
    new_size1 = 0xFF;
    new_size2 = (FT_Byte)(sum - 0xFF);
  }
  else
  {
    /* reduce two or three NPUSHB to one */
    new_size1 = (FT_Byte)sum;
    new_size2 = 0;
  }

  /* pack data */
  p = buf;
  bufp = buf;
  pos_idx = 0;

  if (new_size1 <= 8)
    BCI(PUSHB_1 - 1 + new_size1);
  else
  {
    BCI(NPUSHB);
    BCI(new_size1);
  }
  for (i = 0; i < new_size1; i++)
  {
    if (p == pos[pos_idx])
    {
      pos_idx++;
      p += 2; /* skip old NPUSHB */
    }
    *(bufp++) = *(p++);
  }

  if (new_size2)
  {
    if (new_size2 <= 8)
      BCI(PUSHB_1 - 1 + new_size2);
    else
    {
      BCI(NPUSHB);
      BCI(new_size2);
    }
    for (i = 0; i < new_size2; i++)
    {
      if (p == pos[pos_idx])
      {
        pos_idx++;
        p += 2;
      }
      *(bufp++) = *(p++);
    }
  }

  BCI(CALL);

  return bufp;
}


/* We add a subglyph for each composite glyph. */
/* Since subglyphs must contain at least one point, */
/* we have to adjust all point indices accordingly. */
/* Using the `pointsums' array of the `GLYPH' structure */
/* it is straightforward to do that: */
/* Assuming that point with index x is in the interval */
/* pointsums[n] <= x < pointsums[n + 1], */
/* the new point index is x + n. */

static FT_UInt
TA_adjust_point_index(Recorder* recorder,
                      FT_UInt idx)
{
  FONT* font = recorder->font;
  GLYPH* glyph = recorder->glyph;
  FT_UShort i;


  if (!glyph->num_components || !font->hint_composites)
    return idx; /* not a composite glyph */

  for (i = 0; i < glyph->num_pointsums; i++)
    if (idx < glyph->pointsums[i])
      break;

  return idx + i;
}


/* We omit one-point segments not part of an edge, */
/* thus we have to adjust indices into the `segments' array. */
/* If `segment' is NULL, the number of all segments */
/* without non-edge one-point segments is returned. */

static FT_UShort
TA_get_segment_index(TA_Segment segment,
                     Recorder* recorder)
{
  FONT* font = recorder->font;
  TA_GlyphHints hints = &font->loader->hints;
  TA_AxisHints axis = &hints->axis[TA_DIMENSION_VERT];


  return segment ? recorder->segment_map[segment - axis->segments]
                 : recorder->segment_map[axis->num_segments];
}


/* we store the segments in the storage area; */
/* each segment record consists of the first and last point */

static FT_Byte*
TA_sfnt_build_glyph_segments(SFNT* sfnt,
                             Recorder* recorder,
                             FT_Byte* bufp,
                             FT_Bool optimize)
{
  FONT* font = recorder->font;
  TA_GlyphHints hints = &font->loader->hints;
  TA_AxisHints axis = &hints->axis[TA_DIMENSION_VERT];
  TA_Point points = hints->points;
  TA_Segment segments = axis->segments;
  TA_Segment seg;
  TA_Segment seg_limit;

  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;

  FT_UInt style_id = data->style_ids
                       [font->loader->metrics->style_class->style];

  FT_Outline outline = font->loader->gloader->base.outline;

  FT_UInt* args;
  FT_UInt* arg;
  FT_UInt num_args;
  FT_UShort num_segments;

  FT_Bool need_words = 0;

  FT_Int n;
  FT_UInt base;
  FT_UShort num_packed_segments;
  FT_UShort num_storage;
  FT_UShort num_stack_elements;
  FT_UShort num_twilight_points;


  seg_limit = segments + axis->num_segments;
  num_segments = TA_get_segment_index(NULL, recorder);

  /* to pack the data in the bytecode more tightly, */
  /* we store up to the first nine segments in nibbles if possible, */
  /* using delta values */
  base = 0;
  num_packed_segments = 0;
  for (seg = segments; seg < seg_limit; seg++)
  {
    FT_UInt first = (FT_UInt)(seg->first - points);
    FT_UInt last = (FT_UInt)(seg->last - points);


    if (TA_get_segment_index(seg, recorder) == 0xFFFF)
      continue;

    first = TA_adjust_point_index(recorder, first);
    last = TA_adjust_point_index(recorder, last);

    if (first - base >= 16)
      break;
    if (first > last || last - first >= 16)
      break;
    if (num_packed_segments == 9)
      break;
    num_packed_segments++;
    base = last;
  }

  /* also handle wrap-around segments */
  num_segments += recorder->num_wrap_around_segments;

  /* wrap-around segments are pushed with four arguments; */
  /* a segment stored in nibbles needs only one byte instead of two */
  num_args = num_packed_segments
             + 2 * (num_segments - num_packed_segments)
             + 2 * recorder->num_wrap_around_segments
             + 3;

  /* collect all arguments temporarily in an array (in reverse order) */
  /* so that we can easily split into chunks of 255 args */
  /* as needed by NPUSHB and NPUSHW, respectively */
  args = (FT_UInt*)malloc(num_args * sizeof (FT_UInt));
  if (!args)
    return NULL;

  arg = args + num_args - 1;

  if (num_segments > 0xFF)
    need_words = 1;

  /* the number of packed segments is indicated by the function number */
  if (recorder->glyph->num_components && font->hint_composites)
    *(arg--) = bci_create_segments_composite_0 + num_packed_segments;
  else
    *(arg--) = bci_create_segments_0 + num_packed_segments;

  *(arg--) = CVT_SCALING_VALUE_OFFSET(style_id);
  *(arg--) = num_segments;

  base = 0;
  n = 0;
  for (seg = segments; seg < seg_limit; seg++)
  {
    FT_UInt first = (FT_UInt)(seg->first - points);
    FT_UInt last = (FT_UInt)(seg->last - points);
    FT_UInt low_nibble;
    FT_UInt high_nibble;


    if (TA_get_segment_index(seg, recorder) == 0xFFFF)
      continue;
    if (n >= num_packed_segments)
      break;

    first = TA_adjust_point_index(recorder, first);
    last = TA_adjust_point_index(recorder, last);

    low_nibble = first - base;
    high_nibble = last - first;

    *(arg--) = 16 * high_nibble + low_nibble;

    base = last;
    n++;
  }

  for (; seg < seg_limit; seg++)
  {
    FT_UInt first = (FT_UInt)(seg->first - points);
    FT_UInt last = (FT_UInt)(seg->last - points);


    if (TA_get_segment_index(seg, recorder) == 0xFFFF)
      continue;

    *(arg--) = TA_adjust_point_index(recorder, first);
    *(arg--) = TA_adjust_point_index(recorder, last);

    /* we push the last and first contour point */
    /* as a third and fourth argument in wrap-around segments */
    if (first > last)
    {
      for (n = 0; n < outline.n_contours; n++)
      {
        FT_UInt end = (FT_UInt)outline.contours[n];


        if (first <= end)
        {
          *(arg--) = TA_adjust_point_index(recorder, end);
          if (end > 0xFF)
            need_words = 1;

          if (n == 0)
            *(arg--) = TA_adjust_point_index(recorder, 0);
          else
            *(arg--) = TA_adjust_point_index(recorder,
                         (FT_UInt)outline.contours[n - 1] + 1);
          break;
        }
      }
    }

    if (last > 0xFF)
      need_words = 1;
  }

  /* emit the second part of wrap-around segments as separate segments */
  /* so that edges can easily link to them */
  for (seg = segments; seg < seg_limit; seg++)
  {
    FT_UInt first = (FT_UInt)(seg->first - points);
    FT_UInt last = (FT_UInt)(seg->last - points);


    if (TA_get_segment_index(seg, recorder) == 0xFFFF)
      continue;

    if (first > last)
    {
      for (n = 0; n < outline.n_contours; n++)
      {
        if (first <= (FT_UInt)outline.contours[n])
        {
          if (n == 0)
            *(arg--) = TA_adjust_point_index(recorder, 0);
          else
            *(arg--) = TA_adjust_point_index(recorder,
                         (FT_UInt)outline.contours[n - 1] + 1);
          break;
        }
      }

      *(arg--) = TA_adjust_point_index(recorder, last);
    }
  }

  /* with most fonts it is very rare */
  /* that any of the pushed arguments is larger than 0xFF, */
  /* thus we refrain from further optimizing this case */
  bufp = TA_build_push(bufp, args, num_args, need_words, optimize);

  BCI(CALL);

  /* see storage area layout description in `tabytecode.h' */
  num_storage = sal_segment_offset + num_segments * 3;
  if (num_storage > sfnt->max_storage)
    sfnt->max_storage = num_storage;

  num_twilight_points = num_segments * 2;
  if (num_twilight_points > sfnt->max_twilight_points)
    sfnt->max_twilight_points = num_twilight_points;

  /* both this function and `TA_emit_hints_record' */
  /* push data onto the stack */
  num_stack_elements = (FT_UShort)(ADDITIONAL_STACK_ELEMENTS
                                   + recorder->num_stack_elements
                                   + num_args);
  if (num_stack_elements > sfnt->max_stack_elements)
    sfnt->max_stack_elements = num_stack_elements;

  free(args);

  return bufp;
}


static void
build_delta_exception(const Ctrl* ctrl,
                      FT_UInt** delta_args,
                      unsigned int* num_delta_args)
{
  int offset;
  int ppem;
  int x_shift;
  int y_shift;


  ppem = ctrl->ppem - CONTROL_DELTA_PPEM_MIN;

  if (ppem < 16)
    offset = 0;
  else if (ppem < 32)
    offset = 1;
  else
    offset = 2;

  ppem -= offset << 4;

  /*
   * Using
   *
   *   delta_shift = 3   ,
   *
   * the possible shift values in the instructions are indexed as follows:
   *
   *    0   -1px
   *    1   -7/8px
   *   ...
   *    7   -1/8px
   *    8    1/8px
   *   ...
   *   14    7/8px
   *   15    1px
   *
   * (note that there is no index for a zero shift).
   */

  if (ctrl->x_shift < 0)
    x_shift = ctrl->x_shift + 8;
  else
    x_shift = ctrl->x_shift + 7;

  if (ctrl->y_shift < 0)
    y_shift = ctrl->y_shift + 8;
  else
    y_shift = ctrl->y_shift + 7;

  /* add point index and exception specification to appropriate stack */
  if (ctrl->x_shift)
  {
    *(delta_args[offset] + num_delta_args[offset]++) =
      (FT_UInt)((ppem << 4) + x_shift);
    *(delta_args[offset] + num_delta_args[offset]++) =
      (FT_UInt)ctrl->point_idx;
  }

  if (ctrl->y_shift)
  {
    offset += 3;
    *(delta_args[offset] + num_delta_args[offset]++) =
      (FT_UInt)((ppem << 4) + y_shift);
    *(delta_args[offset] + num_delta_args[offset]++) =
      (FT_UInt)ctrl->point_idx;
  }
}


static FT_Byte*
TA_sfnt_build_delta_exceptions(SFNT* sfnt,
                               FONT* font,
                               FT_Long idx,
                               FT_Byte* bufp)
{
  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;
  GLYPH* glyph = &data->glyphs[idx];

  FT_Face face = font->loader->face;

  unsigned int num_points;
  int i;

  FT_UShort num_before_IUP_stack_elements = 0;
  FT_UShort num_after_IUP_stack_elements = 0;

  /* DELTAP[1-3] stacks for both x and y directions */
  FT_UInt* delta_before_IUP_args[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
  FT_UInt* delta_after_IUP_args[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
  unsigned int num_delta_before_IUP_args[6] = {0, 0, 0, 0, 0, 0};
  unsigned int num_delta_after_IUP_args[6] = {0, 0, 0, 0, 0, 0};
  FT_UInt* args = NULL;

  FT_Bool need_before_IUP_words = 0;
  FT_Bool need_after_IUP_words = 0;
  FT_Bool need_before_IUP_word_counts = 0;
  FT_Bool need_after_IUP_word_counts = 0;
  FT_Bool allocated_before_IUP = 0;
  FT_Bool allocated_after_IUP = 0;


  num_points = glyph->num_points;

  /* loop over all fitting control instructions */
  for (;;)
  {
    const Ctrl* ctrl = TA_control_get_ctrl(font);


    if (!ctrl)
      break;

    /* check type */
    if (!(ctrl->type == Control_Delta_before_IUP
          || ctrl->type == Control_Delta_after_IUP))
      break;

    /* too large values of font and glyph indices in `ctrl' */
    /* are handled by later calls of this function */
    if (face->face_index < ctrl->font_idx
        || idx < ctrl->glyph_idx)
      break;

    if (ctrl->type == Control_Delta_before_IUP
        && !allocated_before_IUP)
    {
      for (i = 0; i < 6; i++)
      {
        /* see the comment on allocating `ins_buf' in function */
        /* `TA_sfnt_build_glyph_instructions' for more on the array sizes; */
        /* we have to increase by 2 to push the number of argument pairs */
        /* and the function for a LOOPCALL instruction */
        delta_before_IUP_args[i] = (FT_UInt*)malloc((16 * 2 * num_points + 2)
                                                    * sizeof (FT_UInt));
        if (!delta_before_IUP_args[i])
        {
          bufp = NULL;
          goto Done;
        }
      }

      allocated_before_IUP = 1;
    }

    if (ctrl->type == Control_Delta_after_IUP
        && !allocated_after_IUP)
    {
      for (i = 0; i < 6; i++)
      {
        /* we have to increase by 1 for the number of argument pairs */
        /* as needed by the DELTA instructions */
        delta_after_IUP_args[i] = (FT_UInt*)malloc((16 * 2 * num_points + 1)
                                                   * sizeof (FT_UInt));
        if (!delta_after_IUP_args[i])
        {
          bufp = NULL;
          goto Done;
        }
      }

      allocated_after_IUP = 1;
    }

    /* since we walk sequentially over all glyphs (with points), */
    /* and the control instruction entries have the same order, */
    /* we don't need to test for equality of font and glyph indices: */
    /* at this very point in the code we certainly have a hit */
    if (ctrl->type == Control_Delta_before_IUP)
    {
      build_delta_exception(ctrl,
                            delta_before_IUP_args,
                            num_delta_before_IUP_args);

      if (ctrl->point_idx > 255)
        need_before_IUP_words = 1;
    }
    else
    {
      build_delta_exception(ctrl,
                            delta_after_IUP_args,
                            num_delta_after_IUP_args);

      if (ctrl->point_idx > 255)
        need_after_IUP_words = 1;
    }

    TA_control_get_next(font);
  }

  /* nothing to do if no control instructions */
  if (!(allocated_before_IUP || allocated_after_IUP))
    return bufp;

  /* add number of argument pairs and function number to the stacks */
  for (i = 0; i < 6; i++)
  {
    if (num_delta_before_IUP_args[i])
    {
      unsigned int n = num_delta_before_IUP_args[i] >> 1;


      if (n > 255)
        need_before_IUP_word_counts = 1;

      *(delta_before_IUP_args[i] + num_delta_before_IUP_args[i]) = n;
      num_delta_before_IUP_args[i]++;

      *(delta_before_IUP_args[i] + num_delta_before_IUP_args[i]) =
        bci_deltap1 + (i % 3);
      num_delta_before_IUP_args[i]++;
    }
  }

  /* add number of argument pairs to the stacks */
  for (i = 0; i < 6; i++)
  {
    if (num_delta_after_IUP_args[i])
    {
      unsigned int n = num_delta_after_IUP_args[i] >> 1;


      if (n > 255)
        need_after_IUP_word_counts = 1;

      *(delta_after_IUP_args[i] + num_delta_after_IUP_args[i]) = n;
      num_delta_after_IUP_args[i]++;
    }
  }

  /* merge `before IUP' delta stacks into a single one */
  if (need_before_IUP_words || !need_before_IUP_word_counts)
  {
    FT_UInt num_args = 0;


    for (i = 0; i < 6; i++)
    {
      FT_UInt* args_new;
      FT_UInt num_args_new;


      if (!num_delta_before_IUP_args[i])
        continue;

      num_args_new = num_args + num_delta_before_IUP_args[i];
      args_new = (FT_UInt*)realloc(args, num_args_new * sizeof (FT_UInt));
      if (!args_new)
      {
        bufp = NULL;
        goto Done;
      }

      memcpy(args_new + num_args,
             delta_before_IUP_args[i],
             num_delta_before_IUP_args[i] * sizeof (FT_UInt));

      args = args_new;
      num_args = num_args_new;
    }

    num_before_IUP_stack_elements = (FT_UShort)num_args;

    bufp = TA_build_push(bufp, args, num_args, need_before_IUP_words, 1);
  }
  else
  {
    num_before_IUP_stack_elements = 0;

    /* stack elements are bytes, but counts need words */
    for (i = 0; i < 6; i++)
    {
      FT_UInt num_delta_arg;


      if (!num_delta_before_IUP_args[i])
        continue;

      num_delta_arg = num_delta_before_IUP_args[i] - 2;

      bufp = TA_build_push(bufp,
                           delta_before_IUP_args[i],
                           num_delta_arg,
                           need_before_IUP_words,
                           1);

      num_before_IUP_stack_elements += num_delta_arg + 2;

      num_delta_arg >>= 1;
      BCI(PUSHW_1);
      BCI(HIGH(num_delta_arg));
      BCI(LOW(num_delta_arg));

      /* the function number */
      BCI(PUSHB_1);
      BCI(delta_before_IUP_args[i][num_delta_before_IUP_args[i] - 1]);
    }
  }

  /* emit y DELTA opcodes (via `bci_deltap[1-3]' functions) */
  if (num_delta_before_IUP_args[5])
    BCI(LOOPCALL);
  if (num_delta_before_IUP_args[4])
    BCI(LOOPCALL);
  if (num_delta_before_IUP_args[3])
    BCI(LOOPCALL);

  if (num_delta_before_IUP_args[2]
      || num_delta_before_IUP_args[1]
      || num_delta_before_IUP_args[0])
    BCI(SVTCA_x);

  /* emit x DELTA opcodes */
  if (num_delta_before_IUP_args[2])
    BCI(LOOPCALL);
  if (num_delta_before_IUP_args[1])
    BCI(LOOPCALL);
  if (num_delta_before_IUP_args[0])
    BCI(LOOPCALL);

  if (num_delta_before_IUP_args[2]
      || num_delta_before_IUP_args[1]
      || num_delta_before_IUP_args[0])
    BCI(SVTCA_y);

  if (num_delta_before_IUP_args[5]
      || num_delta_before_IUP_args[4]
      || num_delta_before_IUP_args[3])
    BCI(IUP_y);

  /* merge `after IUP' delta stacks into a single one */
  if (need_after_IUP_words || !need_after_IUP_word_counts)
  {
    FT_UInt num_args = 0;


    for (i = 0; i < 6; i++)
    {
      FT_UInt* args_new;
      FT_UInt num_args_new;


      if (!num_delta_after_IUP_args[i])
        continue;

      num_args_new = num_args + num_delta_after_IUP_args[i];
      args_new = (FT_UInt*)realloc(args, num_args_new * sizeof (FT_UInt));
      if (!args_new)
      {
        bufp = NULL;
        goto Done;
      }

      memcpy(args_new + num_args,
             delta_after_IUP_args[i],
             num_delta_after_IUP_args[i] * sizeof (FT_UInt));

      args = args_new;
      num_args = num_args_new;
    }

    num_after_IUP_stack_elements = (FT_UShort)num_args;

    bufp = TA_build_push(bufp, args, num_args, need_after_IUP_words, 1);
  }
  else
  {
    num_after_IUP_stack_elements = 0;

    /* stack elements are bytes, but counts need words */
    for (i = 0; i < 6; i++)
    {
      FT_UInt num_delta_arg;


      if (!num_delta_after_IUP_args[i])
        continue;

      num_delta_arg = num_delta_after_IUP_args[i] - 1;

      bufp = TA_build_push(bufp,
                           delta_after_IUP_args[i],
                           num_delta_arg,
                           need_after_IUP_words,
                           1);

      num_after_IUP_stack_elements += num_delta_arg + 1;

      num_delta_arg >>= 1;
      BCI(PUSHW_1);
      BCI(HIGH(num_delta_arg));
      BCI(LOW(num_delta_arg));
    }
  }

  /* emit y DELTA opcodes */
  if (num_delta_after_IUP_args[5])
    BCI(DELTAP3);
  if (num_delta_after_IUP_args[4])
    BCI(DELTAP2);
  if (num_delta_after_IUP_args[3])
    BCI(DELTAP1);

  if (num_delta_after_IUP_args[2]
      || num_delta_after_IUP_args[1]
      || num_delta_after_IUP_args[0])
    BCI(SVTCA_x);

  /* emit x DELTA opcodes */
  if (num_delta_after_IUP_args[2])
    BCI(DELTAP3);
  if (num_delta_after_IUP_args[1])
    BCI(DELTAP2);
  if (num_delta_after_IUP_args[0])
    BCI(DELTAP1);

  /* we need to insert a few extra bytecode instructions */
  /* if we have y delta exceptions before IUP */
  if (num_delta_before_IUP_args[5]
      || num_delta_before_IUP_args[4]
      || num_delta_before_IUP_args[3])
  {
    FT_Byte* ins_extra_buf_new;
    FT_Byte ins_extra_len_new;


    ins_extra_len_new = glyph->ins_extra_len
                        + sizeof (ins_extra_delta_exceptions);
    ins_extra_buf_new = (FT_Byte*)realloc(glyph->ins_extra_buf,
                                          ins_extra_len_new);
    if (!ins_extra_buf_new)
    {
      bufp = NULL;
      goto Done;
    }

    /* set `cvtl_do_iup_y' to zero at the beginning of the bytecode */
    /* by activating `ins_extra_delta_exceptions' */
    memcpy(ins_extra_buf_new + glyph->ins_extra_len,
           ins_extra_delta_exceptions,
           sizeof (ins_extra_delta_exceptions));

    glyph->ins_extra_buf = ins_extra_buf_new;
    glyph->ins_extra_len = ins_extra_len_new;

    /* reset `cvtl_do_iup_y' for next glyph */
    BCI(PUSHB_2);
    BCI(cvtl_do_iup_y);
    BCI(100);
    BCI(WCVTP);
  }

Done:
  for (i = 0; i < 6; i++)
  {
    free(delta_before_IUP_args[i]);
    free(delta_after_IUP_args[i]);
  }
  free(args);

  if (num_before_IUP_stack_elements > sfnt->max_stack_elements)
    sfnt->max_stack_elements = num_before_IUP_stack_elements;
  if (num_after_IUP_stack_elements > sfnt->max_stack_elements)
    sfnt->max_stack_elements = num_after_IUP_stack_elements;

  return bufp;
}


static FT_Byte*
TA_sfnt_build_glyph_scaler(SFNT* sfnt,
                           Recorder* recorder,
                           FT_Byte* bufp)
{
  FONT* font = recorder->font;
  FT_GlyphSlot glyph = sfnt->face->glyph;
  FT_Vector* points = glyph->outline.points;
  FT_UInt num_contours = (FT_UInt)glyph->outline.n_contours;

  FT_UInt* args;
  FT_UInt* arg;
  FT_UInt num_args;

  FT_Bool need_words = 0;
  FT_UInt p, q;
  FT_UInt start, end;
  FT_UShort num_storage;
  FT_UShort num_stack_elements;


  num_args = 2 * num_contours + 2;

  /* collect all arguments temporarily in an array (in reverse order) */
  /* so that we can easily split into chunks of 255 args */
  /* as needed by NPUSHB and NPUSHW, respectively */
  args = (FT_UInt*)malloc(num_args * sizeof (FT_UInt));
  if (!args)
    return NULL;

  arg = args + num_args - 1;

  if (num_args > 0xFF)
    need_words = 1;

  if (recorder->glyph->num_components && font->hint_composites)
    *(arg--) = bci_scale_composite_glyph;
  else
    *(arg--) = bci_scale_glyph;
  *(arg--) = num_contours;

  start = 0;
  end = 0;

  for (p = 0; p < num_contours; p++)
  {
    FT_UInt max = start;
    FT_UInt min = start;


    end = (FT_UInt)glyph->outline.contours[p];

    for (q = start; q <= end; q++)
    {
      if (points[q].y < points[min].y)
        min = q;
      if (points[q].y > points[max].y)
        max = q;
    }

    if (min > max)
    {
      *(arg--) = TA_adjust_point_index(recorder, max);
      *(arg--) = TA_adjust_point_index(recorder, min);
    }
    else
    {
      *(arg--) = TA_adjust_point_index(recorder, min);
      *(arg--) = TA_adjust_point_index(recorder, max);
    }

    start = end + 1;
  }

  if (end > 0xFF)
    need_words = 1;

  /* with most fonts it is very rare */
  /* that any of the pushed arguments is larger than 0xFF, */
  /* thus we refrain from further optimizing this case */
  bufp = TA_build_push(bufp, args, num_args, need_words, 1);

  BCI(CALL);

  num_storage = sal_segment_offset;
  if (num_storage > sfnt->max_storage)
    sfnt->max_storage = num_storage;

  num_stack_elements = (FT_UShort)(ADDITIONAL_STACK_ELEMENTS + num_args);
  if (num_stack_elements > sfnt->max_stack_elements)
    sfnt->max_stack_elements = num_stack_elements;

  free(args);

  return bufp;
}


static FT_Byte*
TA_font_build_subglyph_shifter(FONT* font,
                               FT_Byte* bufp)
{
  FT_Face face = font->loader->face;
  FT_GlyphSlot glyph = face->glyph;

  TA_GlyphLoader gloader = font->loader->gloader;

  TA_SubGlyph subglyphs = gloader->current.subglyphs;
  TA_SubGlyph subglyph_limit = subglyphs + gloader->current.num_subglyphs;
  TA_SubGlyph subglyph;

  FT_Int curr_contour = 0;


  for (subglyph = subglyphs; subglyph < subglyph_limit; subglyph++)
  {
    FT_Error error;

    FT_UShort flags = subglyph->flags;
    FT_Pos y_offset = subglyph->arg2;

    FT_Int num_contours;


    /* load subglyph to get the number of contours */
    error = FT_Load_Glyph(face, (FT_UInt)subglyph->index, FT_LOAD_NO_SCALE);
    if (error)
      return NULL;
    num_contours = glyph->outline.n_contours;

    /* nothing to do if there is a point-to-point alignment */
    if (!(flags & FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES))
      goto End;

    /* nothing to do if y offset is zero */
    if (!y_offset)
      goto End;

    /* nothing to do if there are no contours */
    if (!num_contours)
      goto End;

    /* note that calling `FT_Load_Glyph' without FT_LOAD_NO_RECURSE */
    /* ensures that composite subglyphs are represented as simple glyphs */

    if (num_contours > 0xFF
        || curr_contour > 0xFF)
    {
      BCI(PUSHW_2);
      BCI(HIGH(curr_contour));
      BCI(LOW(curr_contour));
      BCI(HIGH(num_contours));
      BCI(LOW(num_contours));
    }
    else
    {
      BCI(PUSHB_2);
      BCI(curr_contour);
      BCI(num_contours);
    }

    /* there are high chances that this value needs PUSHW, */
    /* thus we handle it separately */
    if (y_offset > 0xFF || y_offset < 0)
    {
      BCI(PUSHW_1);
      BCI(HIGH(y_offset));
      BCI(LOW(y_offset));
    }
    else
    {
      BCI(PUSHB_1);
      BCI(y_offset);
    }

    BCI(PUSHB_1);
    BCI(bci_shift_subglyph);
    BCI(CALL);

  End:
    curr_contour += num_contours;
  }

  return bufp;
}


/*
 * The four `ta_ip_*' actions in the `TA_hints_recorder' callback store its
 * data in four arrays (which are simple but waste a lot of memory).  The
 * function below converts them into bytecode.
 *
 * For `ta_ip_before' and `ta_ip_after', the collected points are emitted
 * together with the edge they correspond to.
 *
 * For both `ta_ip_on' and `ta_ip_between', an outer loop is constructed to
 * loop over the edge or edge pairs, respectively, and each edge or edge
 * pair contains an inner loop to emit the correponding points.
 */

static void
TA_build_point_hints(Recorder* recorder,
                     TA_GlyphHints hints)
{
  TA_AxisHints axis = &hints->axis[TA_DIMENSION_VERT];
  TA_Edge edges = axis->edges;

  FT_Byte* p = recorder->hints_record.buf;

  FT_UShort i;
  FT_UShort j;

  FT_UShort prev_edge;
  FT_UShort prev_before_edge;
  FT_UShort prev_after_edge;

  Node1* before_node;
  Node1* after_node;
  Node2* on_node;
  Node3* between_node;


  /* we store everything as 16bit numbers; */
  /* the function numbers (`ta_ip_before', etc.) */
  /* reflect the order in the TA_Action enumeration */

  /* ip_before_points */

  i = 0;
  for (before_node = LLRB_MIN(ip_before_points,
                              &recorder->ip_before_points_head);
       before_node;
       before_node = LLRB_NEXT(ip_before_points,
                               &recorder->ip_before_points_head,
                               before_node))
  {
    /* count points */
    i++;
  }

  if (i)
  {
    TA_Edge edge;
    FT_UShort edge_first_idx;


    recorder->hints_record.num_actions++;

    edge = edges;
    edge_first_idx = TA_get_segment_index(edge->first, recorder);

    *(p++) = 0;
    *(p++) = (FT_Byte)ta_ip_before + ACTION_OFFSET;
    *(p++) = HIGH(edge_first_idx);
    *(p++) = LOW(edge_first_idx);
    *(p++) = HIGH(i);
    *(p++) = LOW(i);

    for (before_node = LLRB_MIN(ip_before_points,
                                &recorder->ip_before_points_head);
         before_node;
         before_node = LLRB_NEXT(ip_before_points,
                                 &recorder->ip_before_points_head,
                                 before_node))
    {
      FT_UInt point;


      point = TA_adjust_point_index(recorder, before_node->point);
      *(p++) = HIGH(point);
      *(p++) = LOW(point);
    }
  }

  /* ip_after_points */

  i = 0;
  for (after_node = LLRB_MIN(ip_after_points,
                             &recorder->ip_after_points_head);
       after_node;
       after_node = LLRB_NEXT(ip_after_points,
                              &recorder->ip_after_points_head,
                              after_node))
  {
    /* count points */
    i++;
  }

  if (i)
  {
    TA_Edge edge;
    FT_UShort edge_first_idx;


    recorder->hints_record.num_actions++;

    edge = edges + axis->num_edges - 1;
    edge_first_idx = TA_get_segment_index(edge->first, recorder);

    *(p++) = 0;
    *(p++) = (FT_Byte)ta_ip_after + ACTION_OFFSET;
    *(p++) = HIGH(edge_first_idx);
    *(p++) = LOW(edge_first_idx);
    *(p++) = HIGH(i);
    *(p++) = LOW(i);

    for (after_node = LLRB_MIN(ip_after_points,
                               &recorder->ip_after_points_head);
         after_node;
         after_node = LLRB_NEXT(ip_after_points,
                                &recorder->ip_after_points_head,
                                after_node))
    {
      FT_UInt point;


      point = TA_adjust_point_index(recorder, after_node->point);
      *(p++) = HIGH(point);
      *(p++) = LOW(point);
    }
  }

  /* ip_on_point_array */

  prev_edge = 0xFFFF;
  i = 0;
  for (on_node = LLRB_MIN(ip_on_points,
                          &recorder->ip_on_points_head);
       on_node;
       on_node = LLRB_NEXT(ip_on_points,
                           &recorder->ip_on_points_head,
                           on_node))
  {
    /* count edges */
    if (on_node->edge != prev_edge)
    {
      i++;
      prev_edge = on_node->edge;
    }
  }

  if (i)
  {
    recorder->hints_record.num_actions++;

    *(p++) = 0;
    *(p++) = (FT_Byte)ta_ip_on + ACTION_OFFSET;
    *(p++) = HIGH(i);
    *(p++) = LOW(i);

    for (on_node = LLRB_MIN(ip_on_points,
                            &recorder->ip_on_points_head);
         on_node;
         on_node = LLRB_NEXT(ip_on_points,
                             &recorder->ip_on_points_head,
                             on_node))
    {
      Node2* edge_node;
      TA_Edge edge;
      FT_UShort edge_first_idx;


      edge = edges + on_node->edge;
      edge_first_idx = TA_get_segment_index(edge->first, recorder);

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);

      /* save current position */
      edge_node = on_node;
      j = 0;
      for (;
           on_node;
           on_node = LLRB_NEXT(ip_on_points,
                               &recorder->ip_on_points_head,
                               on_node))
      {
        /* count points on current edge */
        if (on_node->edge != edge_node->edge)
          break;
        j++;
      }

      *(p++) = HIGH(j);
      *(p++) = LOW(j);

      /* restore current position */
      on_node = edge_node;
      for (;
           on_node;
           on_node = LLRB_NEXT(ip_on_points,
                               &recorder->ip_on_points_head,
                               on_node))
      {
        FT_UInt point;


        if (on_node->edge != edge_node->edge)
          break;

        point = TA_adjust_point_index(recorder, on_node->point);
        *(p++) = HIGH(point);
        *(p++) = LOW(point);

        /* keep track of previous node */
        edge_node = on_node;
      }

      /* reset loop iterator by one element, then continue */
      on_node = edge_node;
    }
  }

  /* ip_between_point_array */

  prev_before_edge = 0xFFFF;
  prev_after_edge = 0xFFFF;
  i = 0;
  for (between_node = LLRB_MIN(ip_between_points,
                               &recorder->ip_between_points_head);
       between_node;
       between_node = LLRB_NEXT(ip_between_points,
                                &recorder->ip_between_points_head,
                                between_node))
  {
    /* count `(before,after)' edge pairs */
    if (between_node->before_edge != prev_before_edge
        || between_node->after_edge != prev_after_edge)
    {
      i++;
      prev_before_edge = between_node->before_edge;
      prev_after_edge = between_node->after_edge;
    }
  }

  if (i)
  {
    recorder->hints_record.num_actions++;

    *(p++) = 0;
    *(p++) = (FT_Byte)ta_ip_between + ACTION_OFFSET;
    *(p++) = HIGH(i);
    *(p++) = LOW(i);

    for (between_node = LLRB_MIN(ip_between_points,
                                 &recorder->ip_between_points_head);
         between_node;
         between_node = LLRB_NEXT(ip_between_points,
                                  &recorder->ip_between_points_head,
                                  between_node))
    {
      Node3* edge_pair_node;
      TA_Edge before;
      TA_Edge after;
      FT_UShort before_first_idx;
      FT_UShort after_first_idx;


      before = edges + between_node->before_edge;
      after = edges + between_node->after_edge;
      before_first_idx = TA_get_segment_index(before->first, recorder);
      after_first_idx = TA_get_segment_index(after->first, recorder);

      *(p++) = HIGH(after_first_idx);
      *(p++) = LOW(after_first_idx);
      *(p++) = HIGH(before_first_idx);
      *(p++) = LOW(before_first_idx);

      /* save current position */
      edge_pair_node = between_node;
      j = 0;
      for (;
           between_node;
           between_node = LLRB_NEXT(ip_between_points,
                                    &recorder->ip_between_points_head,
                                    between_node))
      {
        /* count points associated with current edge pair */
        if (between_node->before_edge != edge_pair_node->before_edge
            || between_node->after_edge != edge_pair_node->after_edge)
          break;
        j++;
      }

      *(p++) = HIGH(j);
      *(p++) = LOW(j);

      /* restore current position */
      between_node = edge_pair_node;
      for (;
           between_node;
           between_node = LLRB_NEXT(ip_between_points,
                                    &recorder->ip_between_points_head,
                                    between_node))
      {
        FT_UInt point;


        if (between_node->before_edge != edge_pair_node->before_edge
            || between_node->after_edge != edge_pair_node->after_edge)
          break;

        point = TA_adjust_point_index(recorder, between_node->point);
        *(p++) = HIGH(point);
        *(p++) = LOW(point);

        /* keep track of previous node */
        edge_pair_node = between_node;
      }

      /* reset loop iterator by one element, then continue */
      between_node = edge_pair_node;
    }
  }

  recorder->hints_record.buf = p;
}


static FT_Bool
TA_hints_record_is_different(Hints_Record* hints_records,
                             FT_UInt num_hints_records,
                             FT_Byte* start,
                             FT_Byte* end)
{
  Hints_Record last_hints_record;


  if (!hints_records)
    return 1;

  /* we only need to compare with the last hints record */
  last_hints_record = hints_records[num_hints_records - 1];

  if ((FT_UInt)(end - start) != last_hints_record.buf_len)
    return 1;

  if (memcmp(start, last_hints_record.buf, last_hints_record.buf_len))
    return 1;

  return 0;
}


static FT_Error
TA_add_hints_record(Hints_Record** hints_records,
                    FT_UInt* num_hints_records,
                    FT_Byte* start,
                    Hints_Record hints_record)
{
  Hints_Record* hints_records_new;
  FT_UInt buf_len;
  /* at this point, `hints_record.buf' still points into `ins_buf' */
  FT_Byte* end = hints_record.buf;


  buf_len = (FT_UInt)(end - start);

  /* now fill the structure completely */
  hints_record.buf_len = buf_len;
  hints_record.buf = (FT_Byte*)malloc(buf_len);
  if (!hints_record.buf)
    return FT_Err_Out_Of_Memory;

  memcpy(hints_record.buf, start, buf_len);

  (*num_hints_records)++;
  hints_records_new =
    (Hints_Record*)realloc(*hints_records, *num_hints_records
                                           * sizeof (Hints_Record));
  if (!hints_records_new)
  {
    free(hints_record.buf);
    (*num_hints_records)--;
    return FT_Err_Out_Of_Memory;
  }
  else
    *hints_records = hints_records_new;

  (*hints_records)[*num_hints_records - 1] = hints_record;

  return FT_Err_Ok;
}


static FT_Byte*
TA_emit_hints_record(Recorder* recorder,
                     Hints_Record* hints_record,
                     FT_Byte* bufp,
                     FT_Bool optimize)
{
  FT_Byte* p;
  FT_Byte* endp;
  FT_Bool need_words = 0;

  FT_UInt i, j;
  FT_UInt num_arguments;
  FT_UInt num_args;


  /* check whether any argument is larger than 0xFF */
  endp = hints_record->buf + hints_record->buf_len;
  for (p = hints_record->buf; p < endp; p += 2)
    if (*p)
    {
      need_words = 1;
      break;
    }

  /* with most fonts it is very rare */
  /* that any of the pushed arguments is larger than 0xFF, */
  /* thus we refrain from further optimizing this case */

  num_arguments = hints_record->buf_len / 2;
  p = endp - 2;

  if (need_words)
  {
    for (i = 0; i < num_arguments; i += 255)
    {
      num_args = (num_arguments - i > 255) ? 255 : (num_arguments - i);

      if (optimize && num_args <= 8)
        BCI(PUSHW_1 - 1 + num_args);
      else
      {
        BCI(NPUSHW);
        BCI(num_args);
      }
      for (j = 0; j < num_args; j++)
      {
        BCI(*p);
        BCI(*(p + 1));
        p -= 2;
      }
    }
  }
  else
  {
    /* we only need the lower bytes */
    p++;

    for (i = 0; i < num_arguments; i += 255)
    {
      num_args = (num_arguments - i > 255) ? 255 : (num_arguments - i);

      if (optimize && num_args <= 8)
        BCI(PUSHB_1 - 1 + num_args);
      else
      {
        BCI(NPUSHB);
        BCI(num_args);
      }
      for (j = 0; j < num_args; j++)
      {
        BCI(*p);
        p -= 2;
      }
    }
  }

  /* collect stack depth data */
  if (num_arguments > recorder->num_stack_elements)
    recorder->num_stack_elements = (FT_UShort)num_arguments;

  return bufp;
}


static FT_Byte*
TA_emit_hints_records(Recorder* recorder,
                      Hints_Record* hints_records,
                      FT_UInt num_hints_records,
                      FT_Byte* bufp,
                      FT_Bool optimize)
{
  FT_UInt i;
  Hints_Record* hints_record;


  hints_record = hints_records;

  /* emit hints records in `if' clauses, */
  /* with the ppem size as the condition */
  for (i = 0; i < num_hints_records - 1; i++)
  {
    BCI(MPPEM);
    if (hints_record->size > 0xFF)
    {
      BCI(PUSHW_1);
      BCI(HIGH((hints_record + 1)->size));
      BCI(LOW((hints_record + 1)->size));
    }
    else
    {
      BCI(PUSHB_1);
      BCI((hints_record + 1)->size);
    }
    BCI(LT);
    BCI(IF);
    bufp = TA_emit_hints_record(recorder, hints_record, bufp, optimize);
    BCI(ELSE);

    hints_record++;
  }

  bufp = TA_emit_hints_record(recorder, hints_record, bufp, optimize);

  for (i = 0; i < num_hints_records - 1; i++)
    BCI(EIF);

  return bufp;
}


static void
TA_free_hints_records(Hints_Record* hints_records,
                      FT_UInt num_hints_records)
{
  FT_UInt i;


  for (i = 0; i < num_hints_records; i++)
    free(hints_records[i].buf);

  free(hints_records);
}


static FT_Byte*
TA_hints_recorder_handle_segments(FT_Byte* bufp,
                                  Recorder* recorder,
                                  TA_Edge edge,
                                  FT_UShort* wraps)
{
  TA_Segment seg;
  FT_UShort seg_idx;
  FT_UShort num_segs = 0;
  FT_UShort* wrap;
  FT_UShort num_segments;


  seg_idx = TA_get_segment_index(edge->first, recorder);
  num_segments = TA_get_segment_index(NULL, recorder);

  /* we store everything as 16bit numbers */
  *(bufp++) = HIGH(seg_idx);
  *(bufp++) = LOW(seg_idx);

  /* wrap-around segments are stored as two segments */
  if (edge->first->first > edge->first->last)
    num_segs++;

  seg = edge->first->edge_next;
  while (seg != edge->first)
  {
    num_segs++;

    if (seg->first > seg->last)
      num_segs++;

    seg = seg->edge_next;
  }

  *(bufp++) = HIGH(num_segs);
  *(bufp++) = LOW(num_segs);

  if (edge->first->first > edge->first->last)
  {
    /* emit second part of wrap-around segment; */
    /* the bytecode positions such segments after `normal' ones */
    wrap = wraps;
    for (;;)
    {
      if (seg_idx == *wrap)
        break;
      wrap++;
    }

    *(bufp++) = HIGH(num_segments + (wrap - wraps));
    *(bufp++) = LOW(num_segments + (wrap - wraps));
  }

  seg = edge->first->edge_next;
  while (seg != edge->first)
  {
    seg_idx = TA_get_segment_index(seg, recorder);

    *(bufp++) = HIGH(seg_idx);
    *(bufp++) = LOW(seg_idx);

    if (seg->first > seg->last)
    {
      wrap = wraps;
      for (;;)
      {
        if (seg_idx == *wrap)
          break;
        wrap++;
      }

      *(bufp++) = HIGH(num_segments + (wrap - wraps));
      *(bufp++) = LOW(num_segments + (wrap - wraps));
    }

    seg = seg->edge_next;
  }

  return bufp;
}


static void
TA_hints_recorder(TA_Action action,
                  TA_GlyphHints hints,
                  TA_Dimension dim,
                  void* arg1,
                  TA_Edge arg2,
                  TA_Edge arg3,
                  TA_Edge lower_bound,
                  TA_Edge upper_bound)
{
  TA_AxisHints axis = &hints->axis[dim];
  TA_Edge edges = axis->edges;
  TA_Point points = hints->points;

  TA_Segment segments = axis->segments;
  TA_Segment seg_limit = segments + axis->num_segments;
  TA_Segment seg;

  Recorder* recorder = (Recorder*)hints->user;
  SFNT* sfnt = recorder->sfnt;
  FONT* font = recorder->font;
  FT_UShort* wraps = recorder->wrap_around_segments;
  FT_Byte* p = recorder->hints_record.buf;

  TA_StyleClass style_class = font->loader->metrics->style_class;
  TA_ScriptClass script_class = ta_script_classes[style_class->script];

  FT_UInt style = style_class->style;
  FT_Bool top_to_bottom_hinting = script_class->top_to_bottom_hinting;


  if (dim == TA_DIMENSION_HORZ)
    return;

  if (!recorder->segment_map_initialized)
  {
    FT_UShort* segment_map = recorder->segment_map;
    FT_UShort idx;


    idx = 0;
    for (seg = segments; seg < seg_limit; seg++)
    {
      if (seg->edge)
        *segment_map = idx++;
      else
        *segment_map = 0xFFFF;

      segment_map++;
    }
    *segment_map = idx;

    recorder->segment_map_initialized = 1;
  }

  if (!recorder->wrap_around_segments_initialized)
  {
    FT_UShort* wrap_around_segment;


    wrap_around_segment = recorder->wrap_around_segments;
    for (seg = segments; seg < seg_limit; seg++)
      if (seg->first > seg->last)
        *(wrap_around_segment++) = TA_get_segment_index(seg, recorder);

    recorder->wrap_around_segments_initialized = 1;
  }

  /* we collect point hints for later processing */
  switch (action)
  {
  case ta_ip_before:
    {
      Node1* before_node;
      TA_Point point = (TA_Point)arg1;


      before_node = (Node1*)malloc(sizeof (Node1));
      if (!before_node)
        return;
      before_node->point = (FT_UShort)(point - points);

      LLRB_INSERT(ip_before_points,
                  &recorder->ip_before_points_head,
                  before_node);
    }
    return;

  case ta_ip_after:
    {
      Node1* after_node;
      TA_Point point = (TA_Point)arg1;


      after_node = (Node1*)malloc(sizeof (Node1));
      if (!after_node)
        return;
      after_node->point = (FT_UShort)(point - points);

      LLRB_INSERT(ip_after_points,
                  &recorder->ip_after_points_head,
                  after_node);
    }
    return;

  case ta_ip_on:
    {
      Node2* on_node;
      TA_Point point = (TA_Point)arg1;
      TA_Edge edge = arg2;


      on_node = (Node2*)malloc(sizeof (Node2));
      if (!on_node)
        return;
      on_node->edge = (FT_UShort)(edge - edges);
      on_node->point = (FT_UShort)(point - points);

      LLRB_INSERT(ip_on_points,
                  &recorder->ip_on_points_head,
                  on_node);
    }
    return;

  case ta_ip_between:
    {
      Node3* between_node;
      TA_Point point = (TA_Point)arg1;
      TA_Edge before = arg2;
      TA_Edge after = arg3;


      between_node = (Node3*)malloc(sizeof (Node3));
      if (!between_node)
        return;
      between_node->before_edge = (FT_UShort)(before - edges);
      between_node->after_edge = (FT_UShort)(after - edges);
      between_node->point = (FT_UShort)(point - points);

      LLRB_INSERT(ip_between_points,
                  &recorder->ip_between_points_head,
                  between_node);
    }
    return;

  case ta_bound:
    /* we ignore the BOUND action since we signal this information */
    /* with the proper function number */
    return;

  default:
    break;
  }

  /* some enum values correspond to 4, 7, 8, or 12 bytecode functions; */
  /* if the value is n, the function numbers are n, ..., n+11, */
  /* to be differentiated with flags */

  switch (action)
  {
  case ta_link:
    {
      TA_Edge base_edge = (TA_Edge)arg1;
      TA_Edge stem_edge = arg2;
      FT_UShort base_first_idx;
      FT_UShort stem_first_idx;


      base_first_idx = TA_get_segment_index(base_edge->first, recorder);
      stem_first_idx = TA_get_segment_index(stem_edge->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + ((stem_edge->flags & TA_EDGE_SERIF) != 0)
               + 2 * ((base_edge->flags & TA_EDGE_ROUND) != 0);

      *(p++) = HIGH(base_first_idx);
      *(p++) = LOW(base_first_idx);
      *(p++) = HIGH(stem_first_idx);
      *(p++) = LOW(stem_first_idx);

      p = TA_hints_recorder_handle_segments(p, recorder, stem_edge, wraps);
    }
    break;

  case ta_anchor:
    {
      TA_Edge edge = (TA_Edge)arg1;
      TA_Edge edge2 = arg2;
      FT_UShort edge_first_idx;
      FT_UShort edge2_first_idx;


      edge_first_idx = TA_get_segment_index(edge->first, recorder);
      edge2_first_idx = TA_get_segment_index(edge2->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + ((edge2->flags & TA_EDGE_SERIF) != 0)
               + 2 * ((edge->flags & TA_EDGE_ROUND) != 0);

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);
      *(p++) = HIGH(edge2_first_idx);
      *(p++) = LOW(edge2_first_idx);

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
    }
    break;

  case ta_adjust:
    {
      TA_Edge edge = (TA_Edge)arg1;
      TA_Edge edge2 = arg2;
      TA_Edge edge_minus_one = lower_bound;
      FT_UShort edge_first_idx;
      FT_UShort edge2_first_idx;


      edge_first_idx = TA_get_segment_index(edge->first, recorder);
      edge2_first_idx = TA_get_segment_index(edge2->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + ((edge2->flags & TA_EDGE_SERIF) != 0)
               + 2 * ((edge->flags & TA_EDGE_ROUND) != 0)
               + 4 * (edge_minus_one != NULL) /* `bound' */
               + 4 * (edge_minus_one != NULL
                      && top_to_bottom_hinting); /* `down' */

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);
      *(p++) = HIGH(edge2_first_idx);
      *(p++) = LOW(edge2_first_idx);

      if (edge_minus_one)
      {
        FT_UShort edge_minus_one_first_idx;


        edge_minus_one_first_idx = TA_get_segment_index(
                                     edge_minus_one->first, recorder);

        *(p++) = HIGH(edge_minus_one_first_idx);
        *(p++) = LOW(edge_minus_one_first_idx);
      }

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
    }
    break;

  case ta_blue_anchor:
    {
      TA_Edge edge = (TA_Edge)arg1;
      TA_Edge blue = arg2;
      FT_UShort blue_first_idx;
      FT_UShort edge_first_idx;


      blue_first_idx = TA_get_segment_index(blue->first, recorder);
      edge_first_idx = TA_get_segment_index(edge->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET;

      *(p++) = HIGH(blue_first_idx);
      *(p++) = LOW(blue_first_idx);

      if (edge->best_blue_is_shoot)
      {
        *(p++) = HIGH(CVT_BLUE_SHOOTS_OFFSET(style) + edge->best_blue_idx);
        *(p++) = LOW(CVT_BLUE_SHOOTS_OFFSET(style) + edge->best_blue_idx);
      }
      else
      {
        *(p++) = HIGH(CVT_BLUE_REFS_OFFSET(style) + edge->best_blue_idx);
        *(p++) = LOW(CVT_BLUE_REFS_OFFSET(style) + edge->best_blue_idx);
      }

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
    }
    break;

  case ta_stem:
    {
      TA_Edge edge = (TA_Edge)arg1;
      TA_Edge edge2 = arg2;
      TA_Edge edge_minus_one = lower_bound;
      FT_UShort edge_first_idx;
      FT_UShort edge2_first_idx;


      edge_first_idx = TA_get_segment_index(edge->first, recorder);
      edge2_first_idx = TA_get_segment_index(edge2->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + ((edge2->flags & TA_EDGE_SERIF) != 0)
               + 2 * ((edge->flags & TA_EDGE_ROUND) != 0)
               + 4 * (edge_minus_one != NULL) /* `bound' */
               + 4 * (edge_minus_one != NULL
                      && top_to_bottom_hinting); /* `down' */

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);
      *(p++) = HIGH(edge2_first_idx);
      *(p++) = LOW(edge2_first_idx);

      if (edge_minus_one)
      {
        FT_UShort edge_minus_one_first_idx;


        edge_minus_one_first_idx = TA_get_segment_index(
                                     edge_minus_one->first, recorder);

        *(p++) = HIGH(edge_minus_one_first_idx);
        *(p++) = LOW(edge_minus_one_first_idx);
      }

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
      p = TA_hints_recorder_handle_segments(p, recorder, edge2, wraps);
    }
    break;

  case ta_blue:
    {
      TA_Edge edge = (TA_Edge)arg1;
      FT_UShort edge_first_idx;


      edge_first_idx = TA_get_segment_index(edge->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET;

      if (edge->best_blue_is_shoot)
      {
        *(p++) = HIGH(CVT_BLUE_SHOOTS_OFFSET(style) + edge->best_blue_idx);
        *(p++) = LOW(CVT_BLUE_SHOOTS_OFFSET(style) + edge->best_blue_idx);
      }
      else
      {
        *(p++) = HIGH(CVT_BLUE_REFS_OFFSET(style) + edge->best_blue_idx);
        *(p++) = LOW(CVT_BLUE_REFS_OFFSET(style) + edge->best_blue_idx);
      }

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
    }
    break;

  case ta_serif:
    {
      TA_Edge serif = (TA_Edge)arg1;
      TA_Edge base = serif->serif;
      FT_UShort serif_first_idx;
      FT_UShort base_first_idx;


      serif_first_idx = TA_get_segment_index(serif->first, recorder);
      base_first_idx = TA_get_segment_index(base->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + (lower_bound != NULL)
               + 2 * (upper_bound != NULL)
               + 3 * ((lower_bound != NULL || upper_bound != NULL)
                      && top_to_bottom_hinting); /* `down' */

      *(p++) = HIGH(serif_first_idx);
      *(p++) = LOW(serif_first_idx);
      *(p++) = HIGH(base_first_idx);
      *(p++) = LOW(base_first_idx);

      if (lower_bound)
      {
        FT_UShort lower_bound_first_idx;


        lower_bound_first_idx = TA_get_segment_index(lower_bound->first,
                                                     recorder);

        *(p++) = HIGH(lower_bound_first_idx);
        *(p++) = LOW(lower_bound_first_idx);
      }
      if (upper_bound)
      {
        FT_UShort upper_bound_first_idx;


        upper_bound_first_idx = TA_get_segment_index(upper_bound->first,
                                                     recorder);

        *(p++) = HIGH(upper_bound_first_idx);
        *(p++) = LOW(upper_bound_first_idx);
      }

      p = TA_hints_recorder_handle_segments(p, recorder, serif, wraps);
    }
    break;

  case ta_serif_anchor:
  case ta_serif_link2:
    {
      TA_Edge edge = (TA_Edge)arg1;
      FT_UShort edge_first_idx;


      edge_first_idx = TA_get_segment_index(edge->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + (lower_bound != NULL)
               + 2 * (upper_bound != NULL)
               + 3 * ((lower_bound != NULL || upper_bound != NULL)
                      && top_to_bottom_hinting); /* `down' */

      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);

      if (lower_bound)
      {
        FT_UShort lower_bound_first_idx;


        lower_bound_first_idx = TA_get_segment_index(lower_bound->first,
                                                     recorder);

        *(p++) = HIGH(lower_bound_first_idx);
        *(p++) = LOW(lower_bound_first_idx);
      }
      if (upper_bound)
      {
        FT_UShort upper_bound_first_idx;


        upper_bound_first_idx = TA_get_segment_index(upper_bound->first,
                                                     recorder);

        *(p++) = HIGH(upper_bound_first_idx);
        *(p++) = LOW(upper_bound_first_idx);
      }

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
    }
    break;

  case ta_serif_link1:
    {
      TA_Edge edge = (TA_Edge)arg1;
      TA_Edge before = arg2;
      TA_Edge after = arg3;
      FT_UShort before_first_idx;
      FT_UShort edge_first_idx;
      FT_UShort after_first_idx;


      before_first_idx = TA_get_segment_index(before->first, recorder);
      edge_first_idx = TA_get_segment_index(edge->first, recorder);
      after_first_idx = TA_get_segment_index(after->first, recorder);

      *(p++) = 0;
      *(p++) = (FT_Byte)action + ACTION_OFFSET
               + (lower_bound != NULL)
               + 2 * (upper_bound != NULL)
               + 3 * ((lower_bound != NULL || upper_bound != NULL)
                      && top_to_bottom_hinting); /* `down' */

      *(p++) = HIGH(before_first_idx);
      *(p++) = LOW(before_first_idx);
      *(p++) = HIGH(edge_first_idx);
      *(p++) = LOW(edge_first_idx);
      *(p++) = HIGH(after_first_idx);
      *(p++) = LOW(after_first_idx);

      if (lower_bound)
      {
        FT_UShort lower_bound_first_idx;


        lower_bound_first_idx = TA_get_segment_index(lower_bound->first,
                                                     recorder);

        *(p++) = HIGH(lower_bound_first_idx);
        *(p++) = LOW(lower_bound_first_idx);
      }
      if (upper_bound)
      {
        FT_UShort upper_bound_first_idx;


        upper_bound_first_idx = TA_get_segment_index(upper_bound->first,
                                                     recorder);

        *(p++) = HIGH(upper_bound_first_idx);
        *(p++) = LOW(upper_bound_first_idx);
      }

      p = TA_hints_recorder_handle_segments(p, recorder, edge, wraps);
    }
    break;

  default:
    /* there are more cases in the enumeration */
    /* which are handled with flags */
    break;
  }

  recorder->hints_record.num_actions++;
  recorder->hints_record.buf = p;
}


static FT_Error
TA_init_recorder(Recorder* recorder,
                 SFNT* sfnt,
                 FONT* font,
                 GLYPH* glyph,
                 TA_GlyphHints hints)
{
  TA_AxisHints axis = &hints->axis[TA_DIMENSION_VERT];

  TA_Segment segments = axis->segments;
  TA_Segment seg_limit = segments + axis->num_segments;
  TA_Segment seg;


  recorder->sfnt = sfnt;
  recorder->font = font;
  recorder->glyph = glyph;

  LLRB_INIT(&recorder->ip_before_points_head);
  LLRB_INIT(&recorder->ip_after_points_head);
  LLRB_INIT(&recorder->ip_on_points_head);
  LLRB_INIT(&recorder->ip_between_points_head);

  recorder->num_stack_elements = 0;

  /* no need to clean up allocated arrays in case of error; */
  /* this is handled later by `TA_free_recorder' */

  /* we use segment_map[axis->num_segments] */
  /* as the total number of mapped segments, so allocate one more element */
  recorder->segment_map =
    (FT_UShort*)malloc((size_t)(axis->num_segments + 1) * sizeof (FT_UShort));
  if (!recorder->segment_map)
    return FT_Err_Out_Of_Memory;

  /* `segment_map' gets initialized later on, */
  /* after the first call of `ta_loader_load_glyph' */
  recorder->segment_map_initialized = 0;

  recorder->num_wrap_around_segments = 0;
  for (seg = segments; seg < seg_limit; seg++)
    if (seg->first > seg->last)
      recorder->num_wrap_around_segments++;

  recorder->wrap_around_segments =
    (FT_UShort*)malloc(recorder->num_wrap_around_segments
                       * sizeof (FT_UShort));
  if (!recorder->wrap_around_segments)
    return FT_Err_Out_Of_Memory;

  /* `wrap_around_segments' gets initialized later on; */
  /* it needs function `TA_get_segment_index' which uses data */
  /* that hasn't been initialized yet either */
  recorder->wrap_around_segments_initialized = 0;

  return FT_Err_Ok;
}


static void
TA_reset_recorder(Recorder* recorder,
                  FT_Byte* bufp)
{
  recorder->hints_record.buf = bufp;
  recorder->hints_record.num_actions = 0;
}


static void
TA_rewind_recorder(Recorder* recorder,
                   FT_Byte* bufp,
                   FT_UInt size)
{
  Node1* before_node;
  Node1* after_node;
  Node2* on_node;
  Node3* between_node;

  Node1* next_before_node;
  Node1* next_after_node;
  Node2* next_on_node;
  Node3* next_between_node;


  TA_reset_recorder(recorder, bufp);

  recorder->hints_record.size = size;

  /* deallocate our red-black trees */

  for (before_node = LLRB_MIN(ip_before_points,
                              &recorder->ip_before_points_head);
       before_node;
       before_node = next_before_node)
  {
    next_before_node = LLRB_NEXT(ip_before_points,
                                 &recorder->ip_before_points_head,
                                 before_node);
    LLRB_REMOVE(ip_before_points,
                &recorder->ip_before_points_head,
                before_node);
    free(before_node);
  }

  for (after_node = LLRB_MIN(ip_after_points,
                             &recorder->ip_after_points_head);
       after_node;
       after_node = next_after_node)
  {
    next_after_node = LLRB_NEXT(ip_after_points,
                                &recorder->ip_after_points_head,
                                after_node);
    LLRB_REMOVE(ip_after_points,
                &recorder->ip_after_points_head,
                after_node);
    free(after_node);
  }

  for (on_node = LLRB_MIN(ip_on_points,
                          &recorder->ip_on_points_head);
       on_node;
       on_node = next_on_node)
  {
    next_on_node = LLRB_NEXT(ip_on_points,
                             &recorder->ip_on_points_head,
                             on_node);
    LLRB_REMOVE(ip_on_points,
                &recorder->ip_on_points_head,
                on_node);
    free(on_node);
  }

  for (between_node = LLRB_MIN(ip_between_points,
                               &recorder->ip_between_points_head);
       between_node;
       between_node = next_between_node)
  {
    next_between_node = LLRB_NEXT(ip_between_points,
                                  &recorder->ip_between_points_head,
                                  between_node);
    LLRB_REMOVE(ip_between_points,
                &recorder->ip_between_points_head,
                between_node);
    free(between_node);
  }
}


static void
TA_free_recorder(Recorder* recorder)
{
  free(recorder->segment_map);
  free(recorder->wrap_around_segments);

  TA_rewind_recorder(recorder, NULL, 0);
}


FT_Error
TA_sfnt_build_glyph_instructions(SFNT* sfnt,
                                 FONT* font,
                                 FT_Long idx)
{
  FT_Face face = sfnt->face;
  FT_Error error;

  FT_Byte* ins_buf;
  FT_UInt ins_len;
  FT_Byte* bufp;
  FT_Byte* p;

  SFNT_Table* glyf_table = &font->tables[sfnt->glyf_idx];
  glyf_Data* data = (glyf_Data*)glyf_table->data;
  /* `idx' is never negative */
  GLYPH* glyph = &data->glyphs[idx];

  TA_FaceGlobals globals = (TA_FaceGlobals)sfnt->face->autohint.data;
  FT_UShort* gstyles = globals->glyph_styles;
  FT_Bool use_gstyle_data = 1;

  TA_GlyphHints hints;

  FT_UInt num_action_hints_records = 0;
  FT_UInt num_point_hints_records = 0;
  Hints_Record* action_hints_records = NULL;
  Hints_Record* point_hints_records = NULL;

  Recorder recorder;
  FT_UShort num_stack_elements;
  FT_Bool optimize = 0;

  FT_Int32 load_flags;
  FT_UInt size;

  /* we store only three positions, but it simplifies the algorithm in */
  /* `TA_optimize_push' if we have one additional element */
  FT_Byte* pos[4];

#ifdef TA_DEBUG
  int _ta_debug_save;
#endif


  /* XXX: right now, we abuse this flag to control */
  /*      the global behaviour of the auto-hinter */
  load_flags = 1 << 29; /* vertical hinting only */
  if (!font->adjust_subglyphs)
  {
    if (font->hint_composites)
      load_flags |= FT_LOAD_NO_SCALE;
    else
      load_flags |= FT_LOAD_NO_RECURSE;
  }

  /* computing the segments is resolution independent, */
  /* thus the pixel size in this call is arbitrary -- */
  /* however, we avoid unnecessary debugging output */
  /* if we use the lowest value of the hinting range */
  error = FT_Set_Pixel_Sizes(face,
                             font->hinting_range_min,
                             font->hinting_range_min);
  if (error)
    return error;

  /* this data is needed for `ta_glyph_hints_reload' (in file `tahints.c') */
  /* to modify `out' directions of points at the user's request */
  /* (which will eventually become single-point segments) */
  error = TA_control_segment_dir_collect(font, face->face_index, idx);
  if (error)
    return error;

#ifdef TA_DEBUG
  /* temporarily disable some debugging output */
  /* to avoid getting the information twice */
  _ta_debug_save = _ta_debug;
  _ta_debug = 0;
#endif

  ta_loader_register_hints_recorder(font->loader, NULL, NULL);
  error = ta_loader_load_glyph(font, face, (FT_UInt)idx, load_flags);

#ifdef TA_DEBUG
  _ta_debug = _ta_debug_save;
#endif

  if (error)
    return error;

  /* do nothing if we have an empty glyph */
  if (!(font->loader->gloader->current.num_subglyphs
        || face->glyph->outline.n_contours))
    return FT_Err_Ok;

  hints = &font->loader->hints;

  /*
   * We allocate a buffer which is certainly large enough
   * to hold all of the created bytecode instructions;
   * later on it gets reallocated to its real size.
   *
   * The value `1000' is a very rough guess, not tested well.
   *
   * For delta exceptions, we have three DELTA commands,
   * covering 3*16 ppem values.
   * Since a point index can be larger than 255,
   * we assume two bytes everywhere for the necessary PUSH calls.
   * This value must be doubled for the other arguments of DELTA.
   * Additionally, we have both x and y deltas,
   * which need to be handled separately in the bytecode.
   * In summary, this is approx. 3*16 * 2*2 * 2 = 400 bytes per point,
   * adding some bytes for the necessary overhead.
   */
  ins_len = (FT_UInt)(hints->num_points
                      * (1000
                         + ((font->control_data_head != NULL) ? 400 : 0)));
  ins_buf = (FT_Byte*)malloc(ins_len);
  if (!ins_buf)
    return FT_Err_Out_Of_Memory;

  /* handle composite glyph */
  if (font->loader->gloader->current.num_subglyphs)
  {
    bufp = TA_font_build_subglyph_shifter(font, ins_buf);
    if (!bufp)
    {
      error = FT_Err_Out_Of_Memory;
      goto Err;
    }

    use_gstyle_data = 0;

    goto Done1;
  }


  if (font->fallback_scaling)
  {
    if (font->loader->metrics->style_class == &ta_none_dflt_style_class)
    {
      /* the scaling value of `none_dflt' */
      /* (this is, hinting without script-specific blue zones) */
      /* is always 1, which corresponds to a no-op */
      bufp = ins_buf;
    }
    else
    {
      /* since `TA_init_recorder' hasn't been called yet, */
      /* we manually initialize the `sfnt', `font', and `glyph' fields */
      recorder.sfnt = sfnt;
      recorder.font = font;
      recorder.glyph = glyph;

      bufp = TA_sfnt_build_glyph_scaler(sfnt, &recorder, ins_buf);
      if (!bufp)
      {
        error = FT_Err_Out_Of_Memory;
        goto Err;
      }
    }

    use_gstyle_data = 0;

    goto Done1;
  }

  error = TA_init_recorder(&recorder, sfnt, font, glyph, hints);
  if (error)
    goto Err;

  /* loop over a large range of pixel sizes */
  /* to find hints records which get pushed onto the bytecode stack */

#ifdef DEBUGGING
  if (font->debug)
  {
    int num_chars, i;
    char buf[256];


    (void)FT_Get_Glyph_Name(face, (FT_UInt)idx, buf, 256);

    num_chars = fprintf(stderr, "glyph %ld", idx);
    if (*buf)
      num_chars += fprintf(stderr, " (%s)", buf);
    fprintf(stderr, "\n");
    for (i = 0; i < num_chars; i++)
      putc('=', stderr);
    fprintf(stderr, "\n\n");

  }
#endif

  /* we temporarily use `ins_buf' to record the current glyph hints */
  ta_loader_register_hints_recorder(font->loader,
                                    TA_hints_recorder,
                                    (void*)&recorder);

  /*
   * It is important that we start the loop with the smallest PPEM value
   * used for hinting, since the number of segments that form an edge can
   * become smaller for larger PPEM values.  For efficiency, we skip
   * non-edge one-point segments, and `TA_get_segment_index' would return
   * wrong indices otherwise.
   */
  for (size = font->hinting_range_min;
       size <= font->hinting_range_max;
       size++)
  {
#ifdef DEBUGGING
    int have_dumps = 0;
#endif


    TA_rewind_recorder(&recorder, ins_buf, size);

    error = FT_Set_Pixel_Sizes(face, size, size);
    if (error)
      goto Err;

#ifdef DEBUGGING
    if (font->debug)
    {
      int num_chars, i;


      num_chars = fprintf(stderr, "size %d\n", size);
      for (i = 0; i < num_chars - 1; i++)
        putc('-', stderr);
      fprintf(stderr, "\n\n");
    }
#endif

    /* calling `ta_loader_load_glyph' uses the */
    /* `TA_hints_recorder' function as a callback, */
    /* modifying `hints_record' */
    error = ta_loader_load_glyph(font, face, (FT_UInt)idx, load_flags);
    if (error)
      goto Err;

    if (TA_hints_record_is_different(action_hints_records,
                                     num_action_hints_records,
                                     ins_buf, recorder.hints_record.buf))
    {
#ifdef DEBUGGING
      if (font->debug)
      {
        have_dumps = 1;

        ta_glyph_hints_dump_edges((TA_GlyphHints)_ta_debug_hints);
        ta_glyph_hints_dump_segments((TA_GlyphHints)_ta_debug_hints);
        ta_glyph_hints_dump_points((TA_GlyphHints)_ta_debug_hints);

        fprintf(stderr, "action hints record:\n");
        if (ins_buf == recorder.hints_record.buf)
          fprintf(stderr, "  (none)");
        else
        {
          fprintf(stderr, "  ");
          for (p = ins_buf; p < recorder.hints_record.buf; p += 2)
            fprintf(stderr, " %2d", *p * 256 + *(p + 1));
        }
        fprintf(stderr, "\n");
      }
#endif

      error = TA_add_hints_record(&action_hints_records,
                                  &num_action_hints_records,
                                  ins_buf, recorder.hints_record);
      if (error)
        goto Err;
    }

    /* now handle point records */

    TA_reset_recorder(&recorder, ins_buf);

    /* use the point hints data collected in `TA_hints_recorder' */
    TA_build_point_hints(&recorder, hints);

    if (TA_hints_record_is_different(point_hints_records,
                                     num_point_hints_records,
                                     ins_buf, recorder.hints_record.buf))
    {
#ifdef DEBUGGING
      if (font->debug)
      {
        if (!have_dumps)
        {
          int num_chars, i;


          num_chars = fprintf(stderr, "size %d\n", size);
          for (i = 0; i < num_chars - 1; i++)
            putc('-', stderr);
          fprintf(stderr, "\n\n");

          ta_glyph_hints_dump_edges((TA_GlyphHints)_ta_debug_hints);
          ta_glyph_hints_dump_segments((TA_GlyphHints)_ta_debug_hints);
          ta_glyph_hints_dump_points((TA_GlyphHints)_ta_debug_hints);
        }

        fprintf(stderr, "point hints record:\n");
        if (ins_buf == recorder.hints_record.buf)
          fprintf(stderr, "  (none)");
        else
        {
          fprintf(stderr, "  ");
          for (p = ins_buf; p < recorder.hints_record.buf; p += 2)
            fprintf(stderr, " %2d", *p * 256 + *(p + 1));
        }
        fprintf(stderr, "\n\n");
      }
#endif

      error = TA_add_hints_record(&point_hints_records,
                                  &num_point_hints_records,
                                  ins_buf, recorder.hints_record);
      if (error)
        goto Err;
    }
  }

  if (num_action_hints_records == 1 && !action_hints_records[0].num_actions)
  {
    /* since we only have a single empty record we just scale the glyph */
    bufp = TA_sfnt_build_glyph_scaler(sfnt, &recorder, ins_buf);
    if (!bufp)
    {
      error = FT_Err_Out_Of_Memory;
      goto Err;
    }

    use_gstyle_data = 0;

    goto Done;
  }

  /* if there is only a single record, */
  /* we do a global optimization later on */
  if (num_action_hints_records > 1)
    optimize = 1;

  /* store the hints records and handle stack depth */
  pos[0] = ins_buf;
  bufp = TA_emit_hints_records(&recorder,
                               point_hints_records,
                               num_point_hints_records,
                               ins_buf,
                               optimize);

  num_stack_elements = recorder.num_stack_elements;
  recorder.num_stack_elements = 0;

  pos[1] = bufp;
  bufp = TA_emit_hints_records(&recorder,
                               action_hints_records,
                               num_action_hints_records,
                               bufp,
                               optimize);

  recorder.num_stack_elements += num_stack_elements;

  pos[2] = bufp;
  bufp = TA_sfnt_build_glyph_segments(sfnt, &recorder, bufp, optimize);
  if (!bufp)
  {
    error = FT_Err_Out_Of_Memory;
    goto Err;
  }

  if (num_action_hints_records == 1)
    bufp = TA_optimize_push(ins_buf, pos);

Done:
  TA_free_hints_records(action_hints_records, num_action_hints_records);
  TA_free_hints_records(point_hints_records, num_point_hints_records);
  TA_free_recorder(&recorder);

Done1:
  /* handle delta exceptions */
  if (font->control_data_head)
  {
    bufp = TA_sfnt_build_delta_exceptions(sfnt, font, idx, bufp);
    if (!bufp)
    {
      error = FT_Err_Out_Of_Memory;
      goto Err;
    }
  }

  /* we need to insert a few extra bytecode instructions */
  /* for non-base glyphs */
  if (use_gstyle_data && (gstyles[idx] & TA_NONBASE))
  {
    FT_Byte* ins_extra_buf_new;
    FT_Byte ins_extra_len_new;


    ins_extra_len_new = glyph->ins_extra_len
                        + sizeof (ins_extra_ignore_std_width);
    ins_extra_buf_new = (FT_Byte*)realloc(glyph->ins_extra_buf,
                                          ins_extra_len_new);
    if (!ins_extra_buf_new)
    {
      bufp = NULL;
      goto Done;
    }

    /* set `cvtl_ignore_std_width' to 100 at the beginning of the bytecode */
    /* by activating `ins_extra_ignore_std_width' */
    memcpy(ins_extra_buf_new + glyph->ins_extra_len,
           ins_extra_ignore_std_width,
           sizeof (ins_extra_ignore_std_width));

    glyph->ins_extra_buf = ins_extra_buf_new;
    glyph->ins_extra_len = ins_extra_len_new;

    /* reset `cvtl_ignore_std_width' for next glyph */
    BCI(PUSHB_2);
    BCI(cvtl_ignore_std_width);
    BCI(0);
    BCI(WCVTP);
  }

  ins_len = (FT_UInt)(bufp - ins_buf);

  if (ins_len > sfnt->max_instructions)
    sfnt->max_instructions = (FT_UShort)ins_len;

  glyph->ins_buf = (FT_Byte*)realloc(ins_buf, ins_len);
  glyph->ins_len = ins_len;

  return FT_Err_Ok;

Err:
  TA_free_hints_records(action_hints_records, num_action_hints_records);
  TA_free_hints_records(point_hints_records, num_point_hints_records);
  TA_free_recorder(&recorder);
  free(ins_buf);

  return error;
}


/* end of tabytecode.c */
