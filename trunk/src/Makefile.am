#CRENGINE_FLAGS = ${CRENGINE_CFLAGS}
#CRENGINE_LIBS = ${CRENGINE_LIBS}

INCLUDES = @FREETYPE_CFLAGS@ @PNG_CFLAGS@ @CRENGINE_CFLAGS@ -I$(top_srcdir)/crengine/include
AM_CFLAGS=-Wall
AM_CXXFLAGS=-Wall
AM_CPPFLAGS = -I$(top_srcdir)/crengine/include

COMMON_LDADD = @FREETYPE_LIBS@ \
	@XCB_KEYSYMS_LIBS@ \
	@XCB_LIBS@ @XCB_AUX_LIBS@ @XCB_IMAGE_LIBS@ @PNG_LIBS@ @CRENGINE_LIBS@ @TINYDICT_LIBS@ \
	@FONTCONFIG_LIBS@ \
	-lfreetype -ljpeg -lpng -lz
AM_CPPFLAGS += @CRENGINE_CFLAGS@ @TINYDICT_CFLAGS@ @FONTCONFIG_CFLAGS@

AM_CPPFLAGS += -DCRSKIN=\"$(pkgdatadir)\"

COMMON_SOURCES = cr3main.cpp settings.cpp mainwnd.cpp cr3_def_cover.cpp bgfit.cpp

#if WITH_DICT
COMMON_SOURCES += dictdlg.cpp
#endif

noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = ${COMMON_SOURCES}

if WITH_JINKE
lib_LTLIBRARIES = libfb2.la
libfb2_la_SOURCES = cr3jinke.cpp
libfb2_la_LIBADD =  libcommon.la ${COMMON_LDADD}
libfb2_la_LDFLAGS = --static-libtool-libs -rpath ${libdir} 
else
#if WITH_DICT
#COMMON_LDADD += $(top_builddir)/libdictd/libdictd.la
#COMMON_LDADD += $(top_builddir)/libdictd/libmaa/libmaa.la
#endif
AM_CPPFLAGS += -DWITH_XCB
bin_PROGRAMS = cr3
cr3_SOURCES = cr3xcb.cpp
cr3_LDADD = libcommon.la ${COMMON_LDADD}
endif


