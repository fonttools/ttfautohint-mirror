# Makefile.am

# Copyright (C) 2011-2019 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

DOCSRC = ttfautohint-1.pandoc \
         ttfautohint-2.pandoc \
         ttfautohint-3.pandoc \
         ttfautohint-4.pandoc \
         ../NEWS

DOCIMGSVG = img/blue-zones.svg \
            img/glyph-terms.svg \
            img/o-and-i.svg \
            img/segment-edge.svg

DOCIMGPDF = img/blue-zones.pdf \
            img/glyph-terms.pdf \
            img/o-and-i.pdf \
            img/segment-edge.pdf

DOCIMGPNG = img/ttfautohintGUI.png \
            img/a-before-hinting.png \
            img/a-after-hinting.png \
            img/a-after-autohinting.png \
            img/afii10108-11px-after-hinting.png \
            img/afii10108-11px-before-hinting.png \
            img/afii10108-12px-after-hinting.png \
            img/afii10108-12px-before-hinting.png \
            img/composite-no-round-xy-to-grid-option-c.png \
            img/composite-no-round-xy-to-grid.png \
            img/composite-round-xy-to-grid.png \
            img/e-17px-x14.png \
            img/e-17px-x17.png \
            img/fira-16px-ie11-win81.png \
            img/Halant-Regular-O-Q.png \
            img/Halant-Regular-O-Q-unhinted-12px.png \
            img/Halant-Regular-O-good-Q-badly-hinted-12px.png \
            img/Halant-Regular-O-good-Q-better-hinted-12px.png \
            img/Halant-Regular-O-good-Q-well-hinted-12px.png \
            img/Merriweather-Black-g-21px-comparison.png

DOC = ttfautohint.html \
      ttfautohint.pdf \
      ttfautohint.txt \
      $(DOCIMGPNG) \
      $(DOCIMGSVG) \
      $(DOCIMGPDF) \
      footnote-popup.js \
      jquery-1.11.2.min.js \
      toc-unfold.js

EXTRA_DIST = c2pandoc.sed \
             taranges.sed \
             make-snapshot.sh \
             strip-comments.sh \
             ttfautohint-1.pandoc \
             ttfautohint-2.pandoc \
             ttfautohint-3.pandoc \
             ttfautohint-4.pandoc \
             template.html \
             template.tex \
             longtable-patched.sty \
             ttfautohint-css.html \
             ttfautohint-js.html

if WITH_DOC
  nobase_dist_doc_DATA = $(DOC)
endif


ttfautohint-2.pandoc: $(top_builddir)/lib/ttfautohint.h
	$(SED) -f $(srcdir)/c2pandoc.sed < $< > $@

ttfautohint-4.pandoc: $(top_srcdir)/lib/taranges.c
	$(SED) -f $(srcdir)/taranges.sed < $< > $@

ttfautohint.txt: $(DOCSRC)
	$(SHELL) $(srcdir)/strip-comments.sh $^ > $@

if WITH_DOC

  # suffix rules must always start in column 0
.svg.pdf:
	  $(INKSCAPE) --export-pdf=$@ $<

  # build snapshot image of ttfautohintGUI:
  # this needs X11 and ImageMagick's `import' tool
  # (in the `make-snaphshot.sh' script)
  img/ttfautohintGUI.png: $(top_srcdir)/frontend/maingui.cpp \
                          $(top_srcdir)/configure.ac \
                          $(srcdir)/ttfautohintGUI.stylesheet
	  cd $(top_builddir)/frontend \
	    && $(MAKE) $(AM_MAKEFLAGS) ttfautohintGUI$(EXEEXT)
	  $(SHELL) $(srcdir)/make-snapshot.sh \
	             $(top_builddir)/frontend/ttfautohintGUI$(EXEEXT) \
	               --stylesheet=$(srcdir)/ttfautohintGUI.stylesheet \
	               > $@

  ttfautohint.html: ttfautohint.txt $(DOCIMGPNG) $(DOCIMGSVG) \
                    ttfautohint-css.html ttfautohint-js.html \
                    template.html $(top_builddir)/.version
	  $(PANDOC) --smart \
	            --template=$(srcdir)/template.html \
	            --default-image-extension=".svg" \
	            --variable="version:$(VERSION)" \
	            --toc \
	            --include-in-header=$(srcdir)/ttfautohint-css.html \
	            --include-in-header=$(srcdir)/ttfautohint-js.html \
	            --standalone \
	            --output=$@ $<

  ttfautohint.pdf: ttfautohint.txt $(DOCIMGPNG) $(DOCIMGPDF) \
                   template.tex $(top_builddir)/.version
	  TEXINPUTS="$(srcdir);" \
	  $(PANDOC) --smart \
	            --latex-engine=$(LATEX) \
	            --template=$(srcdir)/template.tex \
	            --default-image-extension=".pdf" \
	            --variable="version:$(VERSION)" \
	            --number-sections \
	            --toc \
	            --chapters \
	            --standalone \
	            --output=$@ $<
else
.svg.pdf:
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install inkscape and reconfigure"

  img/ttfautohintGUI.png: $(top_srcdir)/frontend/maingui.cpp \
                          $(top_srcdir)/configure.ac
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install ImageMagick's \`import' tool and reconfigure"

  ttfautohint.html: ttfautohint.txt $(DOCIMGPNG) $(DOCIMGSVG) \
                    ttfautohint-css.html template.html $(top_builddir)/.version
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install pandoc and reconfigure"

  ttfautohint.pdf: $ttfautohint.txt $(DOCIMGPNG) $(DOCIMGPDF) \
                   template.tex $(top_builddir)/.version
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install pdftex and pandoc, then reconfigure"
endif

# end of Makefile.am
