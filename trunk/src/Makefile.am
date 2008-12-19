#CRENGINE_FLAGS = ${CRENGINE_CFLAGS}
#CRENGINE_LIBS = ${CRENGINE_LIBS}

INCLUDES = @FREETYPE_CFLAGS@ @PNG_CFLAGS@ @CRENGINE_CFLAGS@ -I$(top_srcdir)/crengine/include
AM_CFLAGS=-Wall
AM_CPPFLAGS = -I$(top_srcdir)/crengine/include

if WITH_DICT
INCLUDES += -I $(top_srcdir)/libdictd
endif

CPPFLAGS += -DCRSKIN=\"$(pkgdatadir)\"

bin_PROGRAMS = cr3

cr3_SOURCES = cr3main.cpp settings.cpp mainwnd.cpp cr3xcb.cpp
if WITH_DICT
cr3_SOURCES += dictdlg.cpp
endif
cr3_LDADD = @FREETYPE_LIBS@ \
	@XCB_KEYSYMS_LIBS@ \
	@XCB_LIBS@ @XCB_AUX_LIBS@ @XCB_IMAGE_LIBS@ @PNG_LIBS@ @CRENGINE_LIBS@ \
	-lfreetype -ljpeg -lz
cr3_CPPFLAGS = @CRENGINE_CFLAGS@
if WITH_DICT
cr3_LDADD += $(top_builddir)/libdictd/libdictd.la
cr3_LDADD += $(top_builddir)/libdictd/libmaa/libmaa.la
endif


