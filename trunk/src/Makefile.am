INCLUDES = @EWL_CFLAGS@ @PNG_CFLAGS@
AM_CFLAGS=-Wall _LINUX LINUX

bin_PROGRAMS = cr3

cr3_SOURCES = cr3main.cpp
cr3_LDADD = @EWL_LIBS@ @PNG_LIBS@ -L../crengine/src -lcrengine -lfreetype -ljpeg -lz
cr3_CPPFLAGS = -I../crengine/include -DLINUX -D_LINUX
