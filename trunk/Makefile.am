# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = crengine

# src/ must be last, because src/cr3 must be linked with plugins
SUBDIRS += src

#CRENGINE_CFLAGS="-I$(abs_top_srcdir)/crengine/include -DLINUX -D_LINUX -DCOLOR_BACKBUFFER=0 "
#CRENGINE_LIBS="-L$(abs_top_srcdir)/crengine/src -lcrengine "

pkgdatadir = $(prefix)/share/crengine

skindir = $(pkgdatadir)/skin
hyphdir = $(pkgdatadir)/hyph
dist_skin_DATA = skin/cr3skin.xml \
    skin/cr3_main_menu_logo.png  skin/cr3_main_menu.png  skin/cr3_menu_item.png skin/cr3_menu_shortcut.png  skin/cr3_menu_title.png  skin/cr3_settings_menu.png
dist_pkgdata_DATA = src/fb2.css src/cr3_def_cover.png
dist_hyph_DATA = hyph/Russian_EnUS_hyphen_(Alan).pdb

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
