/* tawrtsys.h */

/*
 * Copyright (C) 2013-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */


/* originally file `afwrtsys.h' (2013-Aug-05) from FreeType */


#ifndef TAWRTSYS_H_
#define TAWRTSYS_H_

  /* Since preprocessor directives can't create other preprocessor */
  /* directives, we have to include the header files manually.     */

#include "tadummy.h"
#include "talatin.h"
#if 0
#  include "tacjk.h"
#  include "taindic.h"
#endif
#ifdef FT_OPTION_AUTOFIT2
#  include "talatin2.h"
#endif

#endif /* TAWRTSYS_H_ */


/* The following part can be included multiple times. */
/* Define `WRITING_SYSTEM' as needed.                 */


/* Add new writing systems here.  The arguments are the writing system */
/* name in lowercase and uppercase, respectively.                      */

WRITING_SYSTEM(dummy, DUMMY)
WRITING_SYSTEM(latin, LATIN)
#if 0
WRITING_SYSTEM(cjk, CJK)
WRITING_SYSTEM(indic, INDIC)
#endif
#ifdef FT_OPTION_AUTOFIT2
WRITING_SYSTEM(latin2, LATIN2)
#endif

/* end of tawrtsys.h */
