CoolReader 3 - free e-book reader
(c) Vadim Lopatin, 1998-2009

LICENSE: All source codes (except thirdparty directory) 
are provided under the terms of GNU GPL license, version 2

Source codes are available at sourceforge:
git clone git://crengine.git.sourceforge.net/gitroot/crengine/crengine



Directories:

crengine   - CREngine (DOM/XML/CSS ebook rendering library) sources
cr3gui     - CR3 with CR3GUI for e-ink devices sources
cr3qt      - CR3 with Qt based GUI
cr3wx      - CR3 with wxWidgets based GUI
thirdparty - third party libraries, to use if not found in system (zlib, libpng, libjpeg, freetype)
tinydict   - small library for .dict file format support
tools      - miscellaneous configuration files

External dependencies: 

common: zlib, libpng, libjpeg, freetype
cr3gui/xcb: libxcb, fontconfig
cr3gui/nanoX: libnanoX
cr3/Qt: qt4-core, qt4-gui
cr3/wx: wxWidgets 2.8


Build instructions:

# Building QT version
# libqt4-dev should be installed
mkdir qtbuild
cd qtbuild
cmake -D GUI=QT ..
make
sudo make install

# Building wxWidgets version
# libwxgtk2.8-dev should be installed
mkdir wxbuild
cd wxbuild
cmake -D GUI=WX ..
make

# Building ARM version on OpenInkpot:
mkdir armbuild
cd armbuild
cmake -D CMAKE_TOOLCHAIN_FILE=../tools/toolchain-arm-oi.cmake -D CMAKE_BUILD_TYPE=Release -D GUI=CRGUI_XCB ..
make

# Building Jinke/LBook V3 viewer plugin (libfb2.so):
mkdir v3build
cd v3build
mkdir dest
cmake -D CMAKE_TOOLCHAIN_FILE=../tools/toolchain-arm-v3.cmake -D GUI=CRGUI_JINKE_PLUGIN -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=dest ..
make

# Building Jinke/LBook V3 viewer plugin (libfb2.so), new SDK:
mkdir v3build
cd v3build
mkdir dest
cmake -D CMAKE_TOOLCHAIN_FILE=../tools/toolchain-arm-linux-gnueabi.cmake -D GUI=CRGUI_JINKE_PLUGIN -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=dest ..
make

# Building Jinke/LBook V3 fb2props plugin for Bookshelf (libfb2props.so):
mkdir v3fb2propsbuild
cd v3fb2propsbuild
mkdir dest
cmake -D CMAKE_TOOLCHAIN_FILE=../tools/toolchain-arm-v3.cmake -D GUI=FB2PROPS -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=dest ..
make

# Building Jinke/LBook V5 viewer app (cr3):
mkdir v5build
cd v5build
cmake -D CMAKE_TOOLCHAIN_FILE=../tools/toolchain-arm-v5.cmake -D GUI=CRGUI_NANOX -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=dest ..
make
