// info.cpp

// Copyright (C) 2012-2019 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// the next header file is from gnulib defining function `last_component',
// which is a replacement for `basename' (but just returning a pointer
// without allocating a string) that works on Windows also
#include "dirname.h"

#include "info.h"
#include <sds.h>
#include <numberset.h>


#define TTFAUTOHINT_STRING "; ttfautohint"
#define TTFAUTOHINT_STRING_WIDE "\0;\0 \0t\0t\0f\0a\0u\0t\0o\0h\0i\0n\0t"


extern "C" {

const char invalid_ps_chars[96] =
{
  0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, // 0x20  %, (, ), /
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, // 0x30  <, >
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x40
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, // 0x50  [, ]
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x60
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, // 0x70  {, }, DEL
};


char*
check_family_suffix(const char* s)
{
  while (*s)
  {
    int c = (signed char)*s - 0x20;

    // valid range is 0x20-0x7E minus characters %()/<>[]{}
    // (well, the space character 0x20 is not valid within a PS name,
    // but the validity test gets executed before constructing a PS name,
    // which has all space characters removed)
    if (c < 0 || invalid_ps_chars[c])
      return (char*)s;

    s++;
  }

  return NULL;
}


// build string that gets appended to the `Version' field(s)
// return value 1 means allocation error, value 2 too long a string
int
build_version_string(Info_Data* idata)
{
  // since we use `goto' we have to declare variables before the jumps
  unsigned char* info_string;
  unsigned char* info_string_wide;
  unsigned char* dt;
  unsigned char* dtw;
  char* s = NULL;
  char mode[4];
  char mode_letters[] = "nqs";
  int ret = 0;
  sds d;

  d = sdsempty();

  d = sdscatprintf(d, TTFAUTOHINT_STRING " (v%s)", VERSION);
  if (!idata->detailed_info)
    goto Skip;

  if (idata->dehint)
  {
    d = sdscat(d, " -d");
    goto Skip;
  }
  d = sdscatprintf(d, " -l %d", idata->hinting_range_min);
  d = sdscatprintf(d, " -r %d", idata->hinting_range_max);
  d = sdscatprintf(d, " -G %d", idata->hinting_limit);
  d = sdscatprintf(d, " -x %d", idata->increase_x_height);
  if (idata->fallback_stem_width)
    d = sdscatprintf(d, " -H %d", idata->fallback_stem_width);
  d = sdscatprintf(d, " -D %s", idata->default_script);
  d = sdscatprintf(d, " -f %s", idata->fallback_script);
  if (idata->control_name)
  {
    char* bn = last_component(idata->control_name);
    d = sdscatprintf(d, " -m \"%s\"", bn ? bn : idata->control_name);
  }
  if (idata->reference_name)
  {
    char* bn = last_component(idata->reference_name);
    d = sdscatprintf(d, " -R \"%s\"", bn ? bn : idata->reference_name);

    d = sdscatprintf(d, " -Z %d", idata->reference_index);
  }
  // `*_stem_width_mode' can have values -1, 0, and 1
  mode[0] = mode_letters[idata->gray_stem_width_mode + 1];
  mode[1] = mode_letters[idata->gdi_cleartype_stem_width_mode + 1];
  mode[2] = mode_letters[idata->dw_cleartype_stem_width_mode + 1];
  mode[3] = '\0';
  d = sdscatprintf(d, " -a %s", mode);

  if (idata->windows_compatibility)
    d = sdscat(d, " -W");
  if (idata->adjust_subglyphs)
    d = sdscat(d, " -p");
  if (idata->hint_composites)
    d = sdscat(d, " -c");
  if (idata->symbol)
    d = sdscat(d, " -s");
  if (idata->fallback_scaling)
    d = sdscat(d, " -S");
  if (idata->TTFA_info)
    d = sdscat(d, " -t");

  if (idata->x_height_snapping_exceptions_string)
  {
    // only set specific value of `ret' for an allocation error,
    // since syntax errors are handled in TTF_autohint
    number_range* x_height_snapping_exceptions;
    const char* pos = number_set_parse(
                        idata->x_height_snapping_exceptions_string,
                        &x_height_snapping_exceptions,
                        6, 0x7FFF);
    if (*pos)
    {
      if (x_height_snapping_exceptions == NUMBERSET_ALLOCATION_ERROR)
        ret = 1;
      goto Fail;
    }

    s = number_set_show(x_height_snapping_exceptions, 6, 0x7FFF);
    number_set_free(x_height_snapping_exceptions);

    // ensure UTF16-BE version doesn't get too long
    if (strlen(s) > 0xFFFF / 2 - sdslen(d))
    {
      ret = 2;
      goto Fail;
    }

    d = sdscatprintf(d, " -X \"%s\"", s);
  }

Skip:
  if (!d)
  {
    ret = 1;
    goto Fail;
  }

  info_string = (unsigned char*)malloc(sdslen(d) + 1);
  if (!info_string)
  {
    ret = 1;
    goto Fail;
  }
  memcpy(info_string, d, sdslen(d) + 1);

  idata->info_string = info_string;
  idata->info_string_len = (unsigned short)sdslen(d);

  // prepare UTF16-BE version data
  idata->info_string_wide_len = 2 * idata->info_string_len;
  info_string_wide = (unsigned char*)realloc(idata->info_string_wide,
                                             idata->info_string_wide_len);
  if (!info_string_wide)
  {
    ret = 1;
    goto Fail;
  }
  idata->info_string_wide = info_string_wide;

  dt = idata->info_string;
  dtw = idata->info_string_wide;
  for (unsigned short i = 0; i < idata->info_string_len; i++)
  {
    *(dtw++) = '\0';
    *(dtw++) = *(dt++);
  }

Exit:
  free(s);
  sdsfree(d);

  return ret;

Fail:
  free(idata->info_string);
  free(idata->info_string_wide);

  idata->info_string = NULL;
  idata->info_string_wide = NULL;
  idata->info_string_len = 0;
  idata->info_string_wide_len = 0;

  goto Exit;
}


static int
info_name_id_5(unsigned short platform_id,
               unsigned short encoding_id,
               unsigned short* len,
               unsigned char** str,
               Info_Data* idata)
{
  unsigned char ttfautohint_string[] = TTFAUTOHINT_STRING;
  unsigned char ttfautohint_string_wide[] = TTFAUTOHINT_STRING_WIDE;

  // we use memmem, so don't count the trailing \0 character
  size_t ttfautohint_string_len = sizeof (TTFAUTOHINT_STRING) - 1;
  size_t ttfautohint_string_wide_len = sizeof (TTFAUTOHINT_STRING_WIDE) - 1;

  unsigned char* v;
  unsigned short v_len;
  unsigned char* s;
  size_t s_len;
  size_t offset;

  if (platform_id == 1
      || (platform_id == 3 && !(encoding_id == 1
                                || encoding_id == 10)))
  {
    // one-byte or multi-byte encodings
    v = idata->info_string;
    v_len = idata->info_string_len;
    s = ttfautohint_string;
    s_len = ttfautohint_string_len;
    offset = 2;
  }
  else
  {
    // (two-byte) UTF-16BE for everything else
    v = idata->info_string_wide;
    v_len = idata->info_string_wide_len;
    s = ttfautohint_string_wide;
    s_len = ttfautohint_string_wide_len;
    offset = 4;
  }

  // if we already have an ttfautohint info string,
  // remove it up to a following `;' character (or end of string)
  unsigned char* s_start = (unsigned char*)memmem(*str, *len, s, s_len);
  if (s_start)
  {
    unsigned char* s_end = s_start + offset;
    unsigned char* limit = *str + *len;

    while (s_end < limit)
    {
      if (*s_end == ';')
      {
        if (offset == 2)
          break;
        else
        {
          if (*(s_end - 1) == '\0') // UTF-16BE
          {
            s_end--;
            break;
          }
        }
      }

      s_end++;
    }

    while (s_end < limit)
      *s_start++ = *s_end++;

    *len -= s_end - s_start;
  }

  // do nothing if the string would become too long
  if (*len > 0xFFFF - v_len)
    return 0;

  unsigned short len_new = *len + v_len;
  unsigned char* str_new = (unsigned char*)realloc(*str, len_new);
  if (!str_new)
    return 1;

  *str = str_new;
  memcpy(*str + *len, v, v_len);
  *len = len_new;

  return 0;
}


// a structure to collect family data for a given
// (platform_id, encoding_id, language_id) triplet

typedef struct Family_
{
  unsigned short platform_id;
  unsigned short encoding_id;
  unsigned short language_id;

  unsigned short* name_id_1_len;
  unsigned char** name_id_1_str;
  unsigned short* name_id_4_len;
  unsigned char** name_id_4_str;
  unsigned short* name_id_6_len;
  unsigned char** name_id_6_str;
  unsigned short* name_id_16_len;
  unsigned char** name_id_16_str;
  unsigned short* name_id_21_len;
  unsigned char** name_id_21_str;

  sds family_name;
} Family;


// node structure for collected family data

typedef struct Node Node;
struct Node
{
  LLRB_ENTRY(Node) entry;
  Family family;
};


// comparison function for our red-black tree

static int
nodecmp(Node *e1,
        Node *e2)
{
  int diff;

  // sort by platform ID ...
  diff = e1->family.platform_id - e2->family.platform_id;
  if (diff)
    goto Exit;

  // ... then by encoding ID ...
  diff = e1->family.encoding_id - e2->family.encoding_id;
  if (diff)
    goto Exit;

  // ... then by language ID
  diff = e1->family.language_id - e2->family.language_id;

Exit:
  // https://graphics.stanford.edu/~seander/bithacks.html#CopyIntegerSign
  return (diff > 0) - (diff < 0);
}


// the red-black tree function body
typedef struct family_data family_data;

LLRB_HEAD(family_data, Node);

// no trailing semicolon in the next line
LLRB_GENERATE_STATIC(family_data, Node, entry, nodecmp)


static void
family_data_free(Info_Data* idata)
{
  family_data* family_data_head = (family_data*)idata->family_data_head;

  Node* node;
  Node* next_node;

  if (!family_data_head)
    return;

  for (node = LLRB_MIN(family_data, family_data_head);
       node;
       node = next_node)
  {
    next_node = LLRB_NEXT(family_data, family_data_head, node);
    LLRB_REMOVE(family_data, family_data_head, node);
    sdsfree(node->family.family_name);
    free(node);
  }

  free(family_data_head);
}


static int
collect_family_data(unsigned short platform_id,
                    unsigned short encoding_id,
                    unsigned short language_id,
                    unsigned short name_id,
                    unsigned short* len,
                    unsigned char** str,
                    Info_Data* idata)
{
  family_data* family_data_head = (family_data*)idata->family_data_head;
  Node* node;
  Node* val;

  if (!family_data_head)
  {
    // first-time initialization
    family_data_head = (family_data*)malloc(sizeof (family_data));
    if (!family_data_head)
      return 1;

    LLRB_INIT(family_data_head);
    idata->family_data_head = family_data_head;
  }

  node = (Node*)malloc(sizeof (Node));
  if (!node)
    return 1;

  node->family.platform_id = platform_id;
  node->family.encoding_id = encoding_id;
  node->family.language_id = language_id;

  val = LLRB_INSERT(family_data, family_data_head, node);
  if (val)
  {
    // we already have an entry in the tree for our triplet
    free(node);
    node = val;
  }
  else
  {
    // initialize remaining fields
    node->family.name_id_1_len = NULL;
    node->family.name_id_1_str = NULL;
    node->family.name_id_4_len = NULL;
    node->family.name_id_4_str = NULL;
    node->family.name_id_6_len = NULL;
    node->family.name_id_6_str = NULL;
    node->family.name_id_16_len = NULL;
    node->family.name_id_16_str = NULL;
    node->family.name_id_21_len = NULL;
    node->family.name_id_21_str = NULL;

    node->family.family_name = NULL;
  }

  if (name_id == 1)
  {
    node->family.name_id_1_len = len;
    node->family.name_id_1_str = str;
  }
  else if (name_id == 4)
  {
    node->family.name_id_4_len = len;
    node->family.name_id_4_str = str;
  }
  else if (name_id == 6)
  {
    node->family.name_id_6_len = len;
    node->family.name_id_6_str = str;
  }
  else if (name_id == 16)
  {
    node->family.name_id_16_len = len;
    node->family.name_id_16_str = str;
  }
  else if (name_id == 21)
  {
    node->family.name_id_21_len = len;
    node->family.name_id_21_str = str;
  }

  return 0;
}


// `info-callback' function

int
info(unsigned short platform_id,
     unsigned short encoding_id,
     unsigned short language_id,
     unsigned short name_id,
     unsigned short* len,
     unsigned char** str,
     void* user)
{
  Info_Data* idata = (Info_Data*)user;

  // if ID is a version string, append our data
  if (!idata->no_info && name_id == 5)
    return info_name_id_5(platform_id, encoding_id, len, str, idata);

  // if ID is related to a family name, collect the data
  if (*idata->family_suffix
      && (name_id == 1
          || name_id == 4
          || name_id == 6
          || name_id == 16
          || name_id == 21))
    return collect_family_data(platform_id,
                               encoding_id,
                               language_id,
                               name_id,
                               len,
                               str,
                               idata);

  return 0;
}


// Insert `suffix' to `str', right after substring `name'.
// If `name' isn't a substring of `str', append `suffix' to `str'.
// Do nothing in case of allocation error or if resulting string is too long.

static void
insert_suffix(sds suffix,
              sds name,
              unsigned short* len,
              unsigned char** str)
{
  if (!len || !*len || !str || !*str)
    return;

  sds s = sdsempty();

  // check whether `name' is a substring of `str'
  unsigned char* s_start = (unsigned char*)memmem(*str, *len,
                                                  name, sdslen(name));

  // construct new string
  if (s_start)
  {
    size_t substring_end = size_t(s_start - *str) + sdslen(name);

    // everything up to the end of the substring
    s = sdscatlen(s, *str, substring_end);
    // the suffix
    s = sdscatsds(s, suffix);
    // the rest
    s = sdscatlen(s, *str + substring_end, *len - substring_end);
  }
  else
  {
    s = sdscatlen(s, *str, *len);
    s = sdscatsds(s, suffix);
  }

  if (!s)
    return;

  if (sdslen(s) <= 0xFFFF)
  {
    unsigned short len_new = (unsigned short)sdslen(s);
    unsigned char* str_new = (unsigned char*)realloc(*str, len_new);
    if (str_new)
    {
      *str = str_new;
      memcpy(*str, s, len_new);
      *len = len_new;
    }
  }

  sdsfree(s);
}


// `info-post-callback' function

int
info_post(void* user)
{
  Info_Data* idata = (Info_Data*)user;
  family_data* family_data_head = (family_data*)idata->family_data_head;

  //
  // family_suffix + family_suffix_wide
  //
  sds family_suffix = sdsnew(idata->family_suffix);
  size_t family_suffix_len = sdslen(family_suffix);

  sds family_suffix_wide = sdsempty();
  size_t family_suffix_wide_len = 2 * family_suffix_len;

  // create sds with given size but uninitialized string data
  family_suffix_wide = sdsMakeRoomFor(family_suffix_wide,
                                      family_suffix_wide_len);

  char* fs;

  // construct `family_suffix_wide' by inserting '\0'
  fs = family_suffix;
  char *fsw = family_suffix_wide;
  for (size_t i = 0; i < family_suffix_len; i++)
  {
    *(fsw++) = '\0';
    *(fsw++) = *(fs++);
  }
  sdsIncrLen(family_suffix_wide, (int)family_suffix_wide_len);

  //
  // family_ps_suffix + family_ps_suffix_wide
  //
  sds family_ps_suffix = sdsempty();

  // create sds with estimated size but uninitialized string data;
  // we later set the size to the actual value
  family_ps_suffix = sdsMakeRoomFor(family_ps_suffix,
                                    family_suffix_len);

  // construct `family_ps_suffix' by removing all space characters
  fs = family_suffix;
  char *fps = family_ps_suffix;
  for (size_t i = 0; i < family_suffix_len; i++)
  {
    char c = *(fs++);
    if (c != ' ')
      *(fps++) = c;
  }
  // set correct size
  sdsIncrLen(family_ps_suffix, (int)(fps - family_ps_suffix));

  size_t family_ps_suffix_len = sdslen(family_ps_suffix);

  sds family_ps_suffix_wide = sdsempty();
  size_t family_ps_suffix_wide_len = 2 * family_ps_suffix_len;

  // create sds with given size but uninitialized string data
  family_ps_suffix_wide = sdsMakeRoomFor(family_ps_suffix_wide,
                                         family_ps_suffix_wide_len);

  // construct `family_ps_suffix_wide' by inserting '\0'
  fps = family_ps_suffix;
  char* fpsw = family_ps_suffix_wide;
  for (size_t i = 0; i < family_ps_suffix_len; i++)
  {
    *(fpsw++) = '\0';
    *(fpsw++) = *(fps++);
  }
  sdsIncrLen(family_ps_suffix_wide, (int)family_ps_suffix_wide_len);

  // We try the following algorithm.
  //
  // 1. If we have a `Preferred Family' (ID 16), use it as the family name,
  //    otherwise use the `Font Family Name' (ID 1).  If necessary, search
  //    other language IDs for the current (platform ID, encoding ID) pair
  //    to find a family name.
  //
  // 2. Append the family suffix to the family substring in the `Font Family
  //    Name' (ID 1), the `Full Font Name' (ID 4), the `Preferred Family'
  //    (ID 16), and the `WWS Family Name' (ID 21).  In case the family name
  //    found in step 1 is not a substring, append the suffix to the whole
  //    string.
  //
  // 3. Remove spaces from the family name and locate this substring in the
  //    `PostScript Name' (ID 6), then append the family suffix, also with
  //    spaces removed.  If we don't have a substring, append the stripped
  //    suffix to the whole string.

  // determine family name for all triplets if available
  for (Node* node = LLRB_MIN(family_data, family_data_head);
       node;
       node = LLRB_NEXT(family_data, family_data_head, node))
  {
    Family* family = &node->family;

    if (family->name_id_16_len && *family->name_id_16_len
        && family->name_id_16_str && *family->name_id_16_str)
      family->family_name = sdsnewlen(*family->name_id_16_str,
                                      *family->name_id_16_len);
    else if (family->name_id_1_len && *family->name_id_1_len
             && family->name_id_1_str && *family->name_id_1_str)
      family->family_name = sdsnewlen(*family->name_id_1_str,
                                      *family->name_id_1_len);
  }

  sds family_name = sdsempty();
  sds family_ps_name = sdsempty();

  // process all name ID strings in triplets
  for (Node* node = LLRB_MIN(family_data, family_data_head);
       node;
       node = LLRB_NEXT(family_data, family_data_head, node))
  {
    Family family = node->family;
    bool is_wide;

    sdsclear(family_name);
    sdsclear(family_ps_name);

    if (family.family_name)
      family_name = sdscatsds(family_name, family.family_name);
    else
    {
      Node* n;

      // use family name from a triplet that actually has one
      for (n = LLRB_MIN(family_data, family_data_head);
           n;
           n = LLRB_NEXT(family_data, family_data_head, n))
      {
        Family f = n->family;

        if (f.platform_id == family.platform_id
            && f.encoding_id == family.encoding_id
            && f.family_name)
        {
          family_name = sdscatsds(family_name, f.family_name);
          break;
        }
      }

      if (!n)
        continue; // no valid family name found
    }

    if (family.platform_id == 1
        || (family.platform_id == 3 && !(family.encoding_id == 1
                                         || family.encoding_id == 10)))
      is_wide = false; // one-byte or multi-byte encodings
    else
      is_wide = true; // (two-byte) UTF-16BE

    sds suffix = is_wide ? family_suffix_wide : family_suffix;
    insert_suffix(suffix,
                  family_name,
                  family.name_id_1_len,
                  family.name_id_1_str);
    insert_suffix(suffix,
                  family_name,
                  family.name_id_4_len,
                  family.name_id_4_str);
    insert_suffix(suffix,
                  family_name,
                  family.name_id_16_len,
                  family.name_id_16_str);
    insert_suffix(suffix,
                  family_name,
                  family.name_id_21_len,
                  family.name_id_21_str);

    size_t family_name_len = sdslen(family_name);
    if (is_wide)
      family_name_len &= ~1U; // ensure even value for the loop below

    // set sds to estimated size;
    // we later set the size to the actual value
    family_ps_name = sdsMakeRoomFor(family_ps_name,
                                    family_name_len);

    // construct `family_ps_name' by removing all space characters
    char *fn = family_name;
    char *fpn = family_ps_name;
    if (is_wide)
    {
      for (size_t i = 0; i < family_name_len; i += 2)
      {
        char c1 = *(fn++);
        char c2 = *(fn++);
        if (!(c1 == '\0' && c2 == ' '))
        {
          *(fpn++) = c1;
          *(fpn++) = c2;
        }
      }
    }
    else
    {
      for (size_t i = 0; i < family_name_len; i++)
      {
        char c = *(fn++);
        if (c != ' ')
          *(fpn++) = c;
      }
    }
    // set correct size
    sdsIncrLen(family_ps_name, (int)(fpn - family_ps_name));

    sds ps_suffix = is_wide ? family_ps_suffix_wide : family_ps_suffix;
    insert_suffix(ps_suffix,
                  family_ps_name,
                  family.name_id_6_len,
                  family.name_id_6_str);
  }

  sdsfree(family_suffix);
  sdsfree(family_suffix_wide);
  sdsfree(family_ps_suffix);
  sdsfree(family_ps_suffix_wide);
  sdsfree(family_name);
  sdsfree(family_ps_name);

  family_data_free(idata);

  return 0;
}

} // extern "C"

// end of info.cpp
