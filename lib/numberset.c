/* numberset.c */

/*
 * Copyright (C) 2012-2017 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <sds.h>
#include <numberset.h>


number_range*
number_set_new(int start,
               int end,
               int min,
               int max)
{
  number_range* nr;


  if (start > end)
  {
    int tmp;


    tmp = start;
    start = end;
    end = tmp;
  }

  if (start < min || end > max)
    return NUMBERSET_INVALID_RANGE;

  nr = (number_range*)malloc(sizeof (number_range));
  if (!nr)
    return NUMBERSET_ALLOCATION_ERROR;

  nr->start = start;
  nr->end = end;
  nr->next = NULL;

  return nr;
}


number_range*
number_set_prepend(number_range* list,
                   number_range* element)
{
  if (!element)
    return list;

  if (!list)
    return element;

  if (element->start <= list->end)
  {
    if (element->end < list->start)
      return NUMBERSET_NOT_ASCENDING;
    else
      return NUMBERSET_OVERLAPPING_RANGES;
  }

  if (element->start == list->end + 1)
  {
    /* merge adjacent ranges */
    list->end = element->end;

    free(element);

    return list;
  }

  element->next = list;

  return element;
}


number_range*
number_set_insert(number_range* list,
                  number_range* element)
{
  number_range* nr = list;
  number_range* prev;


  if (!element)
    return list;

  if (!list)
    return element;

  prev = NULL;
  while (nr)
  {
    if (element->start <= nr->end
        && element->end >= nr->start)
      return NUMBERSET_OVERLAPPING_RANGES;

    /* merge adjacent ranges */
    if (element->start == nr->end + 1)
    {
      nr->end = element->end;

      free(element);

      return list;
    }
    if (element->end + 1 == nr->start)
    {
      nr->start = element->start;

      free(element);

      return list;
    }

    /* insert element */
    if (element->start > nr->end)
    {
      if (prev)
        prev->next = element;
      element->next = nr;

      return prev ? list : element;
    }

    prev = nr;
    nr = nr->next;
  }

  /* append element */
  prev->next = element;
  element->next = NULL;

  return list;
}


number_range*
number_set_normalize(number_range* list)
{
  number_range* cur;
  number_range* prev;


  if (!list)
    return NULL;

  prev = list;
  cur = list->next;

  if (!cur)
    return list; /* only a single element, nothing to do */

  while (cur)
  {
    if (prev->end + 1 == cur->start)
    {
      number_range* tmp;


      prev->end = cur->end;

      tmp = cur;
      cur = cur->next;
      prev->next = cur;

      free(tmp);
    }
    else
    {
      prev = cur;
      cur = cur->next;
    }
  }

  return list;
}


number_range*
number_set_reverse(number_range* list)
{
  number_range* cur;


  cur = list;
  list = NULL;

  while (cur)
  {
    number_range* tmp;


    tmp = cur;
    cur = cur->next;
    tmp->next = list;
    list = tmp;
  }

  return list;
}


const char*
number_set_parse(const char* s,
                 number_range** number_set,
                 int min,
                 int max)
{
  number_range* cur = NULL;
  number_range* new_range;

  const char* last_pos = s;
  int last_start = -1;
  int last_end = -1;
  int t;
  number_range* error_code = NULL;


  if (!s)
    return NULL;

  if (min < 0)
    min = 0;
  if (max < 0)
    max = INT_MAX;
  if (min > max)
  {
    t = min;
    min = max;
    max = t;
  }

  for (;;)
  {
    int digit;
    int n = -1;
    int m = -1;


    while (isspace(*s))
      s++;

    if (*s == ',')
    {
      s++;
      continue;
    }
    else if (*s == '-')
    {
      last_pos = s;
      n = min;
    }
    else if (isdigit(*s))
    {
      last_pos = s;
      n = 0;
      do
      {
        digit = *s - '0';
        if (n > INT_MAX / 10
            || (n == INT_MAX / 10 && digit > 5))
        {
          error_code = NUMBERSET_OVERFLOW;
          break;
        }

        n = n * 10 + digit;
        s++;
      } while (isdigit(*s));

      if (error_code)
        break;

      while (isspace(*s))
        s++;
    }
    else if (*s == '\0')
      break; /* end of data */
    else
    {
      error_code = NUMBERSET_INVALID_CHARACTER;
      break;
    }

    if (*s == '-')
    {
      s++;

      while (isspace(*s))
        s++;

      if (isdigit(*s))
      {
        m = 0;
        do
        {
          digit = *s - '0';
          if (m > INT_MAX / 10
              || (m == INT_MAX / 10 && digit > 5))
          {
            error_code = NUMBERSET_OVERFLOW;
            break;
          }

          m = m * 10 + digit;
          s++;
        } while (isdigit(*s));

        if (error_code)
          break;
      }
    }
    else
      m = n;

    if (m == -1)
      m = max;

    if (m < n)
    {
      t = n;
      n = m;
      m = t;
    }

    if (n < min || m > max)
    {
      error_code = NUMBERSET_INVALID_RANGE;
      break;
    }

    if (last_end >= n)
    {
      if (last_start >= m)
        error_code = NUMBERSET_NOT_ASCENDING;
      else
        error_code = NUMBERSET_OVERLAPPING_RANGES;
      break;
    }

    if (cur
        && last_end + 1 == n)
    {
      /* merge adjacent ranges */
      cur->end = m;
    }
    else
    {
      if (number_set)
      {
        new_range = (number_range*)malloc(sizeof (number_range));
        if (!new_range)
        {
          error_code = NUMBERSET_ALLOCATION_ERROR;
          break;
        }

        /* prepend new range to list */
        new_range->start = n;
        new_range->end = m;
        new_range->next = cur;
        cur = new_range;
      }
    }

    last_start = n;
    last_end = m;
  } /* end of loop */

  if (error_code)
  {
    number_set_free(cur);

    s = last_pos;
    if (number_set)
      *number_set = error_code;
  }
  else
  {
    /* success */
    if (number_set)
      *number_set = number_set_normalize(number_set_reverse(cur));
  }

  return s;
}


void
number_set_free(number_range* number_set)
{
  number_range* nr = number_set;


  while (nr)
  {
    number_range* tmp;


    tmp = nr;
    nr = nr->next;
    free(tmp);
  }
}


char*
number_set_show(number_range* number_set,
                int min,
                int max)
{
  sds s;
  size_t len;
  char* res;

  number_range* nr = number_set;

  const char* comma;


  if (min < 0)
    min = 0;
  if (max < 0)
    max = INT_MAX;
  if (min > max)
  {
    int t;


    t = min;
    min = max;
    max = t;
  }

  s = sdsempty();

  while (nr)
  {
    if (nr->start > max)
      goto Exit;
    if (nr->end < min)
      goto Again;

    comma = *s ? ", " : "";

    if (nr->start == nr->end)
      s = sdscatprintf(s, "%s%i", comma, nr->start);
    else if (nr->start <= min
             && nr->end >= max)
      s = sdscatprintf(s, "-");
    else if (nr->start <= min)
      s = sdscatprintf(s, "-%i", nr->end);
    else if (nr->end >= max)
      s = sdscatprintf(s, "%s%i-", comma, nr->start);
    else
      s = sdscatprintf(s, "%s%i-%i", comma, nr->start, nr->end);

  Again:
    nr = nr->next;
  }

Exit:
  if (!s)
    return NULL;

  /* we return an empty string for an empty number set */
  /* (this is, number_set == NULL or unsuitable `min' and `max' values) */
  len = sdslen(s) + 1;
  res = (char*)malloc(len);
  if (res)
    memcpy(res, s, len);

  sdsfree(s);

  return res;
}


int
number_set_is_element(number_range* number_set,
                      int number)
{
  number_range* nr = number_set;


  while (nr)
  {
    if (number < nr->start)
      return 0;
    if (nr->start <= number
        && number <= nr->end)
      return 1;
    nr = nr->next;
  }

  return 0;
}


int
number_set_get_first(number_set_iter* iter_p)
{
  if (!iter_p || !iter_p->range)
    return 0;

  iter_p->val = iter_p->range->start;

  return iter_p->val;
}


int
number_set_get_next(number_set_iter* iter_p)
{
  if (!iter_p || !iter_p->range)
    return 0;

  iter_p->val++;

  if (iter_p->val > iter_p->range->end)
  {
    iter_p->range = iter_p->range->next;

    if (iter_p->range)
      iter_p->val = iter_p->range->start;
  }

  return iter_p->val;
}

/* end of numberset.c */
