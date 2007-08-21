#!/usr/bin/make

# Cool Reader Engine make file
# (c) Vadim Lopatin, 2007
#
# available options
#===================================
#
# Common options:
#   BUILD=Debug  - to build in debug mode
#   BUILD=Release - to build release (optimized) library
#   SHARED=1     - to build shared (.so) library
#   SHARED=0     - to build static (.a) library
#
# LBOOK build options:
#   LBOOK=arm    - to build for ARM
#   LBOOK=i386   - to build for Simulator
#   V2HOME=path  - path to V2SDK directory
#
# Library names:
#   lib/libcrengine.a - linux static release
#   lib/libcrengined.a - linux static debug
#   lib/libcrengine-arm.a - LBook static arm release
#   lib/libcrengine-arm-d.a - LBook static arm debug
#   lib/libcrengine-i386.a - LBook static simulator release
#   lib/libcrengine-i386-d.a - LBook static simulator debug

#.SILENT :

ifeq ($(LBOOK),arm)
    CC = arm-linux-g++
    HOSTTYPE = arm
else
    CC = g++
    HOSTTYPE = i386
endif

# source release name
releasefile = crengine-src.zip

#	inc =	-I../_Linux/IncludeX \
#			-I/usr/X11R6/include \
#			-I/usr/X11R6/include/freetype2 \
#			-I/usr/X11R6/include/X11/Xft \
#			-I/usr/X11R6/include/X11/extensions
#	lib =	-L/usr/X11R6/lib \
#			-lXft

# type of build we're doing
ifndef BUILD
	BUILD = Debug
endif

#V2 path
ifdef LBOOK
ifndef V2HOME
	V2HOME=../v3sdk-1.0/
endif
endif

# the directory for the output files
ifdef LBOOK
	builddir = build/lbook/$(HOSTTYPE)/$(BUILD)
else
	builddir = build/linux/$(HOSTTYPE)/$(BUILD)
endif

# the name of the target file
target = crengine

# path to input files
VPATH = % \
	$(builddir):\
	./src:

ccflags = -fPIC -w -Wall -fno-inline -fexceptions -fpermissive
defs = -DLGI_RES -DLINUX -D_LINUX -D_REENTRANT -D_FILE64_MAPPING

ifeq ($(USE_JPEG),0)
	defs += -DUSE_JPEG=0 
endif
ifeq ($(USE_PNG),0)
	defs += -DUSE_PNG=0 
endif

#LBOOK
ifdef LBOOK
	defs += -DLBOOK=1 -D_LINUX
endif

ifeq ($(BUILD),Debug)
	#debug builds..
	flags = -g $(ccflags) $(defs)
	defs += -D_DEBUG 
ifdef LBOOK
	tag = -$(HOSTTYPE)-d
else
	tag = d
endif
else
	#release builds
	flags = -s -Os $(ccflags) $(defs)
ifdef LBOOK
	tag = -$(HOSTTYPE)
endif
endif

ifeq ($(SHARED),1)
	targetfile = lib$(target)$(tag).so
else
	targetfile = lib$(target)$(tag).a
endif

# libraries
libs =		-static-libgcc \
			-lstdc++ \
			$(lib)

#			-L../Lgi/$(builddir) \
#			-lpthread \
#			-ldl \

#		-I ../freetype2/include \

#include paths
ifdef LBOOK
incpath =	-I.  -D$(HOSTTYPE) \
		-I$(V2HOME)/include \
		-I$(V2HOME)/include/freetype2 \
		-I $(V2HOME)/include/nanox \
		-I $(V2HOME)/include/flnx \
		-I $(V2HOME)/include/ -I include \
		-I $(V2HOME)/include/$(HOSTTYPE)/libjpeg \
		-I $(V2HOME)/include/$(HOSTTYPE)/libpng \
		-I $(V2HOME)/include/$(HOSTTYPE)/zlib \
		$(inc)

else
incpath =	-DUSE_FREETYPE=1 -I./include/common \
		-I /usr/include/freetype2 \
			$(inc)
endif
#			-I/usr/include/freetype2 \
#			-I/usr/lib/glib-2.0/include \
#			-I./../wxWidgets/src/jpeg \
#			-I./../wxWidgets/lib/wx/include/gtk2-unicode-release-static-2.8 \
#			-I./../wxWidgets/include \


# all the objects required
srcs	 =	\
		crtxtenc \
		cp_stats \
		hyphman \
		lstridmap \
		lvbmpbuf \
		lvdocview \
		lvdrawbuf \
		lvpagesplitter \
		lvfnt \
		lvfntman \
		lvimg \
		lvmemman \
		lvrend \
		lvstream \
		lvstring \
		lvstsheet \
		lvstyles \
		lvtextfm \
		lvtinydom \
		lvxml \
		hist \
		props \
		wolutil \

ifndef LBOOK
srcs	 +=	\
		xutils
endif

srcs	 +=	\
		$(deps)


# executable target
ifeq ($(SHARED),1)
$(targetfile) : $(addsuffix .o,$(srcs))
	@echo Linking $(targetfile) [$(BUILD)] as shared library
	$(CC) -nodefaultlibs -lc -shared \
		-Wl,-soname,$(targetfile),-export-dynamic \
		-o $(builddir)/$(targetfile) $(addprefix $(builddir)/,$(^F)) \
		$(libs) 
	cp $(builddir)/$(targetfile) lib
else
$(targetfile) : $(addsuffix .o,$(srcs))
	@echo Linking $(targetfile) [$(BUILD)] as static library
	ar cr $(builddir)/$(targetfile) $(addprefix $(builddir)/,$(^F))
	cp $(builddir)/$(targetfile) lib
endif

# compile command
%.o : %.cpp
	@echo "$(<F) [$(gui)]"
	ls $(builddir) > /dev/null 2> /dev/null || mkdir -p $(builddir)
	$(CC) $(incpath) $(flags) -c $< -o $(builddir)/$(@F)

# dependency generation
%.d : %.cpp
	ls .deps > /dev/null 2> /dev/null || mkdir .deps
	$(SHELL) -ec '$(CC) -M $(incpath) $(flags) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > .deps/$@; \
		[ -s $@ ] || rm -f $@'

depends : $(addsuffix .d,$(srcs))
	@echo "Dependencies done..."

# include all the dependency files...
-include $(addprefix .deps/,$(addsuffix .d,$(srcs)))

#clean the directory out
clean :
	echo Cleaning $(builddir)
	rm -f $(builddir)/*
	rm -rf .deps
	rm -f lib/$(targetfile)
	
doc : FORCE
	cd doxygen; doxygen ./crengine.cfg
	
FORCE :

ZipFlags = -x '*/.svn/*'

app :
	cd Tools/Fb2Linux
	make clean
	make BUILD=$(BUILD)
	cd ../..

appclean :
	rm -rf Tools/Fb2Linux/$(BUILD)/*

distrib : 
	rm -f $(releasefile) 2> /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/src	-i '*.cpp'	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/include	-i '*.h'	$(ZipFlags) > /dev/null
#	cd ..; zip -9 -r crengine/$(releasefile) crengine/lib/libcrengined.a	$(ZipFlags) > /dev/null
#	cd ..; zip -9 -r crengine/$(releasefile) crengine/lib/libcrengine.a	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/lib/projects/vc6	-i '*.dsp' '*.dsw'	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/Tools/FontConv	-i '*.h' '*.cpp' '*.txt' '*.rc' '*.clw' '*.dsp' '*.dsw'	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/Tools/FontConv/res	-i '*.*'	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/Tools/Fb2Test	-i '*.h' '*.res' '*.cpp' '*.txt' '*.rc' '*.clw' '*.dsp' '*.dsw'	'*.css' '*.fb2'  $(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/Tools/Fb2Linux	-i '*.cpp' '*.h' '*.css' '*.fb2'	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/Tools/Fb2Linux/makefile	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine/doxygen	-i '*.cfg'	$(ZipFlags) > /dev/null
	cd ..; zip -9 -r crengine/$(releasefile) crengine -i '*.sh'  $(ZipFlags)
	cd ..; zip -9 -r crengine/$(releasefile) crengine/LICENSE	$(ZipFlags)
	cd ..; zip -9 -r crengine/$(releasefile) crengine/Makefile	$(ZipFlags)
	cd ..; zip -9 -r crengine/$(releasefile) crengine	-i '*.txt' '*.bat' $(ZipFlags) > /dev/null
	ls -l $(releasefile)

all :
	make clean BUILD=Release SHARED=1
	make clean BUILD=Debug SHARED=1
	make clean BUILD=Release SHARED=0
	make clean BUILD=Debug SHARED=0
	make BUILD=Release SHARED=0
	make BUILD=Debug SHARED=0
	make BUILD=Release appclean
	make BUILD=Debug appclean
	make BUILD=Release app
	make BUILD=Debug app
#	make doc
#	make distrib
#	make BUILD=Release SHARED=1
#	make BUILD=Debug SHARED=1
