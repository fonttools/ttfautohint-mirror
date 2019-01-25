/* numberset-test.c */

/*
 * Copyright (C) 2017-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */

/*
 * Compile with
 *
 *   $(CC) $(CFLAGS) \
 *         -I.. -I. \
 *         -o numberset-test numberset-test.c numberset.c sds.c
 *
 * after configuration.  The resulting binary aborts with an assertion
 * message in case of an error, otherwise it produces no output.
 *
 * If you want to check the code coverage with `gcov', add compiler options
 *
 *   -fprofile-arcs -ftest-coverage
 *
 * Except memory allocation errors, all code in `numberset.c' is covered.
 */


#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <numberset.h>


int
main(void)
{
  number_range* list;
  number_range* nr;
  number_range* nr1;
  number_range* nr2;
  number_range* nr3;
  number_set_iter iter;
  char* s;
  char* p;
  const char* r;
  const char* in;
  const char* out;
  int i;

  int wraps[] = {-1, 4, 9, 20};
  size_t num_wraps = sizeof(wraps) / sizeof(int);


  /* number_set_new */
  /* -------------- */

  /* start < min */
  nr = number_set_new(1, 1, 2, 2);
  assert(nr == NUMBERSET_INVALID_RANGE);

  /* end < max */
  nr = number_set_new(2, 2, 1, 1);
  assert(nr == NUMBERSET_INVALID_RANGE);

  /* min < 0, max < 0 */
  nr = number_set_new(1, 2, -1, -1);
  assert(nr < NUMBERSET_INVALID_WRAP_RANGE);
  free(nr);

  /* min > max, start > end */
  nr = number_set_new(2, 1, 2, 1);
  assert(nr < NUMBERSET_INVALID_WRAP_RANGE);
  free(nr);

  /* wrap_range_check_wraps */
  /* ---------------------- */

  /* !wraps */
  i = wrap_range_check_wraps(0, NULL);
  assert(i == 1);

  /* num_wraps < 2 */
  i = wrap_range_check_wraps(1, wraps);
  assert(i == 1);

  /* wraps[0] < -1 */
  {
    int wraps_bad1[] = { -2, 2 };
    size_t num_wraps_bad1 = sizeof(wraps_bad1) / sizeof(int);


    i = wrap_range_check_wraps(num_wraps_bad1, wraps_bad1);
    assert(i == 1);
  }

  /* wraps[n] >= wraps[n+1] */
  {
    int wraps_bad2[] = { 1, 1 };
    size_t num_wraps_bad2 = sizeof(wraps_bad2) / sizeof(int);


    i = wrap_range_check_wraps(num_wraps_bad2, wraps_bad2);
    assert(i == 1);
  }

  /* wraps ok */
  i = wrap_range_check_wraps(num_wraps, wraps);
  assert(i == 0);

  /* wrap_range_new */
  /* -------------- */

  /* num_wraps < 2 */
  nr = wrap_range_new(0, 0, 0, NULL);
  assert(nr == NUMBERSET_INVALID_WRAP_RANGE);

  /* start < wraps[n] < end */
  nr = wrap_range_new(2, 6, num_wraps, wraps);
  assert(nr == NUMBERSET_INVALID_WRAP_RANGE);

  /* start <= end */
  nr = wrap_range_new(1, 1, num_wraps, wraps);
  assert(nr < NUMBERSET_INVALID_WRAP_RANGE);
  free(nr);

  /* start > end */
  nr = wrap_range_new(2, 1, num_wraps, wraps);
  assert(nr < NUMBERSET_INVALID_WRAP_RANGE);
  free(nr);

  /* number_set_show */
  /* --------------- */

  /* single integer normal range, min < 0, max < 0 */
  nr = number_set_new(1, 1, 1, 1);
  s = number_set_show(nr, -1, -1);
  assert(!strcmp(s, "1"));
  free(nr);
  free(s);

  /* one normal full range, max < min */
  nr = number_set_new(1, 2, 1, 2);
  s = number_set_show(nr, 2, 1);
  assert(!strcmp(s, "-"));
  free(nr);
  free(s);

  /* one wrap-around range */
  nr = wrap_range_new(2, 1, num_wraps, wraps);
  s = number_set_show(nr, 0, 0);
  assert(!strcmp(s, "2-1"));
  free(nr);
  free(s);

  /* two normal ranges, start > max */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(4, 5, 4, 5);
  nr->next = nr1;
  s = number_set_show(nr, 0, 3);
  assert(!strcmp(s, "1-2"));
  free(nr);
  free(nr1);
  free(s);

  /* two normal ranges, start > max */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(4, 5, 4, 5);
  nr->next = nr1;
  s = number_set_show(nr, 3, 6);
  assert(!strcmp(s, "4-5"));
  free(nr);
  free(nr1);
  free(s);

  /* three ranges */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(4, 5, 4, 5);
  nr2 = number_set_new(7, 8, 7, 8);
  nr->next = nr1;
  nr1->next = nr2;
  s = number_set_show(nr, 1, 8);
  assert(!strcmp(s, "-2, 4-5, 7-"));
  free(nr);
  free(nr1);
  free(nr2);
  free(s);

  /* number_set_prepend */
  /* ------------------ */

  /* !element */
  list = number_set_prepend(NUMBERSET_INVALID_CHARACTER, NULL);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* !list */
  list = number_set_prepend(NULL, NUMBERSET_INVALID_CHARACTER);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* different range types */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = wrap_range_new(3, 4, num_wraps, wraps);
  list = number_set_prepend(nr, nr1);
  assert(list == NUMBERSET_INVALID_RANGE);
  free(nr);
  free(nr1);

  /* ranges not ascending */
  nr = number_set_new(3, 4, 3, 4);
  nr1 = number_set_new(1, 2, 1, 2);
  list = number_set_prepend(nr, nr1);
  assert(list == NUMBERSET_NOT_ASCENDING);
  free(nr);
  free(nr1);

  /* ranges overlapping */
  nr = number_set_new(1, 5, 1, 5);
  nr1 = number_set_new(1, 2, 1, 2);
  list = number_set_prepend(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* merge adjacent ranges */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(3, 4, 3, 4);
  list = number_set_prepend(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "1-4"));
  number_set_free(list);
  free(s);

  /* normal prepend */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(4, 5, 4, 5);
  list = number_set_prepend(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "4-5, 1-2"));
  number_set_free(list);
  free(s);

  /* wrap_range_prepend */
  /* ------------------ */

  /* !element */
  list = wrap_range_prepend(NUMBERSET_INVALID_CHARACTER, NULL);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* !list */
  list = wrap_range_prepend(NULL, NUMBERSET_INVALID_CHARACTER);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* different range types */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = wrap_range_new(3, 4, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  assert(list == NUMBERSET_INVALID_RANGE);
  free(nr);
  free(nr1);

  /* ranges not ascending (different intervals) */
  nr = wrap_range_new(10, 12, num_wraps, wraps);
  nr1 = wrap_range_new(1, 2, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  assert(list == NUMBERSET_NOT_ASCENDING);
  free(nr);
  free(nr1);

  /* appending to real wrap-around range */
  nr = wrap_range_new(12, 10, num_wraps, wraps);
  nr1 = wrap_range_new(13, 14, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* ranges not ascending (same interval) */
  nr = wrap_range_new(12, 14, num_wraps, wraps);
  nr1 = wrap_range_new(10, 11, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  assert(list == NUMBERSET_NOT_ASCENDING);
  free(nr);
  free(nr1);

  /* ranges overlapping (same interval) */
  nr = wrap_range_new(1, 3, num_wraps, wraps);
  nr1 = wrap_range_new(2, 4, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* ranges overlapping (with real wrap-around range) */
  nr = wrap_range_new(12, 13, num_wraps, wraps);
  nr1 = wrap_range_new(15, 16, num_wraps, wraps);
  nr2 = wrap_range_new(17, 18, num_wraps, wraps);
  nr3 = wrap_range_new(19, 14, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  list = wrap_range_prepend(list, nr2);
  list = wrap_range_prepend(list, nr3);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);
  free(nr2);
  free(nr3);

  /* normal prepend (different intervals) */
  nr = wrap_range_new(1, 2, num_wraps, wraps);
  nr1 = wrap_range_new(18, 20, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "18-20, 1-2"));
  number_set_free(list);
  free(s);

  /* normal prepend (same interval) */
  nr = wrap_range_new(11, 12, num_wraps, wraps);
  nr1 = wrap_range_new(18, 20, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "18-20, 11-12"));
  number_set_free(list);
  free(s);

  /* prepend real wrap-around range (different intervals) */
  nr = wrap_range_new(3, 4, num_wraps, wraps);
  nr1 = wrap_range_new(6, 7, num_wraps, wraps);
  nr2 = wrap_range_new(12, 13, num_wraps, wraps);
  nr3 = wrap_range_new(19, 14, num_wraps, wraps);
  list = wrap_range_prepend(nr, nr1);
  list = wrap_range_prepend(list, nr2);
  list = wrap_range_prepend(list, nr3);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "19-14, 12-13, 6-7, 3-4"));
  number_set_free(list);
  free(s);

  /* number_set_insert */
  /* ----------------- */

  /* !element */
  list = number_set_insert(NUMBERSET_INVALID_CHARACTER, NULL);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* !list */
  list = number_set_insert(NULL, NUMBERSET_INVALID_CHARACTER);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* different range types */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = wrap_range_new(3, 4, num_wraps, wraps);
  list = number_set_insert(nr, nr1);
  assert(list == NUMBERSET_INVALID_RANGE);
  free(nr);
  free(nr1);

  /* ranges overlapping */
  nr = number_set_new(1, 5, 1, 5);
  nr1 = number_set_new(1, 2, 1, 2);
  list = number_set_insert(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* merge adjacent ranges (right) */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(8, 9, 8, 9);
  nr2 = number_set_new(3, 4, 3, 4);
  list = number_set_insert(nr, nr1);
  list = number_set_insert(list, nr2);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "8-9, 1-4"));
  number_set_free(list);
  free(s);

  /* merge adjacent ranges (left) */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(8, 9, 8, 9);
  nr2 = number_set_new(6, 7, 6, 7);
  list = number_set_insert(nr, nr1);
  list = number_set_insert(list, nr2);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "6-9, 1-2"));
  number_set_free(list);
  free(s);

  /* merge adjacent ranges (middle) */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(5, 6, 5, 6);
  nr2 = number_set_new(3, 4, 3, 4);
  list = number_set_insert(nr, nr1);
  list = number_set_insert(list, nr2);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "1-6"));
  number_set_free(list);
  free(s);

  /* prepend */
  nr = number_set_new(4, 5, 4, 5);
  nr1 = number_set_new(1, 2, 1, 2);
  list = number_set_insert(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "4-5, 1-2"));
  number_set_free(list);
  free(s);

  /* normal insert */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = number_set_new(7, 8, 7, 8);
  nr2 = number_set_new(4, 5, 4, 5);
  list = number_set_insert(nr, nr1);
  list = number_set_insert(list, nr2);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "7-8, 4-5, 1-2"));
  number_set_free(list);
  free(s);

  /* wrap_range_insert */
  /* ----------------- */

  /* !element */
  list = wrap_range_insert(NUMBERSET_INVALID_CHARACTER, NULL);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* !list */
  list = wrap_range_insert(NULL, NUMBERSET_INVALID_CHARACTER);
  assert(list == NUMBERSET_INVALID_CHARACTER);

  /* different range types */
  nr = number_set_new(1, 2, 1, 2);
  nr1 = wrap_range_new(3, 4, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  assert(list == NUMBERSET_INVALID_RANGE);
  free(nr);
  free(nr1);

  /* ranges overlapping */
  nr = wrap_range_new(1, 4, num_wraps, wraps);
  nr1 = wrap_range_new(1, 2, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* appending real wrap-around range to real wrap-around range */
  /* (same interval) */
  nr = wrap_range_new(12, 10, num_wraps, wraps);
  nr1 = wrap_range_new(13, 11, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* appending to real wrap-around range (same interval) */
  nr = wrap_range_new(12, 10, num_wraps, wraps);
  nr1 = wrap_range_new(13, 14, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  free(nr);
  free(nr1);

  /* prepend (different intervals) */
  nr = wrap_range_new(14, 17, num_wraps, wraps);
  nr1 = wrap_range_new(1, 2, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "14-17, 1-2"));
  number_set_free(list);
  free(s);

  /* prepend (same interval) */
  nr = wrap_range_new(3, 4, num_wraps, wraps);
  nr1 = wrap_range_new(1, 2, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "3-4, 1-2"));
  number_set_free(list);
  free(s);

  /* append (different intervals) */
  nr = wrap_range_new(1, 2, num_wraps, wraps);
  nr1 = wrap_range_new(14, 17, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "14-17, 1-2"));
  number_set_free(list);
  free(s);

  /* append (same interval) */
  nr = wrap_range_new(1, 2, num_wraps, wraps);
  nr1 = wrap_range_new(3, 4, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "3-4, 1-2"));
  number_set_free(list);
  free(s);

  /* insert (with real wrap-around range) */
  nr = wrap_range_new(13, 14, num_wraps, wraps);
  nr1 = wrap_range_new(18, 12, num_wraps, wraps);
  nr2 = wrap_range_new(15, 16, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  list = wrap_range_insert(list, nr2);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "18-12, 15-16, 13-14"));
  number_set_free(list);
  free(s);

  /* number_set_reverse */
  /* ------------------ */

  /* !list */
  list = number_set_reverse(NULL);
  assert(list == NULL);

  /* normal list */
  nr = wrap_range_new(13, 14, num_wraps, wraps);
  nr1 = wrap_range_new(18, 12, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  list = number_set_reverse(list);
  s = number_set_show(list, -1, -1);
  assert(!strcmp(s, "13-14, 18-12"));
  number_set_free(list);
  free(s);

  /* number_set_get_first */
  /* -------------------- */

  /* !iter */
  i = number_set_get_first(NULL);
  assert(i == -1);

  /* !iter->range */
  iter.range = NULL;
  i = number_set_get_first(&iter);
  assert(i == -1);

  /* number_set_get_next */
  /* ------------------- */

  /* !iter */
  i = number_set_get_next(NULL);
  assert(i == -1);

  /* !iter->range */
  iter.range = NULL;
  i = number_set_get_next(&iter);
  assert(i == -1);

  /* number_set_get_first & number_set_get_next (single normal range) */
  /* ---------------------------------------------------------------- */

  list = number_set_new(12, 18, 12, 18);
  iter.range = list;
  s = (char*)malloc(100);
  p = s;
  i = number_set_get_first(&iter);
  while (i >= 0)
  {
    p += sprintf(p, "%d ", i);
    i = number_set_get_next(&iter);
  }
  assert(!strcmp(s, "12 13 14 15 16 17 18 "));
  number_set_free(list);
  free(s);


  /* number_set_get_first & number_set_get_next (single wrap-around range) */
  /* --------------------------------------------------------------------- */

  list = wrap_range_new(18, 12, num_wraps, wraps);
  iter.range = list;
  s = (char*)malloc(100);
  p = s;
  i = number_set_get_first(&iter);
  while (i >= 0)
  {
    p += sprintf(p, "%d ", i);
    i = number_set_get_next(&iter);
  }
  assert(!strcmp(s, "18 19 20 10 11 12 "));
  number_set_free(list);
  free(s);

  /* number_set_get_first & number_set_get_next (two ranges) */
  /* ------------------------------------------------------- */

  nr = wrap_range_new(13, 14, num_wraps, wraps);
  nr1 = wrap_range_new(18, 12, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  list = number_set_reverse(list);
  iter.range = list;
  s = (char*)malloc(100);
  p = s;
  i = number_set_get_first(&iter);
  while (i >= 0)
  {
    p += sprintf(p, "%d ", i);
    i = number_set_get_next(&iter);
  }
  assert(!strcmp(s, "13 14 18 19 20 10 11 12 "));
  number_set_free(list);
  free(s);

  /* number_set_get_first & number_set_get_next (three ranges) */
  /* --------------------------------------------------------- */

  nr = wrap_range_new(2, 3, num_wraps, wraps);
  nr1 = wrap_range_new(8, 6, num_wraps, wraps);
  nr2 = wrap_range_new(18, 12, num_wraps, wraps);
  list = wrap_range_insert(nr, nr1);
  list = wrap_range_insert(list, nr2);
  list = number_set_reverse(list);
  iter.range = list;
  s = (char*)malloc(100);
  p = s;
  i = number_set_get_first(&iter);
  while (i >= 0)
  {
    p += sprintf(p, "%d ", i);
    i = number_set_get_next(&iter);
  }
  assert(!strcmp(s, "2 3 8 9 5 6 18 19 20 10 11 12 "));
  number_set_free(list);
  free(s);

  /* number_set_is_element */
  /* --------------------- */

  /* normal range */
  nr = number_set_new(12, 18, 12, 18);
  s = (char*)malloc(100);
  p = s;
  for (i = 8; i < 23; i++)
    if (number_set_is_element(nr, i))
      p += sprintf(p, "%d ", i);
  assert(!strcmp(s, "12 13 14 15 16 17 18 "));
  number_set_free(nr);
  free(s);

  /* real wrap-around range */
  nr = wrap_range_new(18, 12, num_wraps, wraps);
  s = (char*)malloc(100);
  p = s;
  for (i = 8; i < 23; i++)
    if (number_set_is_element(nr, i))
      p += sprintf(p, "%d ", i);
  assert(!strcmp(s, "10 11 12 18 19 20 "));
  number_set_free(nr);
  free(s);

  /* number_set_parse */
  /* ---------------- */

  /* !s */
  r = number_set_parse(NULL, NULL, 0, 0);

  /* min < 0, max < 0, empty input */
  in = "";
  r = number_set_parse(in, NULL, -1, -1);
  assert(r == in);

  /* min > max, numeric overflow */
  list = NULL;
  s = (char*)malloc(100);
  sprintf(s, "%ld", (long)INT_MAX * 2);
  r = number_set_parse(s, &list, 10, 5);
  assert(list == NUMBERSET_OVERFLOW);
  assert(r == s);
  free(s);

  /* numeric overflow of range */
  list = NULL;
  s = (char*)malloc(100);
  sprintf(s, "1-%ld", (long)INT_MAX * 2);
  r = number_set_parse(s, &list, 10, 5);
  assert(list == NUMBERSET_OVERFLOW);
  assert(r == s);
  free(s);

  /* invalid range (n < min) */
  list = NULL;
  in = "3-5";
  r = number_set_parse(in, &list, 4, 6);
  assert(list == NUMBERSET_INVALID_RANGE);
  assert(r == in);

  /* invalid range (m > max) */
  list = NULL;
  in = "3-5";
  r = number_set_parse(in, &list, 1, 2);
  assert(list == NUMBERSET_INVALID_RANGE);
  assert(r == in);

  /* not ascending */
  list = NULL;
  in = "3-5 1-3";
  r = number_set_parse(in, &list, 0, 6);
  assert(list == NUMBERSET_NOT_ASCENDING);
  assert(r == in + strlen("3-5 "));

  /* overlapping ranges */
  list = NULL;
  in = "3-5 4-6";
  r = number_set_parse(in, &list, 0, 7);
  assert(list == NUMBERSET_OVERLAPPING_RANGES);
  assert(r == in + strlen("3-5 "));

  /* invalid character */
  list = NULL;
  in = "123.456";
  r = number_set_parse(in, &list, -1, -1);
  assert(list == NUMBERSET_INVALID_CHARACTER);
  assert(r == in);

  /* whitespace, m < n, no list */
  list = NULL;
  in = " 10 - 5";
  out = "5-10";
  r = number_set_parse(in, &list, 10, 5);
  s = number_set_show(list, 2, 12);
  assert(r == in + strlen(in));
  assert(!strcmp(s, out));
  number_set_free(list);
  free(s);

  /* no output */
  in = "5-10";
  r = number_set_parse(in, NULL, 10, 5);
  assert(r == in + strlen(in));

  /* multiple range type representation forms, with merging */
  list = NULL;
  in = "-3, 4, 6-8, 10-";
  out = "-4, 6-8, 10-";
  r = number_set_parse(in, &list, 1, 13);
  s = number_set_show(list, 2, 12);
  assert(r == in + strlen(in));
  assert(!strcmp(s, out));
  number_set_free(list);
  free(s);

  return 0;
}

/* end of numberset-test.c */
