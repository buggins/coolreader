# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = crengine
if WITH_DICT
SUBDIRS += tinydict
endif

# src/ must be last, because src/cr3 must be linked with plugins
SUBDIRS += src

#CRENGINE_CFLAGS="-I$(abs_top_srcdir)/crengine/include -DLINUX -D_LINUX -DCOLOR_BACKBUFFER=0 "
#CRENGINE_LIBS="-L$(abs_top_srcdir)/crengine/src -lcrengine "


MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
config.guess \
config.h.in \
config.sub \
configure \
install-sh \
ltconfig \
ltmain.sh \
missing \
mkinstalldirs \
stamp-h.in \
build-stamp \
configure-stamp \
depcomp 
