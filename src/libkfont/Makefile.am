AM_CPPFLAGS = \
	-I$(srcdir) -I$(builddir) -I$(srcdir)/.. -I$(builddir)/.. \
	-I$(srcdir)/../libkbdfile \
	-DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\" \
	-U_GNU_SOURCE -D_FORTIFY_SOURCE=2 \
	-funit-at-a-time


libkfont_la_SOURCES = \
	error.c \
	font-combined.c font-getters.c font-legacy.c font-load.c font-psf.c \
	unimap.c \
	../xmalloc.c ../xmalloc.h

libkfont_la_LIBADD = $(builddir)/../libkbdfile/libkbdfile.la

kfont_font_inspect_SOURCES = \
	tools/kfont-font-inspect.c \
	../version.h

kfont_font_render_glyphs_SOURCES = \
	tools/kfont-font-render-glyphs.c \
	../version.h

kfont_font_render_text_SOURCES = \
	tools/kfont-font-render-text.c \
	../version.h

kfont_unimap_inspect_SOURCES = \
	tools/kfont-unimap-inspect.c \
	../version.h

kfont_setfont_SOURCES = \
	tools/kfont-setfont.c \
	../version.h

kfont_font_inspect_LDADD = libkfont.la
kfont_font_render_glyphs_LDADD = libkfont.la
kfont_font_render_text_LDADD = libkfont.la
kfont_unimap_inspect_LDADD = libkfont.la
kfont_setfont_LDADD = libkfont.la
noinst_PROGRAMS = kfont-font-inspect kfont-font-render-glyphs kfont-font-render-text kfont-unimap-inspect kfont-setfont

if BUILD_LIBKFONT
lib_LTLIBRARIES = libkfont.la
libkfont_la_LDFLAGS = -export-symbols-regex ^kfont_.*
else
noinst_LTLIBRARIES = libkfont.la
endif

AM_TESTS_ENVIRONMENT = cd ./test-data/; ./generate-fonts.sh; cd ..;
TESTS = ./test-data/test.sh
