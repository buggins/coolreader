
METASOURCES = AUTO
#if WITH_JINKE
noinst_LTLIBRARIES = libcrengine.la
#else
#lib_LTLIBRARIES = libcrengine.la
#endif

libcrengine_la_SOURCES = cp_stats.cpp crtxtenc.cpp hist.cpp hyphman.cpp \
		lstridmap.cpp lvbmpbuf.cpp lvdocview.cpp lvdrawbuf.cpp lvfnt.cpp lvfntman.cpp \
		lvimg.cpp lvmemman.cpp lvpagesplitter.cpp lvrend.cpp lvstream.cpp lvstring.cpp \
		lvstsheet.cpp lvstyles.cpp lvtextfm.cpp lvtinydom.cpp lvxml.cpp props.cpp rtfimp.cpp \
		wolutil.cpp crgui.cpp crskin.cpp cri18n.cpp
AM_CXXFLAGS = @FREETYPE_CFLAGS@ @PNG_CFLAGS@ -Wall
AM_CFLAGS = @FREETYPE_CFLAGS@ @PNG_CFLAGS@
