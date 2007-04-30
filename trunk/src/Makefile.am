# these are the headers for your project
noinst_HEADERS = cr3.h rescont.h view.h toc.h

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = cr3

# the application source, library search path, and link libraries
cr3_SOURCES = cr3.cpp view.cpp wolopt.cpp toc.cpp
cr3_LDFLAGS = $(WX_LIBS)

