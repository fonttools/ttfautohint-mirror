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

# We have to bypass automake's default handling of flex (.l) and bison (.y)
# files, since such files are always treated as traditional lex and yacc
# files, not allowing for flex and bison extensions.  For this reason, we
# call our source files `tadeltas.flex' and `tadeltas.bison' and write
# explicit dependency rules.

libttfautohint_la_SOURCES = \
  llrb.h \
  ta.h \
  tablue.c tablue.h \
  tabytecode.c tabytecode.h \
  tacover.h \
  tacvt.c \
  tadeltas.c tadeltas.h \
  tadeltas-flex-wrapper.c tadeltas-flex.h \
  tadeltas-bison.c tadeltas-bison.h \
  tadsig.c \
  tadummy.c tadummy.h \
  tadump.c \
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
  tascript.c \
  tasfnt.c \
  tasort.c tasort.h \
  tastyles.h \
  tatables.c tatables.h \
  tatime.c \
  tattc.c \
  tattf.c \
  tattfa.c \
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
  tablue.c tablue.h \
  tadeltas-flex.c tadeltas-flex.h \
  tadeltas-bison.c tadeltas-bison.h

EXTRA_DIST = \
  afblue.pl \
  tablue.cin tablue.hin \
  tablue.dat \
  tadeltas.flex tadeltas.bison tadeltas-flex.c

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

TA_V_FLEX = $(TA_V_FLEX_@AM_V@)
TA_V_FLEX_ = $(TA_V_FLEX_@AM_DEFAULT_V@)
TA_V_FLEX_0 = @echo "  FLEX    " $@;

# we use `touch' to make the created .h file newer than the created .c file

tadeltas-flex.c tadeltas-flex.h: tadeltas.flex
	$(TA_V_FLEX)$(FLEX) tadeltas.flex \
	&& touch tadeltas-flex.h
tadeltas-flex.h: tadeltas-flex.c

TA_V_BISON = $(TA_V_BISON_@AM_V@)
TA_V_BISON_ = $(TA_V_BISON_@AM_DEFAULT_V@)
TA_V_BISON_0 = @echo "  BISON   " $@;

tadeltas-bison.c tadeltas-bison.h: tadeltas.bison
	$(TA_V_BISON)$(BISON) tadeltas.bison \
	&& touch tadeltas-bison.h
tadeltas-bison.h: tadeltas-bison.c

## end of Makefile.am
