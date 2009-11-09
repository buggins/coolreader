# build CR3 on linux
# crengine dir should be placed on the same level as cr3
# required libs: wxgtk, zlib, libjpeg, libpng, freetype
ls crengine > /dev/null 2> /dev/null || ln -s ../crengine crengine
WANT_AUTOCONF_2_5="1" WANT_AUTOMAKE_1_6="1" make -f Makefile.cvs
ls optimized > /dev/null 2> /dev/null || mkdir optimized
basedir=`pwd`
cd $basedir/optimized && CPPFLAGS="-I$basedir/crengine/include -I/usr/include/freetype2 -DLINUX -D_LINUX" CXXFLAGS="-O2 -g0" $basedir/configure \
&& cd $basedir/optimized && make
