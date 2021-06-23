## Makefile.am

# due to a limitation of `autoreconf',
# all -I directives currently must be set on a single line
ACLOCAL_AMFLAGS = -I gnulib/m4 -I m4

SUBDIRS = . \
          doc
EXTRA_DIST = bootstrap \
             bootstrap.conf \
             FTL.TXT \
             gnulib/m4/gnulib-cache.m4 \
             GPLv2.TXT \
             README \
             TODO \
             .version

BUILT_SOURCES = .version
.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/VERSION.TXT

noinst_LTLIBRARIES =
MOSTLYCLEANFILES =
CLEANFILES =
DISTCLEANFILES =
MOSTLYCLEANDIRS =

AM_CPPFLAGS = -I$(top_builddir)/lib \
              -I$(top_srcdir)/lib \
              -I$(top_builddir)/gnulib/src \
              -I$(top_srcdir)/gnulib/src

include $(top_srcdir)/gnulib/src/gnulib.mk
include $(top_srcdir)/lib/local.mk
include $(top_srcdir)/frontend/local.mk

## end of Makefile.am
