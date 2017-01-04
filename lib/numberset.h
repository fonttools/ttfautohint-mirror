/* numberset.h */

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


#ifndef NUMBERSET_H_
#define NUMBERSET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A structure defining an integer range, to be used as a linked list.  It
 * gets allocated by a successful call to `number_set_parse'.  Use
 * `number_set_free' to deallocate it.
 */

typedef struct number_range_
{
  int start;
  int end;

  struct number_range_* next;
} number_range;


/*
 * Create and initialize a `number_range' object.  In case of an allocation
 * error, return NUMBERSET_ALLOCATION_ERROR.  If either `start' or `end'
 * exceeds the range [min;max], return NUMBERSET_INVALID_RANGE.
 */

number_range*
number_set_new(int start,
               int end,
               int min,
               int max);


/*
 * Prepend a single `number_range' object `element' to `list' of
 * `number_range' objects, which might be NULL.  `list' is expected to be
 * stored in reversed order; consequently, the range in `element' must be
 * larger than the first element of `list', otherwise an error is returned.
 * If possible, the ranges of `element' and the first element of `list' are
 * merged, in which case `element' gets deallocated.
 *
 * If `element' is NULL, return `list'.
 */

number_range*
number_set_prepend(number_range* list,
                   number_range* element);


/*
 * Insert a single `number_range' object `element' into `list' of
 * `number_range' objects, which might be NULL.  `list' is expected to be
 * stored in reversed order.  If possible, the ranges of `element' and
 * `list' are merged, in which case `element' gets deallocated.
 *
 * If `element' is NULL, return `list'.
 */

number_range*
number_set_insert(number_range* list,
                  number_range* element);


/*
 * Merge adjacent ranges in a list of `number_range' objects (which must be
 * in normal order), deallocating list elements where appropriate.
 */

number_range*
number_set_normalize(number_range* list);


/*
 * Reverse a list of `number_range' objects.
 */

number_range*
number_set_reverse(number_range* list);


/*
 * Parse a description in string `s' for a set of non-negative integers
 * within the limits given by the input parameters `min' and `max', and
 * which consists of the following ranges, separated by commas (`n' and `m'
 * are non-negative integers):
 *
 *   -n       min <= x <= n
 *   n        x = n; this is a shorthand for `n-n'
 *   n-m      n <= x <= m (or  m <= x <= n  if  m < n)
 *   m-       m <= x <= max
 *   -        min <= x <= max
 *
 * Superfluous commas are ignored, as is whitespace around numbers, dashes,
 * and commas.  The ranges must be ordered, without overlaps.  As a
 * consequence, `-n' and `m-' can occur at most once and must be then the
 * first and last range, respectively; similarly, `-' cannot be paired with
 * any other range.
 *
 * In the following examples, `min' is 4 and `max' is 12:
 *
 *   -                ->    4, 5, 6, 7, 8, 9, 10, 11, 12
 *   -3, 5-           ->    invalid first range
 *   4, 6-8, 10-      ->    4, 6, 7, 8, 10, 11, 12
 *   4-8, 6-10        ->    invalid overlapping ranges
 *
 * In case of success (this is, the number set description in `s' is valid)
 * the return value is a pointer to the final zero byte in string `s'.  In
 * case of an error, the return value is a pointer to the beginning position
 * of the offending range in string `s'.
 *
 * If s is NULL, the function exits immediately with NULL as the return
 * value.
 *
 * If the user provides a non-NULL `number_set' value, `number_set_parse'
 * stores a linked list of ordered number ranges in `*number_set', allocated
 * with `malloc'.  If there is no range at all (for example, an empty string
 * or whitespace and commas only) no data gets allocated, and `*number_set'
 * is set to NULL.  In case of error, `*number_set' returns an error code;
 * you should use the following macros to compare with.
 *
 *   NUMBERSET_INVALID_CHARACTER   invalid character in description string
 *   NUMBERSET_OVERFLOW            numerical overflow
 *   NUMBERSET_INVALID_RANGE       invalid range, exceeding `min' or `max'
 *   NUMBERSET_OVERLAPPING_RANGES  overlapping ranges
 *   NUMBERSET_NOT_ASCENDING       not ascending ranges or values
 *   NUMBERSET_ALLOCATION_ERROR    allocation error
 *
 * Note that a negative value for `min' is replaced with zero, and a
 * negative value for `max' with the largest representable integer, INT_MAX.
 */

#define NUMBERSET_INVALID_CHARACTER (number_range*)-1
#define NUMBERSET_OVERFLOW (number_range*)-2
#define NUMBERSET_INVALID_RANGE (number_range*)-3
#define NUMBERSET_OVERLAPPING_RANGES (number_range*)-4
#define NUMBERSET_NOT_ASCENDING (number_range*)-5
#define NUMBERSET_ALLOCATION_ERROR (number_range*)-6

const char*
number_set_parse(const char* s,
                 number_range** number_set,
                 int min,
                 int max);


/*
 * Free the allocated data in `number_set'.
 */

void
number_set_free(number_range* number_set);


/*
 * Return a string representation of `number_set', viewed through a
 * `window', so to say, spanned up by the parameters `min' and `max'.  After
 * use, the string should be deallocated with a call to `free'.  In case of
 * an allocation error, the return value is NULL.
 *
 * Note that a negative value for `min' is replaced with zero, and a
 * negative value for `max' with the largest representable integer, INT_MAX.
 */

char*
number_set_show(number_range* number_set,
                int min,
                int max);


/*
 * Return value 1 if `number' is element of `number_set', zero otherwise.
 */

int
number_set_is_element(number_range* number_set,
                      int number);


/*
 * A structure used to iterate over a number set.
 */

typedef struct number_set_iter_
{
  number_range* range;
  int val;
} number_set_iter;


/*
 * Get first element of a number set.  `iter_p' must be initialized with the
 * `number_range' structure to iterate over.  After the call, `iter_p' is
 * ready to be used in a call to `number_set_get_next'.
 */

int
number_set_get_first(number_set_iter* iter_p);


/*
 * Get next element of a number set, using `iter_p' from a previous call to
 * `number_set_get_first' or `number_set_get_next'.  If `iter_p->range' is
 * NULL after the call, there is no next element, and the return value is
 * undefined.
 */

int
number_set_get_next(number_set_iter* iter_p);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NUMBERSET_H_ */

/* end of numberset.h */
