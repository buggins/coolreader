#CRENGINE_FLAGS = ${CRENGINE_CFLAGS}
#CRENGINE_LIBS = ${CRENGINE_LIBS}

INCLUDES = @FREETYPE_CFLAGS@ @PNG_CFLAGS@ @CRENGINE_CFLAGS@ -I../crengine/include
AM_CFLAGS=-Wall
AM_CPPFLAGS = -I$(top_srcdir)/crengine/include

bin_PROGRAMS = cr3

cr3_SOURCES = cr3main.cpp
cr3_LDADD = ../crengine/src/libcrengine.la @FREETYPE_LIBS@ \
	@XCB_KEYSYMS_LIBS@ @XCB_LIBS@ @XCB_AUX_LIBS@ @XCB_IMAGE_LIBS@ @PNG_LIBS@ @CRENGINE_LIBS@ \
	-lfreetype -ljpeg -lz
cr3_CPPFLAGS = @CRENGINE_CFLAGS@



