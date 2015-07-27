/* tafeature.c */

/*
 * Copyright (C) 2015 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */

#include "taharfbuzz.h"


#undef COVERAGE
#define COVERAGE(name, NAME, description, \
                 tag1, tag2, tag3, tag4) \
                   HB_TAG(tag1, tag2, tag3, tag4),

const hb_tag_t feature_tags[] =
{

#include <tacover.h>

  HB_TAG('d', 'f', 'l', 't')
};

size_t feature_tags_size = sizeof (feature_tags) / sizeof (feature_tags[0]);

/* end of tafeature.c */
