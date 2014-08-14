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

#include <ta.h>
#include <tadeltas.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include "llrb.h" /* a red-black tree implementation */


/* a test to check the validity of the first character */
/* in a PostScript glyph name -- for simplicity, we include `.' also */

const char* namestart_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz"
                              "._";

static int
isnamestart(int c)
{
  return strchr(namestart_chars, c) != NULL;
}


static char*
get_token(char** string_p)
{
  const char* s = *string_p;
  const char* p = *string_p;


  while (*p && !isspace(*p))
    p++;

  *string_p = (char*)p;
  return (char*)s;
}


/* in the following functions, `*string_p' is never '\0'; */
/* in case of error, `*string_p' should be set to a meaningful position */

static TA_Error
get_font_idx(FONT* font,
             char** string_p,
             long* font_idx_p)
{
  const char* s = *string_p;
  char* endptr;
  long font_idx;


  errno = 0;
  font_idx = strtol(s, &endptr, 0);

  if (errno == ERANGE)
  {
    /* *string_p = s; */
    return TA_Err_Deltas_Invalid_Font_Index;
  }

  /* there must be a whitespace character after the number */
  if (errno || !isspace(*endptr))
  {
    *string_p = endptr;
    return TA_Err_Deltas_Syntax_Error;
  }

  /* we have already tested that the first character in `s' is a digit, */
  /* so `font_idx' can't be negative */
  if (font_idx >= font->num_sfnts)
  {
    /* *string_p = s; */
    return TA_Err_Deltas_Invalid_Font_Index;
  }

  *string_p = endptr;
  *font_idx_p = font_idx;

  return TA_Err_Ok;
}


static TA_Error
get_glyph_idx(FONT* font,
              long font_idx,
              char** string_p,
              long* glyph_idx_p)
{
  const char* s = *string_p;
  char* endptr;
  long glyph_idx;

  FT_Face face = font->sfnts[font_idx].face;


  if (isdigit(*s))
  {
    /* glyph index */

    errno = 0;
    glyph_idx = strtol(s, &endptr, 0);

    if (errno == ERANGE)
    {
      /* *string_p = s; */
      return TA_Err_Deltas_Invalid_Glyph_Index;
    }

    /* there must be a whitespace character after the number */
    if (errno || !isspace(*endptr))
    {
      *string_p = endptr;
      return TA_Err_Deltas_Syntax_Error;
    }

    /* we have already tested that the first character in `s' is a digit, */
    /* so `glyph_idx' can't be negative */
    if (glyph_idx >= face->num_glyphs)
    {
      /* *string_p = s; */
      return TA_Err_Deltas_Invalid_Glyph_Index;
    }
  }
  else if (isnamestart(*s))
  {
    /* glyph name */

    char* token;


    endptr = (char*)s;
    s = get_token(&endptr);

    token = strndup(s, endptr - s);
    if (!token)
    {
      /* *string_p = s; */
      return TA_Err_Deltas_Allocation_Error;
    }

    /* explicitly compare with `.notdef' */
    /* since `FT_Get_Name_Index' returns glyph index 0 */
    /* for both this glyph name and invalid ones */
    if (!strcmp(token, ".notdef"))
      glyph_idx = 0;
    else
    {
      glyph_idx = FT_Get_Name_Index(face, token);
      if (glyph_idx == 0)
        glyph_idx = -1;
    }

    free(token);

    if (glyph_idx < 0)
    {
      /* *string_p = s; */
      return TA_Err_Deltas_Invalid_Glyph_Name;
    }
  }
  else
  {
    /* invalid */

    /* *string_p = s; */
    return TA_Err_Deltas_Syntax_Error;
  }

  *string_p = endptr;
  *glyph_idx_p = glyph_idx;

  return TA_Err_Ok;
}


static TA_Error
get_range(char** string_p,
          number_range** number_set_p,
          int min,
          int max,
          const char* delims)
{
  const char* s = *string_p;
  number_range* number_set = *number_set_p;

  size_t s_len = strcspn(s, delims);
  char* token;
  char* endptr;


  /* there must be a whitespace character before the delimiter */
  /* if it is not \0 */
  if (*delims
      && (!s_len || !isspace(s[s_len - 1])))
  {
    *string_p = (char*)(s + s_len);
    return TA_Err_Deltas_Syntax_Error;
  }

  token = strndup(s, s_len);
  if (!token)
  {
    /* *string_p = s; */
    return TA_Err_Deltas_Allocation_Error;
  }

  endptr = (char*)number_set_parse(token, &number_set, min, max);

  /* use offset relative to `s' */
  endptr = (char*)s + (endptr - token);
  free(token);

  if (number_set == NUMBERSET_ALLOCATION_ERROR)
  {
    /* *string_p = s; */
    return TA_Err_Deltas_Allocation_Error;
  }

  *string_p = endptr;

  if (number_set == NUMBERSET_INVALID_CHARACTER)
    return TA_Err_Deltas_Invalid_Character;
  else if (number_set == NUMBERSET_OVERFLOW)
    return TA_Err_Deltas_Overflow;
  else if (number_set == NUMBERSET_INVALID_RANGE)
    /* this error code should be adjusted by the caller if necessary */
    return TA_Err_Deltas_Invalid_Point_Range;
  else if (number_set == NUMBERSET_OVERLAPPING_RANGES)
    return TA_Err_Deltas_Overlapping_Ranges;
  else if (number_set == NUMBERSET_NOT_ASCENDING)
    return TA_Err_Deltas_Ranges_Not_Ascending;

  *number_set_p = number_set;

  return TA_Err_Ok;
}


static TA_Error
get_shift(char** string_p,
          char* shift_p)
{
  const char* s = *string_p;
  char* endptr;
  double shift;


  errno = 0;
  shift = strtod(s, &endptr);

  if (errno == ERANGE)
  {
    /* *string_p = s; */
    /* this error code should be adjusted by the caller if necessary */
    return TA_Err_Deltas_Invalid_X_Range;
  }

  /* there must be a whitespace character after the number */
  if (errno || !isspace(*endptr))
  {
    *string_p = endptr;
    return TA_Err_Deltas_Syntax_Error;
  }

  if (shift < DELTA_SHIFT_MIN || shift > DELTA_SHIFT_MAX)
  {
    /* *string_p = s; */
    return TA_Err_Deltas_Invalid_X_Range;
  }

  *string_p = endptr;

  /* we round the value to a multiple of 1/(2^DELTA_SHIFT) */
  *shift_p = (char)(shift * DELTA_FACTOR + (shift > 0 ? 0.5 : -0.5));

  return TA_Err_Ok;
}


/*
 * Parse a delta exceptions line.
 *
 * In case of success (this is, the delta exceptions description in `s' is
 * valid), `pos' is a pointer to the final zero byte in string `s'.  In case
 * of an error, it points to the offending character in string `s'.
 *
 * If s is NULL, the function exits immediately, with NULL as the value of
 * `pos'.
 *
 * If the user provides a non-NULL `deltas' value, `deltas_parse' stores the
 * parsed result in `*deltas', allocating the necessary memory.  If there is
 * no data (for example, an empty string or whitespace only) nothing gets
 * allocated, and `*deltas' is set to NULL.
 *
 */

static TA_Error
deltas_parse_line(FONT* font,
                  const char* s,
                  char** err_pos,
                  Deltas** deltas_p,
                  int ppem_min, int ppem_max)
{
  TA_Error error;

  typedef enum State_
  { START,
    HAD_TOKEN1,
    HAD_TOKEN2,
    HAD_FONT_IDX,
    HAD_GLYPH_IDX,
    HAD_P,
    HAD_POINTS,
    HAD_X,
    HAD_X_SHIFT,
    HAD_Y,
    HAD_Y_SHIFT,
    HAD_AT,
    HAD_PPEMS
  } State;

  State state = START;

  char* pos = (char*)s;

  const char* token1;
  const char* token2;

  long font_idx = 0;
  long glyph_idx = -1;
  number_range* points = NULL;
  char x_shift = 0;
  char y_shift = 0;
  number_range* ppems = NULL;


  if (!s)
  {
    *err_pos = NULL;
    return TA_Err_Ok;
  }

  for (;;)
  {
    char c;
    int next_is_space;
    State old_state = state;


    error = TA_Err_Ok;

    while (isspace(*pos))
      pos++;

    /* skip comment */
    if (*pos == '#')
      while (*pos)
        pos++;

    /* EOL */
    if (!*pos)
      break;

    c = *pos;
    next_is_space = isspace(*(pos + 1));

    switch (state)
    {
    case START:
      token1 = get_token(&pos);
      state = HAD_TOKEN1;
      break;

    case HAD_TOKEN1:
      token2 = get_token(&pos);
      state = HAD_TOKEN2;
      break;

    case HAD_TOKEN2:
      pos = (char*)token1;

      /* possibility 1: <font idx> <glyph name> `p' */
      if (isdigit(*token1)
          && (isdigit(*token2) || isnamestart(*token2))
          && (c == 'p' && next_is_space))
      {
        if (!(error = get_font_idx(font, &pos, &font_idx)))
          state = HAD_FONT_IDX;
      }
      /* possibility 2: <glyph name> `p' */
      else if ((isdigit(*token1) || isnamestart(*token1))
               && (c == 'p' && next_is_space))
      {
        if (!(error = get_glyph_idx(font, font_idx,
                                    &pos, &glyph_idx)))
          state = HAD_GLYPH_IDX;
      }
      break;

    case HAD_FONT_IDX:
      /* <glyph idx> */
      if (!(error = get_glyph_idx(font, font_idx,
                                  &pos, &glyph_idx)))
        state = HAD_GLYPH_IDX;
      break;

    case HAD_GLYPH_IDX:
      /* `p' */
      if (c == 'p' && next_is_space)
      {
        state = HAD_P;
        pos += 2;
      }
      break;

    case HAD_P:
      /* <points> */
      {
        FT_Error ft_error;
        FT_Face face = font->sfnts[font_idx].face;
        int num_points;


        ft_error = FT_Load_Glyph(face, glyph_idx, FT_LOAD_NO_SCALE);
        if (ft_error)
        {
          error = TA_Err_Deltas_Invalid_Glyph;
          break;
        }

        num_points = face->glyph->outline.n_points;
        if (!(error = get_range(&pos, deltas_p ? &points : NULL,
                                0, num_points - 1, "xy@")))
          state = HAD_POINTS;
      }
      break;

    case HAD_POINTS:
      /* `x' or `y' or `@' */
      if (next_is_space)
      {
        pos += 2;
        if (c == 'x')
          state = HAD_X;
        else if (c == 'y')
          state = HAD_Y;
        else if (c == '@')
          state = HAD_AT;
        else
          pos -= 2;
      }
      break;

    case HAD_X:
      /* <x_shift> */
      if (!(error = get_shift(&pos, &x_shift)))
        state = HAD_X_SHIFT;
      break;

    case HAD_X_SHIFT:
      /* 'y' or '@' */
      if (next_is_space)
      {
        pos += 2;
        if (c == 'y')
          state = HAD_Y;
        else if (c == '@')
          state = HAD_AT;
        else
          pos -= 2;
      }
      break;

    case HAD_Y:
      /* <y shift> */
      if (!(error = get_shift(&pos, &y_shift)))
        state = HAD_Y_SHIFT;
      if (error == TA_Err_Deltas_Invalid_X_Range)
        error = TA_Err_Deltas_Invalid_Y_Range;
      break;

    case HAD_Y_SHIFT:
      /* '@' */
      if (c == '@' && next_is_space)
      {
        state = HAD_AT;
        pos += 2;
      }
      break;

    case HAD_AT:
      /* <ppems> */
      if (!(error = get_range(&pos, deltas_p ? &ppems : NULL,
                              ppem_min, ppem_max, "")))
        state = HAD_PPEMS;
      if (error == TA_Err_Deltas_Invalid_Point_Range)
        error = TA_Err_Deltas_Invalid_Ppem_Range;
      break;

    case HAD_PPEMS:
      /* to make compiler happy */
      break;
    }

    if (old_state == state)
      break;
  }

  if (state == HAD_PPEMS)
  {
    /* success */
    if (deltas_p)
    {
      Deltas* deltas = (Deltas*)malloc(sizeof (Deltas));


      if (!deltas)
      {
        number_set_free(points);
        number_set_free(ppems);

        *err_pos = (char*)s;

        return TA_Err_Deltas_Allocation_Error;
      }

      deltas->font_idx = font_idx;
      deltas->glyph_idx = glyph_idx;
      deltas->points = points;
      deltas->x_shift = x_shift;
      deltas->y_shift = y_shift;
      deltas->ppems = ppems;
      deltas->next = NULL;

      *deltas_p = deltas;
    }
  }
  else
  {
    if (deltas_p)
    {
      number_set_free(points);
      number_set_free(ppems);

      *deltas_p = NULL;
    }

    if (!error && state != START)
      error = TA_Err_Deltas_Syntax_Error;
  }

  *err_pos = pos;

  return error;
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


/* `len' is the length of the returned string */

char*
deltas_show_line(FONT* font,
                 int* len,
                 Deltas* deltas)
{
  int ret;
  char glyph_name_buf[64];
  char* points_buf = NULL;
  char* ppems_buf = NULL;
  char* deltas_buf = NULL;

  FT_Face face;


  if (!deltas)
    return NULL;

  if (deltas->font_idx >= font->num_sfnts)
    return NULL;

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
    ret = asprintf(&deltas_buf, "%ld %s p %s x %.20g y %.20g @ %s",
                   deltas->font_idx,
                   glyph_name_buf,
                   points_buf,
                   (double)deltas->x_shift / DELTA_FACTOR,
                   (double)deltas->y_shift / DELTA_FACTOR,
                   ppems_buf);
  else
    ret = asprintf(&deltas_buf, "%ld %ld p %s x %.20g y %.20g @ %s",
                   deltas->font_idx,
                   deltas->glyph_idx,
                   points_buf,
                   (double)deltas->x_shift / DELTA_FACTOR,
                   (double)deltas->y_shift / DELTA_FACTOR,
                   ppems_buf);

Exit:
  free(points_buf);
  free(ppems_buf);

  if (ret == -1)
    return NULL;

  *len = ret;

  return deltas_buf;
}


char*
TA_deltas_show(FONT* font,
               Deltas* deltas)
{
  char* s;
  int s_len;


  /* we return an empty string if there is no data */
  s = (char*)malloc(1);
  if (!s)
    return NULL;

  *s = '\0';
  s_len = 1;

  while (deltas)
  {
    char* tmp;
    int tmp_len;
    char* s_new;
    int s_len_new;


    tmp = deltas_show_line(font, &tmp_len, deltas);
    if (!tmp)
    {
      free(s);
      return NULL;
    }

    /* append current line to buffer, followed by a newline character */
    s_len_new = s_len + tmp_len + 1;
    s_new = (char*)realloc(s, s_len_new);
    if (!s_new)
    {
      free(s);
      free(tmp);
      return NULL;
    }

    strcpy(s_new + s_len - 1, tmp);
    s_new[s_len_new - 2] = '\n';
    s_new[s_len_new - 1] = '\0';

    s = s_new;
    s_len = s_len_new;

    free(tmp);

    deltas = deltas->next;
  }

  return s;
}


/* Get a line from a buffer, starting at `pos'.  The final EOL */
/* character (or character pair) of the line (if existing) gets removed. */
/* After the call, `pos' points to the position right after the line in */
/* the buffer.  The line is allocated with `malloc'; it returns NULL for */
/* end of data and allocation errors; the latter can be recognized by a */
/* changed value of `pos'.  */

static char*
get_line(char** pos)
{
  const char* start = *pos;
  const char* p = start;
  size_t len;
  char* s;


  if (!*p)
    return NULL;

  while (*p)
  {
    if (*p == '\n' || *p == '\r')
      break;

    p++;
  }

  len = p - start + 1;

  if (*p == '\r')
  {
    len--;
    p++;

    if (*p == '\n')
      p++;
  }
  else if (*p == '\n')
  {
    len--;
    p++;
  }

  *pos = (char*)p;

  s = (char*)malloc(len + 1);
  if (!s)
    return NULL;

  if (len)
    strncpy(s, start, len);
  s[len] = '\0';

  return s;
}


TA_Error
TA_deltas_parse(FONT* font,
                Deltas** deltas,
                unsigned int* errlinenum_p,
                char** errline_p,
                char** errpos_p)
{
  FT_Error error;

  Deltas* cur;
  Deltas* new_deltas;
  Deltas* tmp;
  Deltas* list;

  unsigned int linenum;

  char* bufpos;
  char* bufpos_old;


  /* nothing to do if no data */
  if (!font->deltas_buf)
  {
    *deltas = NULL;
    return TA_Err_Ok;
  }

  bufpos = font->deltas_buf;
  bufpos_old = bufpos;

  linenum = 0;
  cur = NULL;

  /* parse line by line */
  for (;;)
  {
    char* line;
    char* errpos;


    line = get_line(&bufpos);
    if (!line)
    {
      if (bufpos == bufpos_old) /* end of data */
        break;

      *errlinenum_p = linenum;
      *errline_p = NULL;
      *errpos_p = NULL;

      return FT_Err_Out_Of_Memory;
    }

    error = deltas_parse_line(font,
                              line,
                              &errpos,
                              deltas ? &new_deltas : NULL,
                              DELTA_PPEM_MIN, DELTA_PPEM_MAX);
    if (error)
    {
      *errlinenum_p = linenum;
      *errline_p = line;
      *errpos_p = errpos;

      TA_deltas_free(cur);

      return error;
    }

    if (deltas && new_deltas)
    {
      new_deltas->next = cur;
      cur = new_deltas;
    }

    free(line);
    linenum++;
    bufpos_old = bufpos;
  }

  /* success; now reverse list to have elements in original order */
  list = NULL;

  while (cur)
  {
    tmp = cur;
    cur = cur->next;
    tmp->next = list;
    list = tmp;
  }

  if (deltas)
    *deltas = list;

  return TA_Err_Ok;
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
TA_deltas_build_tree(FONT* font,
                     Deltas* deltas)
{
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
          Deltas deltas;
          number_range ppems;
          number_range points;

          char* s;
          int s_len;


          /* construct Deltas entry for debugging output */
          ppems.start = ppem;
          ppems.end = ppem;
          ppems.next = NULL;
          points.start = point_idx;
          points.end = point_idx;
          points.next = NULL;

          deltas.font_idx = font_idx;
          deltas.glyph_idx = glyph_idx;
          deltas.points = &points;
          deltas.x_shift = x_shift;
          deltas.y_shift = y_shift;
          deltas.ppems = &ppems;
          deltas.next = NULL;

          s = deltas_show_line(font, &s_len, &deltas);
          if (s)
          {
            fprintf(stderr, "Delta exception %s ignored.\n", s);
            free(s);
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
