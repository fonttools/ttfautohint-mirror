// info.h

// Copyright (C) 2012-2019 by Werner Lemberg.
//
// This file is part of the ttfautohint library, and may only be used,
// modified, and distributed under the terms given in `COPYING'.  By
// continuing to use, modify, or distribute this file you indicate that you
// have read `COPYING' and understand and accept it fully.
//
// The file `COPYING' mentioned in the previous paragraph is distributed
// with the ttfautohint library.


#ifndef INFO_H_
#define INFO_H_

#include <ttfautohint.h>
#include <stdbool.h> // for llrb.h
#include <llrb.h>

extern "C" {

typedef struct Info_Data_
{
  bool no_info;
  bool detailed_info;
  unsigned char* info_string;
  unsigned char* info_string_wide;
  unsigned short info_string_len;
  unsigned short info_string_wide_len;

  const char* family_suffix;
  void* family_data_head; // a red-black tree

  int hinting_range_min;
  int hinting_range_max;
  int hinting_limit;

  int gray_stem_width_mode;
  int gdi_cleartype_stem_width_mode;
  int dw_cleartype_stem_width_mode;

  int increase_x_height;
  const char* x_height_snapping_exceptions_string;
  int fallback_stem_width;

  bool windows_compatibility;
  bool adjust_subglyphs;
  bool hint_composites;
  char default_script[5];
  char fallback_script[5];
  bool fallback_scaling;
  bool symbol;
  bool dehint;
  bool TTFA_info;

  const char* control_name;
  const char* reference_name;
  int reference_index;
} Info_Data;


char*
check_family_suffix(const char* s);

int
build_version_string(Info_Data* idata);

int
info(unsigned short platform_id,
     unsigned short encoding_id,
     unsigned short language_id,
     unsigned short name_id,
     unsigned short* str_len,
     unsigned char** str,
     void* user);

int
info_post(void* user);

} // extern "C"

#endif // INFO_H_

// end of info.h
