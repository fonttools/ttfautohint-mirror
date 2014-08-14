## Makefile.am

# Copyright (C) 2011-2014 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

AM_CPPFLAGS = $(FREETYPE_CPPFLAGS) \
              $(HARFBUZZ_CFLAGS)

noinst_LTLIBRARIES = \
  libttfautohint.la \
  libnumberset.la

libnumberset_la_SOURCES = \
  numberset.c numberset.h

libttfautohint_la_SOURCES = \
  llrb.h \
  ta.h \
  tablue.c tablue.h \
  tabytecode.c tabytecode.h \
  tacover.h \
  tacvt.c \
  tadeltas.c tadeltas.h \
  tadsig.c \
  tadummy.c tadummy.h \
  taerror.c \
  tafile.c \
  tafont.c \
  tafpgm.c \
  tagasp.c \
  tagloadr.c tagloadr.h \
  taglobal.c taglobal.h \
  taglyf.c \
  tagpos.c \
  taharfbuzz.c taharfbuzz.h \
  tahints.c tahints.h \
  tahmtx.c \
  talatin.c talatin.h \
  taloader.c taloader.h \
  taloca.c \
  tamaxp.c \
  taname.c \
  tapost.c \
  taprep.c \
  taranges.c taranges.h \
  tasfnt.c \
  tasort.c tasort.h \
  tastyles.h \
  tatables.c tatables.h \
  tatime.c \
  tattc.c \
  tattf.c \
  tatypes.h \
  tawrtsys.h \
  ttfautohint.c \
  ttfautohint.h ttfautohint-errors.h ttfautohint-scripts.h

libttfautohint_la_LIBADD = \
  libnumberset.la \
  $(LIBM) \
  $(FREETYPE_LIBS) \
  $(HARFBUZZ_LIBS)

BUILT_SOURCES = \
  tablue.c tablue.h

EXTRA_DIST = \
  afblue.pl \
  tablue.cin tablue.hin \
  tablue.dat

tablue.c: tablue.dat tablue.cin
	$(AM_V_GEN)rm -f $@-t $@ \
	&& perl afblue.pl $(srcdir)/tablue.dat \
	                  < $(srcdir)/tablue.cin \
	                  > $@-t \
	&& mv $@-t $@

tablue.h: tablue.dat tablue.hin
	$(AM_V_GEN)rm -f $@-t $@ \
	&& perl afblue.pl $(srcdir)/tablue.dat \
	                  < $(srcdir)/tablue.hin \
	                  > $@-t \
	&& mv $@-t $@

## end of Makefile.am
