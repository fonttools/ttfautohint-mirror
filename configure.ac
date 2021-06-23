# configure.ac

# Copyright (C) 2011-2021 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

AC_INIT([ttfautohint],
        [m4_esyscmd(gnulib/git-version-gen VERSION)],
        [freetype-devel@nongnu.org])
AC_CONFIG_AUX_DIR([gnulib])
AC_CONFIG_SRCDIR([lib/ttfautohint.pc.in])

AM_INIT_AUTOMAKE([-Wall -Werror tar-ustar subdir-objects]
                 m4_bmatch(m4_defn([AC_PACKAGE_VERSION]),
                           [-], [gnu],
                           [gnits]))

AC_CONFIG_MACRO_DIRS([gnulib/m4
                      m4])

AM_SILENT_RULES([yes])

dnl Derive version triplet (major, minor, revision) from package version
dnl string.

dnl If `AC_PACKAGE_VERSION' contains the suffix `-dirty', we remove it.  If
dnl it then still contains a dash, we have to remove the last dot and
dnl everything appended to it (this was added by the `git-version-gen'
dnl script to reflect git commits after the last tagged commit, which we
dnl ignore).

dnl No quotes around first argument of `m4_bpatsubst'.
dnl No quotes around `m4_bpatsubst'.
m4_define([ttfa_version], m4_bpatsubst(AC_PACKAGE_VERSION, [-dirty$]))
m4_define([ttfa_version], m4_bpatsubst(ttfa_version, [\.[^.]+-.+$]))

dnl No quotes around first argument of `m4_split'.
dnl No quotes around `m4_split'.
m4_define([ttfa_triplet], m4_split(ttfa_version, [\.]))

dnl No quotes around second argument of `m4_argn'.
dnl No quotes around `m4_argn'.
m4_define([ttfa_major], m4_argn([1], ttfa_triplet))
m4_define([ttfa_minor], m4_argn([2], ttfa_triplet))
m4_define([ttfa_revision], m4_argn([3], ttfa_triplet))

dnl No quotes around first two arguments of `m4_if'.
m4_if(ttfa_revision,
      ,
      [m4_define([ttfa_revision], [0])],
      [])

m4_if(ttfa_version,
      AC_PACKAGE_VERSION,
      [],
      [AC_MSG_WARN([Revision number of version tuplet set to ttfa_revision.])])

ttfautohint_major=ttfa_major
ttfautohint_minor=ttfa_minor
ttfautohint_revision=ttfa_revision

AC_SUBST([ttfautohint_major])
AC_SUBST([ttfautohint_minor])
AC_SUBST([ttfautohint_revision])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_LN_S

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_C_INLINE

gl_EARLY

PKG_PROG_PKG_CONFIG([0.24])

# AM_PROG_AR is new in automake 1.11.2;
# however, MinGW doesn't have it yet (May 2012)
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PATH_PROG([BISON], [bison])
AC_PATH_PROG([FLEX], [flex])

gl_INIT

PKG_CHECK_MODULES([HARFBUZZ], [harfbuzz >= 2.4.0])

AT_WITH_QT([], [], [QTPLUGIN.imageformats = -])
AT_REQUIRE_QT_VERSION([4.6])

if test x"$with_qt" != x"no"; then
  AC_MSG_CHECKING([for QLocale::quoteString])
  AS_VERSION_COMPARE([$QT_VERSION], [4.8],
    [AC_MSG_RESULT(no)],
    [AC_MSG_RESULT(no)],
    [AC_MSG_RESULT(yes)
     AC_DEFINE([HAVE_QT_QUOTESTRING], [1],
       [Define if Qt function QLocale::quoteString is available.])])
fi

AM_CONDITIONAL([USE_QT], [test x"$with_qt" != x"no"])

LT_INIT
LT_LTLIZE_LANG([C])


# We use libtool's convenient check for the math library.
LT_LIB_M
AC_SUBST([LIBM])


AC_ARG_WITH([doc],
            [AS_HELP_STRING([--with-doc],
                            [install documentation @<:@default=yes@:>@])],
            [],
            [with_doc=yes])

AC_ARG_WITH([freetype-config],
            [AS_HELP_STRING([--with-freetype-config=PROG],
                            [use FreeType configuration program PROG])],
            [freetype_config=$withval],
            [freetype_config=yes])

if test "$freetype_config" = "yes"; then
  AC_PATH_TOOL(ft_config,
               freetype-config,
               no)
else
  ft_config="$freetype_config"
fi

if test "$ft_config" = "no"; then
  # freetype-config tool not found or disabled; try pkg-config
  PKG_CHECK_MODULES([FREETYPE], [freetype2 >= 2.4.5])
  if { test -z "$FREETYPE_CFLAGS" || test -z "$FREETYPE_LIBS"; }; then
    AC_MSG_ERROR([FreeType library is missing; see https://www.freetype.org/])
  fi
  FREETYPE_CPPFLAGS="$FREETYPE_CFLAGS"
else
  FREETYPE_CPPFLAGS="`$ft_config --cflags`"
  FREETYPE_LIBS="`$ft_config --libtool`"

  # many platforms no longer install .la files for system libraries
  if test ! -f "$FREETYPE_LIBS"; then
    FREETYPE_LIBS="`$ft_config --libs`"
  fi
fi

AC_SUBST([FREETYPE_CPPFLAGS])
AC_SUBST([FREETYPE_LIBS])


AC_MSG_CHECKING([whether FreeType header files are version 2.4.5 or higher])
old_CPPFLAGS="$CPPFLAGS"
CPPFLAGS=$FREETYPE_CPPFLAGS
AC_PREPROC_IFELSE([AC_LANG_SOURCE([[

#include <ft2build.h>
#include FT_FREETYPE_H
#if (FREETYPE_MAJOR*1000 + FREETYPE_MINOR)*1000 + FREETYPE_PATCH < 2004005
#error Freetype version too low.
#endif

]])],
[AC_MSG_RESULT(yes)
 CPPFLAGS="$old_CPPFLAGS"],
[AC_MSG_ERROR([Need FreeType version 2.4.5 or higher])])


AC_MSG_CHECKING([whether FreeType library is version 2.4.5 or higher])
old_CPPFLAGS="$CPPFLAGS"
CPPFLAGS=$FREETYPE_CPPFLAGS
old_LIBS="$LIBS"
LIBS=$FREETYPE_LIBS
AC_LANG_PUSH([LTLIZED C])
AC_RUN_IFELSE([AC_LANG_SOURCE([[

#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int
main()
{
  FT_Error error;
  FT_Library library;
  FT_Int major, minor, patch;

  error = FT_Init_FreeType(&library);
  if (error)
  {
    printf("(test program reports error code %d)... ", error);
    exit(EXIT_FAILURE);
  }

  FT_Library_Version(library, &major, &minor, &patch);

  printf("(found %d.%d.%d)... ", major, minor, patch);

  if (((major*1000 + minor)*1000 + patch) >= 2004005)
    exit(EXIT_SUCCESS);
  exit(EXIT_FAILURE);
}

]])],
[AC_MSG_RESULT(yes)
 CPPFLAGS="$old_CPPFLAGS"
 LIBS="$old_LIBS"],
[AC_MSG_ERROR([Need FreeType version 2.4.5 or higher])],
[AC_MSG_RESULT([skipped due to cross-compilation])])
AC_LANG_POP


if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

# The documentation is part of the distributed bundle.  In the following,
# tests for the documentation building tools are made fatal in case those
# files are missing (which can happen during bootstrap).

AC_DEFUN([TA_DOC],
  [if test -f "$1"; then
     AC_MSG_WARN([$2])
     with_doc=no
   else
     AC_MSG_ERROR([$2])
   fi])

image_file=$srcdir/doc/img/ttfautohintGUI.png
html_file=$srcdir/doc/ttfautohint.html
pdf_file=$srcdir/doc/ttfautohint.pdf

# We use the Noto font family within the PDF documentation file, since it
# has the best Unicode coverage of all freely available fonts.
#
#   https://github.com/googlei18n/noto-fonts/
#
# To be more precise, we don't directly access the git repository; instead,
# we simply test the presence of a typical font file (using the `kpsewhich'
# program as provided by TeXLive and other TeX distributions); the user
# should set the TTFONTS environment variable to add the corresponding
# directory to the TrueType font search path.
#
# Example:
#
#   TTFONTS="/path/to/git/noto-fonts/unhinted/ttf//;"
#
# (note the trailing `;').

noto_font_file=NotoSerifYezidi-Regular.ttf


if test x"$with_doc" != x"no"; then
  # snapshot image creation
  if test x"$DISPLAY" == x; then
    TA_DOC([$image_file],
           [Need X11 to create snapshot image of ttfautohintGUI])
  else
    AC_CHECK_PROG([IMPORT], [import], [import], [no])
    if test x"$IMPORT" == x"no"; then
      TA_DOC([$image_file],
             [Need ImageMagick to create snapshot image of ttfautohintGUI])
    fi
  fi

  # conversion of SVG to PDF
  AC_CHECK_PROG([INKSCAPE], [inkscape], [inkscape], [no])
  if test x"$INKSCAPE" == x"no"; then
    TA_DOC([$pdf_file],
           [Need inkscape to convert SVG image files to PDF])
  fi

  # documentation creation
  AC_CHECK_PROG([PANDOC], [pandoc], [pandoc], [no])
  if test x"$PANDOC" == x"no"; then
    TA_DOC([$html_file],
           [Need pandoc to create PDF and HTML documentation files])
  fi

  # PDF documentation
  # To support Devanagari and other Indic scripts properly,
  # we currently can use XeTeX only.
  AC_CHECK_PROGS([LATEX], [xelatex], [no])
  if test x"$LATEX" == x"no"; then
    TA_DOC([$pdf_file],
           [Need xelatex to create documentation in PDF format])
  else
    AC_CHECK_PROGS([KPSEWHICH], [kpsewhich], [no])
    if test x"$KPSEWHICH" == x"no"; then
      AC_MSG_WARN([Can't find `kpsewhich' to check presence of Noto font files automatically.])
      TA_DOC([$pdf_file],
             [Need kpsewhich to find fonts for documentation in PDF format.])
    else
      AC_MSG_CHECKING([for font $noto_font_file])
      $KPSEWHICH $noto_font_file &> /dev/null
      if test $? -eq 0; then
        AC_MSG_RESULT([yes])
      else
        AC_MSG_RESULT([no])
        AC_MSG_WARN([Can't find `$noto_font_file'.])
        TA_DOC([$pdf_file],
               [Adjust the `TTFONTS' environment variable so that the Noto fonts are found.])
      fi
    fi
  fi
fi

AM_CONDITIONAL([WITH_DOC], [test x"$with_doc" != x"no"])

# Let autoconf create the `img' subdirectory, if necessary.
# `mkdir' is just a tag.
AC_CONFIG_COMMANDS([doc/img/mkdir], [true])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
                 frontend/static-plugins.cpp])
AC_OUTPUT

# end of configure.ac
