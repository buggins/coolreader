# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = @SUBDIRS@

# src/ must be last, because src/cr3 must be linked with plugins

#CRENGINE_CFLAGS="-I$(abs_top_srcdir)/crengine/include -DLINUX -D_LINUX -DCOLOR_BACKBUFFER=0 "
#CRENGINE_LIBS="-L$(abs_top_srcdir)/crengine/src -lcrengine "

pkgdatadir = $(prefix)/share/crengine

skindir = $(pkgdatadir)/skin
hyphdir = $(pkgdatadir)/hyph
keymapdir = $(sysconfdir)/cr3

dist_skin_DATA = skin/cr3skin.xml \
    skin/cr3_main_menu_logo.png  skin/cr3_menu_shortcut.png skin/scroll_arrow_down_disabled.png skin/scroll_arrow_right_disabled.png skin/scroll_body_horizontal.png \
    skin/cr3_main_menu.png skin/cr3_menu_shortcut_selected.png skin/scroll_arrow_down_enabled.png skin/scroll_arrow_right_enabled.png skin/scroll_body_vertical.png \
    skin/cr3_menu_item.png skin/cr3_menu_title.png skin/scroll_arrow_left_disabled.png skin/scroll_arrow_up_disabled.png skin/scroll_slider_horizontal.png \
    skin/cr3_menu_item_selected.png skin/cr3_settings_menu.png skin/scroll_arrow_left_enabled.png skin/scroll_arrow_up_enabled.png skin/scroll_slider_vertical.png \
    skin/cr3_t9_frame.png skin/cr3_t9_input.png skin/cr3_t9_shortcut.png

dist_pkgdata_DATA = src/fb2.css src/cr3_def_cover.png
dist_hyph_DATA = hyph/Russian_EnUS_hyphen_(Alan).pdb
dist_keymap_DATA = keymaps/keydefs.ini keymaps/keymaps.ini keymaps/kblayout.ini

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
