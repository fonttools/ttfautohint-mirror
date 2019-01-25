/* numberset.h */

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


#ifndef NUMBERSET_H_
#define NUMBERSET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A structure defining a range or wrap-around range of non-negative
 * integers, to be used as a linked list.  It gets allocated by a successful
 * call to `number_set_parse', `number_set_new', and `wrap_range_new'.  Use
 * `number_set_free' to deallocate it.
 *
 * If `base' and `wrap' are not equal, we have a `wrap-around range'.  These
 * two values define a frame which encloses `start' and `end'; `start' can
 * be larger than `end' to indicate wrapping at `wrap', starting again with
 * value `base'.  Example:
 *
 *   start=17, end=14,
 *   base=13, wrap=18  -->  17, 18, 13, 14
 *
 * Normal integer ranges can be merged.  For example, the ranges 3-6 and 7-8
 * can be merged into 3-8, and functions like `number_set_prepend' do this
 * automatically.
 *
 * Wrap-around ranges will not be merged; this is by design to reflect the
 * intended usage of this library (namely to represent groups of
 * horizontally aligned points on a closed glyph outline contour).
 * Additionally, for a given [base;wrap] interval there can only be a single
 * wrap-around range that actually does wrapping; it gets sorted after the
 * other non-wrapping ranges for the same [base;wrap] interval.
 */

typedef struct number_range_
{
  /* all values are >= 0 */
  int start;
  int end;
  int base;
  int wrap;

  struct number_range_* next;
} number_range;


/*
 * Create and initialize a `number_range' object, holding a normal integer
 * range.  In case of an allocation error, return
 * NUMBERSET_ALLOCATION_ERROR.
 *
 * A negative value for `min' is replaced with zero, and a negative value
 * for `max' with the largest representable integer, INT_MAX.
 *
 * If either `start' or `end' exceeds the range [min;max], return
 * NUMBERSET_INVALID_RANGE.
 */

number_range*
number_set_new(int start,
               int end,
               int min,
               int max);


/*
 * Create and initialize a wrap-around range.  In case of an allocation
 * error, return NUMBERSET_ALLOCATION_ERROR.
 *
 * `wraps' specifies an array of at least two `wrap points', in strictly
 * ascending order, with `num_wraps' elements.  For creating a valid
 * wrap-around range, there must exist a pair of adjacent elements in the
 * `wraps' array that enclose `start' and `end'.  To be more precise, if
 * `wA=wraps[n]' and `wB=wraps[n+1]' denote the two adjacent elements of
 * `wraps', both `start' and `end' must be in the range ]wA;wB].  If this
 * constraint is not met, return NUMBERSET_INVALID_RANGE.
 *
 * A corollary of the definitions of `wraps' and `number_range' is that the
 * elements of `wraps' must be all different and non-negative except the
 * first element, which can be -1.
 *
 * For convenience, normal integer ranges and wrap-around ranges use the
 * same data structure (`number_range').  However, calls to `number_set_new'
 * and `wrap_range_new' can't be mixed: Either use the former function for
 * all calls, or you use the latter; you will get an NUMBERSET_INVALID_RANGE
 * error otherwise.
 *
 * Here are some examples that demonstrate the resulting elements of
 * wrap-around ranges for a given `wraps' array and various `start' and
 * `end' values.
 *
 *   wraps = {-1, 4, 9}
 *
 *     range     elements
 *    -------------------------------------------------
 *      4-0      4, 0
 *      6-8      6, 7, 8
 *      8-6      8, 9, 5, 6
 *      3-6      invalid, crossing wrap point 4
 *     10-11     invalid, outside of wrap points array
 *
 * Note that you get undefined results if the elements of `wraps' change
 * between calls to this function.
 */

number_range*
wrap_range_new(int start,
               int end,
               size_t num_wraps,
               int* wraps);


/*
 * Return 0 if the setup of `wraps', as described above, is valid,
 * and 1 otherwise.
 */

int
wrap_range_check_wraps(size_t num_wraps,
                       int* wraps);


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
 * Prepend a single `number_range' object `element' to `list' of
 * `number_range' objects, which might be NULL.  `list' is expected to be
 * stored in reversed order.  By design, there is no range merging.
 *
 * If `element' is NULL, return `list'.
 */

number_range*
number_set_prepend_unsorted(number_range* list,
                            number_range* element);


/*
 * Prepend a single wrap-around `number_range' object `element' to `list' of
 * (wrap-around) `number_range' objects, which might be NULL.  `list' is
 * expected to be stored in reversed order; consequently, the range in
 * `element' must be larger than the first element of `list', otherwise an
 * error is returned.
 *
 * If `element' is NULL, return `list'.
 */

number_range*
wrap_range_prepend(number_range* list,
                   number_range* element);


/*
 * Insert a single `number_range' object `element' into `list' of
 * `number_range' objects, which might be NULL.  `list' is expected to be
 * stored in reversed order.  If possible, the ranges of `element' and
 * `list' are merged, in which case `element' gets deallocated.
 *
 * Don't use this function for unsorted lists (i.e., lists created with
 * `number_set_prepend_unsorted'); you will get undefined behaviour
 * otherwise.
 *
 * If `element' is NULL, return `list'.
 */

number_range*
number_set_insert(number_range* list,
                  number_range* element);


/*
 * Insert a single wrap-around `number_range' object `element' into `list'
 * of (wrap-around) `number_range' objects, which might be NULL.  `list' is
 * expected to be stored in reversed order.
 *
 * If `element' is NULL, return `list'.
 */

number_range*
wrap_range_insert(number_range* list,
                  number_range* element);


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
 *
 * `number_set_parse' is not suited to create wrap-around ranges; this only
 * works with `wrap_range_new'.
 */

#define NUMBERSET_INVALID_CHARACTER (number_range*)-1
#define NUMBERSET_OVERFLOW (number_range*)-2
#define NUMBERSET_INVALID_RANGE (number_range*)-3
#define NUMBERSET_OVERLAPPING_RANGES (number_range*)-4
#define NUMBERSET_NOT_ASCENDING (number_range*)-5
#define NUMBERSET_ALLOCATION_ERROR (number_range*)-6

/*
 * `wrap_range_new' can return an additional error code in case no valid
 * interval could be found.
 */
#define NUMBERSET_INVALID_WRAP_RANGE (number_range*)-7

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
 *
 * If `number_set' represents wrap-around ranges, `min' and `max' are
 * ignored.
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
 *
 * If there is no valid first element, return -1.
 */

int
number_set_get_first(number_set_iter* iter_p);


/*
 * Get next element of a number set, using `iter_p' from a previous call to
 * `number_set_get_first' or `number_set_get_next'.  If there is no next
 * valid element, return -1.
 */

int
number_set_get_next(number_set_iter* iter_p);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NUMBERSET_H_ */

/* end of numberset.h */
