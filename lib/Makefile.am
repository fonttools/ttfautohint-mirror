## Makefile.am

# Copyright (C) 2011-2019 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

ABI_CURRENT = 1
ABI_REVISION = 2
ABI_AGE = 0


AM_CPPFLAGS = -I$(top_builddir)/gnulib/src \
              -I$(top_srcdir)/gnulib/src \
              $(FREETYPE_CPPFLAGS) \
              $(HARFBUZZ_CFLAGS)

noinst_LTLIBRARIES = \
  libsds.la \
  libnumberset.la

lib_LTLIBRARIES = libttfautohint.la

include_HEADERS = \
  ttfautohint-errors.h \
  ttfautohint-scripts.h \
  ttfautohint-coverages.h

nodist_include_HEADERS = \
  ttfautohint.h

libttfautohint_la_LDFLAGS = \
  -no-undefined \
  -version-info $(ABI_CURRENT):$(ABI_REVISION):$(ABI_AGE) \
  -export-symbols-regex "TTF_autohint"

libsds_la_SOURCES = \
  sds-wrapper.c sds.h

libnumberset_la_SOURCES = \
  numberset.c numberset.h

# We have to bypass automake's default handling of flex (.l) and bison (.y)
# files, since such files are always treated as traditional lex and yacc
# files, not allowing for flex and bison extensions.  For this reason, we
# call our source files `tacontrol.flex' and `tacontrol.bison' and write
# explicit dependency rules.

libttfautohint_la_SOURCES = \
  llrb.h \
  ta.h \
  tablue.c tablue.h \
  tabytecode.c tabytecode.h \
  tacontrol.c tacontrol.h \
  tacontrol-flex.c tacontrol-flex.h \
  tacontrol-bison.c tacontrol-bison.h \
  tacvt.c \
  tadsig.c \
  tadummy.c tadummy.h \
  tadump.c \
  taerror.c \
  tafeature.c \
  tafile.c \
  tafont.c \
  tafpgm.c \
  tagasp.c \
  tagloadr.c tagloadr.h \
  taglobal.c taglobal.h \
  taglyf.c \
  tagpos.c \
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
  tashaper.c tashaper.h \
  tasort.c tasort.h \
  tastyles.h \
  tatables.c tatables.h \
  tatime.c \
  tattc.c \
  tattf.c \
  tattfa.c \
  tatypes.h \
  taversion.c \
  tawrtsys.h \
  ttfautohint.c

libttfautohint_la_LIBADD = \
  $(noinst_LTLIBRARIES) \
  $(top_builddir)/gnulib/src/libgnu.la \
  $(LIBM) \
  $(FREETYPE_LIBS) \
  $(HARFBUZZ_LIBS)

BUILT_SOURCES = \
  tablue.c tablue.h \
  tacontrol-flex.c tacontrol-flex.h \
  tacontrol-bison.c tacontrol-bison.h \
  ttfautohint.h

EXTRA_DIST = \
  afblue.pl \
  sds.c \
  tablue.cin tablue.hin \
  tablue.dat \
  tacontrol.flex tacontrol.bison \
  ttfautohint.pc.in \
  numberset-test.c \
  ttfautohint.h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ttfautohint.pc

CLEANFILES = $(pkgconfig_DATA)

ttfautohint.pc: ttfautohint.pc.in $(top_builddir)/config.status
	$(AM_V_GEN)$(SED) \
	  -e 's@%prefix%@$(prefix)@g' \
	  -e 's@%exec_prefix%@$(exec_prefix)@g' \
	  -e 's@%includedir%@$(includedir)@g' \
	  -e 's@%libdir%@$(libdir)@g' \
	  -e 's@%version%@$(ABI_CURRENT).$(ABI_REVISION).$(ABI_AGE)@g' \
	  "$<" \
	  > "$@" \
	|| (rm "$@"; false)

tablue.c: tablue.dat tablue.cin
	$(AM_V_GEN)rm -f $@-t $@ \
	&& perl $(srcdir)/afblue.pl $(srcdir)/tablue.dat \
	                            < $(srcdir)/tablue.cin \
	                            > $@-t \
	&& mv $@-t $@

tablue.h: tablue.dat tablue.hin
	$(AM_V_GEN)rm -f $@-t $@ \
	&& perl $(srcdir)/afblue.pl $(srcdir)/tablue.dat \
	                            < $(srcdir)/tablue.hin \
	                            > $@-t \
	&& mv $@-t $@

ttfautohint.h: ttfautohint.h.in
	$(AM_V_GEN)$(SED) \
	  -e 's@%TTFAUTOHINT_MAJOR%@$(ttfautohint_major)@g' \
	  -e 's@%TTFAUTOHINT_MINOR%@$(ttfautohint_minor)@g' \
	  -e 's@%TTFAUTOHINT_REVISION%@$(ttfautohint_revision)@g' \
	  -e 's@%TTFAUTOHINT_VERSION%@$(VERSION)@g' \
	  "$<" \
	  > "$@" \
	|| (rm "$@"; false)

TA_V_FLEX = $(TA_V_FLEX_@AM_V@)
TA_V_FLEX_ = $(TA_V_FLEX_@AM_DEFAULT_V@)
TA_V_FLEX_0 = @echo "  FLEX    " $@;

# we use `touch' to make the created .h file newer than the created .c file

tacontrol-flex.c tacontrol-flex.h: tacontrol.flex
	$(TA_V_FLEX)$(FLEX) $(srcdir)/tacontrol.flex \
	&& touch tacontrol-flex.h
tacontrol-flex.h: tacontrol-flex.c

TA_V_BISON = $(TA_V_BISON_@AM_V@)
TA_V_BISON_ = $(TA_V_BISON_@AM_DEFAULT_V@)
TA_V_BISON_0 = @echo "  BISON   " $@;

tacontrol-bison.c tacontrol-bison.h: tacontrol.bison
	$(TA_V_BISON)$(BISON) $(srcdir)/tacontrol.bison \
	&& touch tacontrol-bison.h
tacontrol-bison.h: tacontrol-bison.c

## end of Makefile.am
