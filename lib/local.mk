# lib/local.mk

# Copyright (C) 2011-2021 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

ABI_CURRENT = 1
ABI_REVISION = 3
ABI_AGE = 0


noinst_LTLIBRARIES += \
  lib/libsds.la \
  lib/libnumberset.la

lib_LTLIBRARIES = lib/libttfautohint.la

include_HEADERS = \
  lib/ttfautohint-errors.h \
  lib/ttfautohint-scripts.h \
  lib/ttfautohint-coverages.h

nodist_include_HEADERS = \
  lib/ttfautohint.h
lib_libsds_la_SOURCES = \
  lib/sds-wrapper.c lib/sds.h

lib_libnumberset_la_SOURCES = \
  lib/numberset.c lib/numberset.h

# We have to bypass automake's default handling of flex (.l) and bison (.y)
# files, since such files are always treated as traditional lex and yacc
# files, not allowing for flex and bison extensions.  For this reason, we
# call our source files `tacontrol.flex' and `tacontrol.bison' and write
# explicit dependency rules.

lib_libttfautohint_la_SOURCES = \
  lib/llrb.h \
  lib/ta.h \
  lib/tablue.c lib/tablue.h \
  lib/tabytecode.c lib/tabytecode.h \
  lib/tacontrol.c lib/tacontrol.h \
  lib/tacontrol-flex.c lib/tacontrol-flex.h \
  lib/tacontrol-bison.c lib/tacontrol-bison.h \
  lib/tacvt.c \
  lib/tadsig.c \
  lib/tadummy.c lib/tadummy.h \
  lib/tadump.c \
  lib/taerror.c \
  lib/tafeature.c \
  lib/tafile.c \
  lib/tafont.c \
  lib/tafpgm.c \
  lib/tagasp.c \
  lib/tagloadr.c lib/tagloadr.h \
  lib/taglobal.c lib/taglobal.h \
  lib/taglyf.c \
  lib/tagpos.c \
  lib/tahints.c lib/tahints.h \
  lib/tahmtx.c \
  lib/talatin.c lib/talatin.h \
  lib/taloader.c lib/taloader.h \
  lib/taloca.c \
  lib/tamaxp.c \
  lib/taname.c \
  lib/tapost.c \
  lib/taprep.c \
  lib/taranges.c lib/taranges.h \
  lib/tascript.c \
  lib/tasfnt.c \
  lib/tashaper.c lib/tashaper.h \
  lib/tasort.c lib/tasort.h \
  lib/tastyles.h \
  lib/tatables.c lib/tatables.h \
  lib/tatime.c \
  lib/tattc.c \
  lib/tattf.c \
  lib/tattfa.c \
  lib/tatypes.h \
  lib/taversion.c \
  lib/tawrtsys.h \
  lib/ttfautohint.c

lib_libttfautohint_la_CPPFLAGS = \
  $(AM_CPPFLAGS) \
  $(FREETYPE_CPPFLAGS) \
  $(HARFBUZZ_CPPFLAGS)

lib_libttfautohint_la_LDFLAGS = \
  -no-undefined \
  -version-info $(ABI_CURRENT):$(ABI_REVISION):$(ABI_AGE) \
  -export-symbols-regex "TTF_autohint"

lib_libttfautohint_la_LIBADD = \
  $(noinst_LTLIBRARIES) \
  $(LIBM) \
  $(FREETYPE_LIBS) \
  $(HARFBUZZ_LIBS)

BUILT_SOURCES += \
  lib/tablue.c lib/tablue.h \
  lib/tacontrol-flex.c lib/tacontrol-flex.h \
  lib/tacontrol-bison.c lib/tacontrol-bison.h \
  lib/ttfautohint.h

EXTRA_DIST += \
  lib/afblue.pl \
  lib/sds.c \
  lib/tablue.cin lib/tablue.hin \
  lib/tablue.dat \
  lib/tacontrol.flex lib/tacontrol.bison \
  lib/ttfautohint.pc.in \
  lib/numberset-test.c \
  lib/ttfautohint.h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib/ttfautohint.pc

CLEANFILES += $(pkgconfig_DATA)

lib/ttfautohint.pc: lib/ttfautohint.pc.in config.status
	$(AM_V_GEN)$(SED) \
	  -e 's@%prefix%@$(prefix)@g' \
	  -e 's@%exec_prefix%@$(exec_prefix)@g' \
	  -e 's@%includedir%@$(includedir)@g' \
	  -e 's@%libdir%@$(libdir)@g' \
	  -e 's@%version%@$(ABI_CURRENT).$(ABI_REVISION).$(ABI_AGE)@g' \
	  "$<" \
	  > "$@" \
	|| (rm "$@"; false)

lib/tablue.c: lib/tablue.dat lib/tablue.cin
	$(AM_V_GEN)rm -f $@-t $@ \
	&& perl $(srcdir)/lib/afblue.pl $(srcdir)/lib/tablue.dat \
	                                < $(srcdir)/lib/tablue.cin \
	                                > $@-t \
	&& mv $@-t $@

lib/tablue.h: lib/tablue.dat lib/tablue.hin
	$(AM_V_GEN)rm -f $@-t $@ \
	&& perl $(srcdir)/lib/afblue.pl $(srcdir)/lib/tablue.dat \
	                                < $(srcdir)/lib/tablue.hin \
	                                > $@-t \
	&& mv $@-t $@

lib/ttfautohint.h: lib/ttfautohint.h.in
	$(AM_V_GEN)$(SED) \
	  -e 's@%TTFAUTOHINT_MAJOR%@$(ttfautohint_major)@g' \
	  -e 's@%TTFAUTOHINT_MINOR%@$(ttfautohint_minor)@g' \
	  -e 's@%TTFAUTOHINT_REVISION%@$(ttfautohint_revision)@g' \
	  -e 's@%TTFAUTOHINT_VERSION%@$(VERSION)@g' \
	  "$<" \
	  > "$@" \
	|| (rm "$@"; false)

DISTCLEANFILES += lib/ttfautohint.h

TA_V_FLEX = $(TA_V_FLEX_@AM_V@)
TA_V_FLEX_ = $(TA_V_FLEX_@AM_DEFAULT_V@)
TA_V_FLEX_0 = @echo "  FLEX    " $@;

# We use `touch' to make the created .h file newer than the created .c file.

lib/tacontrol-flex.c lib/tacontrol-flex.h: lib/tacontrol.flex
	$(TA_V_FLEX)$(FLEX) --outfile=lib/tacontrol-flex.c \
	                    --header-file=lib/tacontrol-flex.h \
	                    $< \
	&& touch lib/tacontrol-flex.h
lib/tacontrol-flex.h: lib/tacontrol-flex.c

TA_V_BISON = $(TA_V_BISON_@AM_V@)
TA_V_BISON_ = $(TA_V_BISON_@AM_DEFAULT_V@)
TA_V_BISON_0 = @echo "  BISON   " $@;

lib/tacontrol-bison.c lib/tacontrol-bison.h: lib/tacontrol.bison
	$(TA_V_BISON)$(BISON) --output=lib/tacontrol-bison.c \
	                      --defines=lib/tacontrol-bison.h \
	                      $< \
	&& touch lib/tacontrol-bison.h
lib/tacontrol-bison.h: lib/tacontrol-bison.c

# end of lib/local.mk
