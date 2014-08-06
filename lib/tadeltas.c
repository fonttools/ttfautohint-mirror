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


static const char*
get_token(const char** string_p)
{
  const char* s = *string_p;
  const char* p = *string_p;


  while (*p && !isspace(*p))
    p++;

  *string_p = p;
  return s;
}


/* in the following functions, `*string_p' is never '\0'; */
/* in case of error, `*string_p' should be set to a meaningful position */

TA_Error
get_font_idx(FONT* font,
             const char** string_p,
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


TA_Error
get_glyph_idx(FONT* font,
              long font_idx,
              const char** string_p,
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
    s = get_token((const char**)&endptr);

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


TA_Error
get_range(const char** string_p,
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
    *string_p = s + s_len;
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


TA_Error
get_shift(const char** string_p,
          char* shift_p,
          int min,
          int max)
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

  if (shift < min || shift > max)
  {
    /* *string_p = s; */
    return TA_Err_Deltas_Invalid_X_Range;
  }

  *string_p = endptr;

  /* we round the value to a multiple of 1/(2^DELTA_SHIFT) */
  *shift_p = (char)(shift * DELTA_FACTOR + (shift > 0 ? 0.5 : -0.5));

  return TA_Err_Ok;
}


TA_Error
TA_deltas_parse(FONT* font,
                const char* s,
                const char** err_pos,
                Deltas** deltas_p,
                double x_min, double x_max,
                double y_min, double y_max,
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

  Deltas* deltas = NULL;
  const char* pos = s;

  const char* token1;
  const char* token2;


  if (!s)
  {
    *err_pos = NULL;
    return TA_Err_Ok;
  }

  deltas = (Deltas*)malloc(sizeof (Deltas));
  if (!deltas)
  {
    *err_pos = s;

    return TA_Err_Deltas_Allocation_Error;
  }

  deltas->font_idx = 0;
  deltas->glyph_idx = -1;
  deltas->points = NULL;
  deltas->x_shift = 0;
  deltas->y_shift = 0;
  deltas->ppems = NULL;

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
      pos = token1;

      /* possibility 1: <font idx> <glyph name> `p' */
      if (isdigit(*token1)
          && (isdigit(*token2) || isnamestart(*token2))
          && (c == 'p' && next_is_space))
      {
        if (!(error = get_font_idx(font, &pos, &deltas->font_idx)))
          state = HAD_FONT_IDX;
      }
      /* possibility 2: <glyph name> `p' */
      else if ((isdigit(*token1) || isnamestart(*token1))
               && (c == 'p' && next_is_space))
      {
        if (!(error = get_glyph_idx(font, deltas->font_idx,
                                    &pos, &deltas->glyph_idx)))
          state = HAD_GLYPH_IDX;
      }
      break;

    case HAD_FONT_IDX:
      /* <glyph idx> */
      if (!(error = get_glyph_idx(font, deltas->font_idx,
                                  &pos, &deltas->glyph_idx)))
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
        FT_Face face = font->sfnts[deltas->font_idx].face;
        int num_points;


        ft_error = FT_Load_Glyph(face, deltas->glyph_idx, FT_LOAD_NO_SCALE);
        if (ft_error)
        {
          error = TA_Err_Deltas_Invalid_Glyph;
          break;
        }

        num_points = face->glyph->outline.n_points;
        if (!(error = get_range(&pos, &deltas->points,
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
      if (!(error = get_shift(&pos, &deltas->x_shift, x_min, x_max)))
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
      if (!(error = get_shift(&pos, &deltas->y_shift, y_min, y_max)))
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
      if (!(error = get_range(&pos, &deltas->ppems,
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
      *deltas_p = deltas;
    else
      TA_deltas_free(deltas);
  }
  else
  {
    TA_deltas_free(deltas);

    if (!error)
      error = TA_Err_Deltas_Syntax_Error;
  }

  *err_pos = pos;

  return error;
}


void
TA_deltas_free(Deltas* deltas)
{
  if (!deltas)
    return;

  number_set_free(deltas->points);
  number_set_free(deltas->ppems);

  free(deltas);
}


char*
TA_deltas_show(FONT* font,
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
    ret = asprintf(&deltas_buf, "%ld %s p %s x %f y %f @ %s",
                   deltas->font_idx,
                   glyph_name_buf,
                   points_buf,
                   (double)deltas->x_shift / DELTA_FACTOR,
                   (double)deltas->y_shift / DELTA_FACTOR,
                   ppems_buf);
  else
    ret = asprintf(&deltas_buf, "%ld %ld p %s x %f y %f @ %s",
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

  return deltas_buf;
}

/* end of tadeltas.c */
