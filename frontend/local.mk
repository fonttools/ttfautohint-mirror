# frontend/local.mk

# Copyright (C) 2011-2021 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

SUFFIXES = .moc.cpp .h

# Make call to `moc' emit just `MOC'.
moc_verbose = $(moc_verbose_@AM_V@)
moc_verbose_ = $(moc_verbose_@AM_DEFAULT_V@)
moc_verbose_0 = @echo "  MOC     " $@;

# moc from Qt5 aborts if unknown command line options are supplied;
# in particular, it doesn't recognize `-isystem'.
#
# We must also ensure that `config.h' gets included first.
.h.moc.cpp:
	$(moc_verbose){ \
	  echo '#include <config.h>'; \
	  $(MOC) \
	    `echo $(QT_CPPFLAGS) | sed 's/-isystem/-I/g'` \
	    $(EXTRA_CPPFLAGS) \
	    $< ; \
	} > $@.tmp
	@mv $@.tmp $@

DISTCLEANFILES += $(BUILT_SOURCES)

LDADD = lib/libttfautohint.la \
        lib/libsds.la \
        lib/libnumberset.la \
        gnulib/src/libgnu.la \
        $(LTLIBINTL) \
        $(LTLIBTHREAD) \
        $(FREETYPE_LIBS)

bin_PROGRAMS = frontend/ttfautohint

frontend_ttfautohint_SOURCES = \
  frontend/info.cpp \
  frontend/info.h \
  frontend/main.cpp
frontend_ttfautohint_CPPFLAGS = $(AM_CPPFLAGS) \
                                $(FREETYPE_CPPFLAGS)
frontend_ttfautohint_LDADD = $(LDADD)

manpages = frontend/ttfautohint.1

if USE_QT
  bin_PROGRAMS += frontend/ttfautohintGUI

  frontend_ttfautohintGUI_SOURCES = \
    frontend/ddlineedit.cpp \
    frontend/ddlineedit.h \
    frontend/info.cpp \
    frontend/info.h \
    frontend/main.cpp \
    frontend/maingui.cpp \
    frontend/maingui.h \
    frontend/ttlineedit.cpp \
    frontend/ttlineedit.h
  nodist_frontend_ttfautohintGUI_SOURCES = \
    frontend/ddlineedit.moc.cpp \
    frontend/maingui.moc.cpp \
    frontend/static-plugins.cpp \
    frontend/ttlineedit.moc.cpp

  # We want compatibility with Qt < 5.11
  frontend_ttfautohintGUI_CXXFLAGS = $(QT_CXXFLAGS) \
                                     -Wno-deprecated-declarations
  frontend_ttfautohintGUI_LDFLAGS = $(QT_LDFLAGS)
  frontend_ttfautohintGUI_CPPFLAGS = $(AM_CPPFLAGS) \
                                     $(FREETYPE_CPPFLAGS) \
                                     $(QT_CPPFLAGS) \
                                     -DBUILD_GUI
  frontend_ttfautohintGUI_LDADD = $(LDADD) \
                                  $(QT_LIBS)

  BUILT_SOURCES += frontend/ddlineedit.moc.cpp \
                   frontend/maingui.moc.cpp \
                   frontend/static-plugins.cpp \
                   frontend/ttlineedit.moc.cpp

  manpages += frontend/ttfautohintGUI.1
endif

if WITH_DOC
  dist_man_MANS = $(manpages)
endif

# `ttfautohint.h' holds default values for some options,
# `ttfautohint-scripts.' the list of available scripts
frontend/ttfautohint.1: frontend/ttfautohint$(EXEEXT)
	$(HELP2MAN) --output=$@ \
	            --no-info \
	            --name="add new, auto-generated hints to a TrueType font" \
	            $<

frontend/ttfautohintGUI.1: frontend/ttfautohintGUI$(EXEEXT)
	$(HELP2MAN) --output=$@ \
	            --no-info \
	            --name="add new, auto-generated hints to a TrueType font" \
	            --help-option=--help-all \
	            $<

# end of Makefile.am
