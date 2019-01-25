/* numberset.c */

/*
 * Copyright (C) 2012-2019 by Werner Lemberg.
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
  int tmp;


  if (min < 0)
    min = 0;
  if (max < 0)
    max = INT_MAX;
  if (min > max)
  {
    tmp = min;
    min = max;
    max = tmp;
  }

  if (start > end)
  {
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
  nr->base = 0;
  nr->wrap = 0;
  nr->next = NULL;

  return nr;
}


int
wrap_range_check_wraps(size_t num_wraps,
                       int* wraps)
{
  size_t i;


  /* `wraps' must have at least two elements */
  if (!wraps || num_wraps < 2)
    return 1;

  /* first `wraps' element must be larger than or equal to -1 */
  if (wraps[0] < -1)
    return 1;

  /* check that all elements of `wraps' are strictly ascending */
  for (i = 1; i < num_wraps; i++)
    if (wraps[i] <= wraps[i - 1])
      return 1;

  return 0;
}


number_range*
wrap_range_new(int start,
               int end,
               size_t num_wraps,
               int* wraps)
{
  number_range* nr;
  size_t i;
  int s, e;


  if (num_wraps < 2)
    return NUMBERSET_INVALID_WRAP_RANGE;

  if (start > end)
  {
    s = end;
    e = start;
  }
  else
  {
    s = start;
    e = end;
  }

  /* search fitting interval in `wraps' */
  for (i = 1; i < num_wraps; i++)
  {
    if (s > wraps[i - 1] && e <= wraps[i])
      break;
  }
  if (i == num_wraps)
    return NUMBERSET_INVALID_WRAP_RANGE;

  nr = (number_range*)malloc(sizeof (number_range));
  if (!nr)
    return NUMBERSET_ALLOCATION_ERROR;

  nr->start = start;
  nr->end = end;
  nr->base = wraps[i - 1] + 1;
  nr->wrap = wraps[i];
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

  /* `list' and `element' must both be normal integer ranges */
  if (list->base != list->wrap || element->base != element->wrap)
    return NUMBERSET_INVALID_RANGE;

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
number_set_prepend_unsorted(number_range* list,
                            number_range* element)
{
  if (!element)
    return list;

  if (!list)
    return element;

  /* `list' and `element' must both be normal integer ranges */
  if (list->base != list->wrap || element->base != element->wrap)
    return NUMBERSET_INVALID_RANGE;

  element->next = list;

  return element;
}


number_range*
wrap_range_prepend(number_range* list,
                   number_range* element)
{
  if (!element)
    return list;

  if (!list)
    return element;

  /* `list' and `element' must both be wrap-around ranges */
  if (list->base == list->wrap || element->base == element->wrap)
    return NUMBERSET_INVALID_RANGE;

  /* we explicitly assume that the [base;wrap] intervals */
  /* of `list' and `element' either don't overlap ... */
  if (element->base < list->base)
    return NUMBERSET_NOT_ASCENDING;
  if (element->base > list->base)
    goto prepend;

  /* ... or are exactly the same; */
  /* in this case, we can't append if the list's range really wraps around */
  if (list->start > list->end)
    return NUMBERSET_OVERLAPPING_RANGES;

  if (element->start <= list->end)
  {
    if (element->end < list->start)
      return NUMBERSET_NOT_ASCENDING;
    else
      return NUMBERSET_OVERLAPPING_RANGES;
  }

  if (element->start > element->end)
  {
    number_range* nr = list->next;


    /* we must search backwards to check */
    /* whether the wrapped part of the range overlaps */
    /* with an already existing range */
    /* (in the same [base;wrap] interval) */
    while (nr)
    {
      if (element->base != nr->base)
        break;
      if (element->end > nr->start)
        return NUMBERSET_OVERLAPPING_RANGES;

      nr = nr->next;
    }
  }

prepend:
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

  /* `list' and `element' must both be normal integer ranges */
  if (list->base != list->wrap || element->base != element->wrap)
    return NUMBERSET_INVALID_RANGE;

  prev = NULL;
  while (nr)
  {
    if (element->start <= nr->end
        && element->end >= nr->start)
      return NUMBERSET_OVERLAPPING_RANGES;

    /* merge adjacent ranges */
    if (element->end + 1 == nr->start)
    {
      nr->start = element->start;

      free(element);

      if (nr->next
          && nr->next->end + 1 == nr->start)
      {
        element = nr->next;
        element->end = nr->end;
        free(nr);
        return element;
      }

      return list;
    }

    /* insert element */
    if (element->start > nr->end)
    {
      /* merge adjacent ranges */
      if (nr->end + 1 == element->start)
      {
        nr->end = element->end;
        free(element);
        return list;
      }

      if (prev)
        prev->next = element;
      element->next = nr;

      return prev ? list : element;
    }

    prev = nr;
    nr = nr->next;
  }

  /* prepend element */
  prev->next = element;
  element->next = NULL;

  return list;
}


number_range*
wrap_range_insert(number_range* list,
                  number_range* element)
{
  number_range* nr = list;
  number_range* prev;


  if (!element)
    return list;

  if (!list)
    return element;

  /* `list' and `element' must both be wrap-around ranges */
  if (list->base == list->wrap || element->base == element->wrap)
    return NUMBERSET_INVALID_RANGE;

  prev = NULL;
  while (nr)
  {
    /* we explicitly assume that the [base;wrap] intervals */
    /* of `list' and `element' either don't overlap ... */
    if (element->base < nr->base)
      goto next;
    if (element->base > nr->base)
      goto insert;

    /* ... or are exactly the same */

    if (nr->start > nr->end)
    {
      /* range really wraps around */
      if (element->start > element->end)
        return NUMBERSET_OVERLAPPING_RANGES;

      if (element->start <= nr->end
          || element->end >= nr->start)
        return NUMBERSET_OVERLAPPING_RANGES;
    }
    else
    {
      /* normal range */
      if (element->start <= nr->end
          && element->end >= nr->start)
        return NUMBERSET_OVERLAPPING_RANGES;

      /* insert element */
      if (element->start > nr->end)
      {
      insert:
        if (prev)
          prev->next = element;
        element->next = nr;

        return prev ? list : element;
      }
    }

  next:
    prev = nr;
    nr = nr->next;
  }

  /* prepend element */
  prev->next = element;
  element->next = NULL;

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
        new_range->base = 0;
        new_range->wrap = 0;
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
      *number_set = number_set_reverse(cur);
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


  if (nr && nr->base == nr->wrap)
  {
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
  }
  else
  {
    /* `min' and `max' are meaningless for wrap-around ranges */
    min = INT_MIN;
    max = INT_MAX;
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
    if (nr->start > nr->end)
    {
      if (number < nr->base)
        return 0;
      if (number <= nr->end)
        return 1;
      if ( number < nr->start)
        return 0;
      if (number <= nr->wrap)
        return 1;
    }
    else
    {
      if (number < nr->start)
        return 0;
      if (number <= nr->end)
        return 1;
    }

    nr = nr->next;
  }

  return 0;
}


int
number_set_get_first(number_set_iter* iter_p)
{
  if (!iter_p || !iter_p->range)
    return -1;

  iter_p->val = iter_p->range->start;

  return iter_p->val;
}


int
number_set_get_next(number_set_iter* iter_p)
{
  if (!iter_p || !iter_p->range)
    return -1;

  iter_p->val++;

  if (iter_p->range->start > iter_p->range->end)
  {
    /* range really wraps around */
    if (iter_p->val > iter_p->range->wrap)
      iter_p->val = iter_p->range->base;
    else if (iter_p->val < iter_p->range->start
             && iter_p->val > iter_p->range->end)
    {
      iter_p->range = iter_p->range->next;

      if (iter_p->range)
        iter_p->val = iter_p->range->start;
      else
        return -1;
    }
  }
  else
  {
    if (iter_p->val > iter_p->range->end)
    {
      iter_p->range = iter_p->range->next;

      if (iter_p->range)
        iter_p->val = iter_p->range->start;
      else
        return -1;
    }
  }

  return iter_p->val;
}

/* end of numberset.c */
