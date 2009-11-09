# these are the headers for your project
noinst_HEADERS = cr3.h

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = cr3

# the application source, library search path, and link libraries
cr3_SOURCES = cr3.cpp histlist.cpp optdlg.cpp toc.cpp view.cpp wolopt.cpp
cr3_LDFLAGS = $(WX_LIBS) 

cr3_LDADD = $(top_builddir)/crengine/src/libcrengine.a

crenginedir = $(datadir)/crengine
crengine_DATA = fb2.css
AM_CFLAGS = -DCR_WX_SUPPORT
AM_CXXFLAGS = -DCR_WX_SUPPORT
