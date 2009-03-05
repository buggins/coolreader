# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = po @SUBDIRS@

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

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = config.rpath m4/ChangeLog

check-gettext:
	@if test x$(USE_NLS) != "xyes" ; then echo "Missing gettext. Rerun configure and check for" \
	"'checking whether to use NLS... yes'!" ; exit 1 ; fi

update-po: check-gettext
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po
#	@find $(srcdir)/src/ -name "*.cpp" -print | sort > $(srcdir)/po/POTFILES.in.2 ; \
#	if diff $(srcdir)/po/POTFILES.in $(srcdir)/po/POTFILES.in.2 >/dev/null 2>&1 ; then \
#		rm -f $(srcdir)/po/POTFILES.in.2 ; \
#	else \
#		mv $(srcdir)/po/POTFILES.in.2 $(srcdir)/po/POTFILES.in ; \
#	fi
    
update-gmo: check-gettext
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo: check-gettext
	touch po/*.po
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo-%: check-gettext
	@language=`echo $@ | sed s/force-update-gmo-//` ; \
	if test ! -f po/$$language.po ; then echo "file po/$$language.po does not exist" ; exit 1 ; fi ; \
	touch po/$$language.po ; \
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

.PHONY: check-gettext update-po update-gmo force-update-gmo
