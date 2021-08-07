# doc/local.mk

# Copyright (C) 2011-2021 by Werner Lemberg.
#
# This file is part of the ttfautohint library, and may only be used,
# modified, and distributed under the terms given in `COPYING'.  By
# continuing to use, modify, or distribute this file you indicate that you
# have read `COPYING' and understand and accept it fully.
#
# The file `COPYING' mentioned in the previous paragraph is distributed
# with the ttfautohint library.

DOCSRC = \
  doc/ttfautohint-1.pandoc \
  doc/ttfautohint-2.pandoc \
  doc/ttfautohint-3.pandoc \
  doc/ttfautohint-4.pandoc \
  NEWS

DOCIMGSVG = \
  doc/img/blue-zones.svg \
  doc/img/glyph-terms.svg \
  doc/img/o-and-i.svg \
  doc/img/segment-edge.svg

DOCIMGPDF = \
  doc/img/blue-zones.pdf \
  doc/img/glyph-terms.pdf \
  doc/img/o-and-i.pdf \
  doc/img/segment-edge.pdf

DOCIMGPNG = \
  doc/img/ttfautohintGUI.png \
  doc/img/a-before-hinting.png \
  doc/img/a-after-hinting.png \
  doc/img/a-after-autohinting.png \
  doc/img/afii10108-11px-after-hinting.png \
  doc/img/afii10108-11px-before-hinting.png \
  doc/img/afii10108-12px-after-hinting.png \
  doc/img/afii10108-12px-before-hinting.png \
  doc/img/composite-no-round-xy-to-grid-option-c.png \
  doc/img/composite-no-round-xy-to-grid.png \
  doc/img/composite-round-xy-to-grid.png \
  doc/img/e-17px-x14.png \
  doc/img/e-17px-x17.png \
  doc/img/fira-16px-ie11-win81.png \
  doc/img/Halant-Regular-O-Q.png \
  doc/img/Halant-Regular-O-Q-unhinted-12px.png \
  doc/img/Halant-Regular-O-good-Q-badly-hinted-12px.png \
  doc/img/Halant-Regular-O-good-Q-better-hinted-12px.png \
  doc/img/Halant-Regular-O-good-Q-well-hinted-12px.png \
  doc/img/Merriweather-Black-g-21px-comparison.png

DOC = \
  doc/ttfautohint.html \
  doc/ttfautohint.pdf \
  doc/ttfautohint.txt \
  $(DOCIMGPNG) \
  $(DOCIMGSVG) \
  $(DOCIMGPDF) \
  doc/footnote-popup.js \
  doc/jquery-3.6.0.min.js \
  doc/toc-unfold.js

EXTRA_DIST += \
  doc/c2pandoc.sed \
  doc/taranges.sed \
  doc/make-snapshot.sh \
  doc/strip-comments.sh \
  doc/ttfautohint-1.pandoc \
  doc/ttfautohint-2.pandoc \
  doc/ttfautohint-3.pandoc \
  doc/ttfautohint-4.pandoc \
  doc/template.html \
  doc/template.tex \
  doc/longtable-patched.sty \
  doc/fontspec.sty \
  doc/fontspec-xetex.sty \
  doc/ttfautohint-css.html \
  doc/ttfautohint-js.html \
  doc/ttfautohintGUI.stylesheet

if WITH_DOC
  nobase_dist_doc_DATA = $(DOC)
endif


doc/ttfautohint-2.pandoc: lib/ttfautohint.h
	$(SED) -f $(srcdir)/doc/c2pandoc.sed < $< > $@

doc/ttfautohint-4.pandoc: lib/taranges.c
	$(SED) -f $(srcdir)/doc/taranges.sed < $< > $@

doc/ttfautohint.txt: $(DOCSRC)
	$(SHELL) $(srcdir)/doc/strip-comments.sh $^ > $@

if WITH_DOC

  # suffix rules must always start in column 0
.svg.pdf:
	  $(INKSCAPE) --export-pdf=$@ $<

  # build snapshot image of ttfautohintGUI:
  # this needs X11 and ImageMagick's `import' tool
  # (in the `make-snaphshot.sh' script)
  doc/img/ttfautohintGUI.png: frontend/ttfautohintGUI$(EXEEXT) \
                              doc/ttfautohintGUI.stylesheet
	  $(SHELL) $(srcdir)/doc/make-snapshot.sh \
	             frontend/ttfautohintGUI$(EXEEXT) \
	               --stylesheet=$(srcdir)/doc/ttfautohintGUI.stylesheet \
	               > $@

  doc/ttfautohint.html: \
    doc/ttfautohint.txt \
    $(DOCIMGPNG) \
    $(DOCIMGSVG) \
    doc/ttfautohint-css.html \
    doc/ttfautohint-js.html \
    doc/template.html \
    .version
	  $(PANDOC) --from=markdown+smart \
	            --resource-path=$(srcdir)/doc \
	            --resource-path=doc \
	            --template=$(srcdir)/doc/template.html \
	            --default-image-extension=".svg" \
	            --variable="version:$(VERSION)" \
	            --toc \
	            --include-in-header=$(srcdir)/doc/ttfautohint-css.html \
	            --include-in-header=$(srcdir)/doc/ttfautohint-js.html \
	            --standalone \
	            --output=$@ $<

  doc/ttfautohint.pdf: \
    doc/ttfautohint.txt \
    $(DOCIMGPNG) \
    $(DOCIMGPDF) \
    doc/template.tex \
    .version
	  TEXINPUTS="$(srcdir)/doc;" \
	  $(PANDOC) --from=markdown+smart \
	            --resource-path=$(srcdir)/doc \
	            --resource-path=doc \
	            --pdf-engine=$(LATEX) \
	            --template=$(srcdir)/doc/template.tex \
	            --default-image-extension=".pdf" \
	            --variable="version:$(VERSION)" \
	            --number-sections \
	            --toc \
	            --top-level-division=chapter \
	            --standalone \
	            --output=$@ $<
else
.svg.pdf:
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install inkscape and reconfigure"

  doc/img/ttfautohintGUI.png: frontend/ttfautohintGUI$(EXEEXT)
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install ImageMagick's \`import' tool and reconfigure"

  doc/ttfautohint.html: \
    doc/ttfautohint.txt \
    $(DOCIMGPNG) \
    $(DOCIMGSVG) \
    doc/ttfautohint-css.html \
    doc/template.html \
    .version
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install pandoc and reconfigure"

  doc/ttfautohint.pdf: \
    doc/ttfautohint.txt \
    $(DOCIMGPNG) \
    $(DOCIMGPDF) \
    doc/template.tex \
    .version
	  @echo 1>&2 "warning: can't generate \`$@'"
	  @echo 1>&2 "         please install pdftex and pandoc, then reconfigure"
endif

# end of Makefile.am
