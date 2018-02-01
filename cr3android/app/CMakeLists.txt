# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

project(cr3engine)

set(CR3_ROOT ${PROJECT_SOURCE_DIR}/../..)

include_directories(${CR3_ROOT}/crengine/include)
include_directories(${CR3_ROOT}/thirdparty/libpng)
include_directories(${CR3_ROOT}/thirdparty/freetype/include)
include_directories(${CR3_ROOT}/thirdparty/libjpeg)
include_directories(${CR3_ROOT}/thirdparty/antiword)
include_directories(${CR3_ROOT}/thirdparty/chmlib/src)

add_definitions(-DLINUX=1 -D_LINUX=1 -DFOR_ANDROID=1 -DCR3_PATCH -DFT2_BUILD_LIBRARY=1)
add_definitions(-DDOC_DATA_COMPRESSION_LEVEL=1 -DDOC_BUFFER_SIZE=0x1000000)
add_definitions(-DENABLE_CACHE_FILE_CONTENTS_VALIDATION=1)
add_definitions(-DLDOM_USE_OWN_MEM_MAN=0)
add_definitions(-DCR3_ANTIWORD_PATCH=1 -DENABLE_ANTIWORD=1)
add_definitions(-DMAX_IMAGE_SCALE_MUL=2)

#set(LOCAL_CFLAGS "-Wno-psabi -Wno-unused-variable -Wno-sign-compare -Wno-write-strings -Wno-main -Wno-unused-but-set-variable -Wno-unused-function -Wall")
#set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -funwind-tables")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LOCAL_CFLAGS}")

set(CRENGINE_SRC_FILES
    ${CR3_ROOT}/crengine/src/cp_stats.cpp
    ${CR3_ROOT}/crengine/src/lvstring.cpp
    ${CR3_ROOT}/crengine/src/props.cpp
    ${CR3_ROOT}/crengine/src/lstridmap.cpp
    ${CR3_ROOT}/crengine/src/rtfimp.cpp
    ${CR3_ROOT}/crengine/src/lvmemman.cpp
    ${CR3_ROOT}/crengine/src/lvstyles.cpp
    ${CR3_ROOT}/crengine/src/crtxtenc.cpp
    ${CR3_ROOT}/crengine/src/lvtinydom.cpp
    ${CR3_ROOT}/crengine/src/lvstream.cpp
    ${CR3_ROOT}/crengine/src/lvxml.cpp
    ${CR3_ROOT}/crengine/src/chmfmt.cpp
    ${CR3_ROOT}/crengine/src/epubfmt.cpp
    ${CR3_ROOT}/crengine/src/pdbfmt.cpp
    ${CR3_ROOT}/crengine/src/wordfmt.cpp
    ${CR3_ROOT}/crengine/src/lvstsheet.cpp
    ${CR3_ROOT}/crengine/src/txtselector.cpp
    ${CR3_ROOT}/crengine/src/crtest.cpp
    ${CR3_ROOT}/crengine/src/lvbmpbuf.cpp
    ${CR3_ROOT}/crengine/src/lvfnt.cpp
    ${CR3_ROOT}/crengine/src/hyphman.cpp
    ${CR3_ROOT}/crengine/src/lvfntman.cpp
    ${CR3_ROOT}/crengine/src/lvimg.cpp
    ${CR3_ROOT}/crengine/src/crskin.cpp
    ${CR3_ROOT}/crengine/src/lvdrawbuf.cpp
    ${CR3_ROOT}/crengine/src/lvdocview.cpp
    ${CR3_ROOT}/crengine/src/lvpagesplitter.cpp
    ${CR3_ROOT}/crengine/src/lvtextfm.cpp
    ${CR3_ROOT}/crengine/src/lvrend.cpp
    ${CR3_ROOT}/crengine/src/wolutil.cpp
    ${CR3_ROOT}/crengine/src/crconcurrent.cpp
    ${CR3_ROOT}/crengine/src/hist.cpp
)
#    ${CR3_ROOT}/crengine/src/cri18n.cpp
#    ${CR3_ROOT}/crengine/src/crgui.cpp

set(PNG_SRC_FILES
    ${CR3_ROOT}/thirdparty/libpng/pngerror.c
    ${CR3_ROOT}/thirdparty/libpng/pngget.c
    ${CR3_ROOT}/thirdparty/libpng/pngpread.c
    ${CR3_ROOT}/thirdparty/libpng/pngrio.c
    ${CR3_ROOT}/thirdparty/libpng/pngrutil.c
    ${CR3_ROOT}/thirdparty/libpng/pngvcrd.c
    ${CR3_ROOT}/thirdparty/libpng/png.c
    ${CR3_ROOT}/thirdparty/libpng/pngwrite.c
    ${CR3_ROOT}/thirdparty/libpng/pngwutil.c
    ${CR3_ROOT}/thirdparty/libpng/pnggccrd.c
    ${CR3_ROOT}/thirdparty/libpng/pngmem.c
    ${CR3_ROOT}/thirdparty/libpng/pngread.c
    ${CR3_ROOT}/thirdparty/libpng/pngrtran.c
    ${CR3_ROOT}/thirdparty/libpng/pngset.c
    ${CR3_ROOT}/thirdparty/libpng/pngtrans.c
    ${CR3_ROOT}/thirdparty/libpng/pngwio.c
    ${CR3_ROOT}/thirdparty/libpng/pngwtran.c
)

set(JPEG_SRC_FILES
    ${CR3_ROOT}/thirdparty/libjpeg/jaricom.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcapimin.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcapistd.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcarith.c
    ${CR3_ROOT}/thirdparty/libjpeg/jccoefct.c
    ${CR3_ROOT}/thirdparty/libjpeg/jccolor.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcdctmgr.c
    ${CR3_ROOT}/thirdparty/libjpeg/jchuff.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcinit.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcmainct.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcmarker.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcmaster.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcomapi.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcparam.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcprepct.c
    ${CR3_ROOT}/thirdparty/libjpeg/jcsample.c
    ${CR3_ROOT}/thirdparty/libjpeg/jctrans.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdapimin.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdapistd.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdarith.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdatadst.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdatasrc.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdcoefct.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdcolor.c
    ${CR3_ROOT}/thirdparty/libjpeg/jddctmgr.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdhuff.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdinput.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdmainct.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdmarker.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdmaster.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdmerge.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdpostct.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdsample.c
    ${CR3_ROOT}/thirdparty/libjpeg/jdtrans.c
    ${CR3_ROOT}/thirdparty/libjpeg/jerror.c
    ${CR3_ROOT}/thirdparty/libjpeg/jfdctflt.c
    ${CR3_ROOT}/thirdparty/libjpeg/jfdctfst.c
    ${CR3_ROOT}/thirdparty/libjpeg/jfdctint.c
    ${CR3_ROOT}/thirdparty/libjpeg/jidctflt.c
    ${CR3_ROOT}/thirdparty/libjpeg/jidctfst.c
    ${CR3_ROOT}/thirdparty/libjpeg/jidctint.c
    ${CR3_ROOT}/thirdparty/libjpeg/jquant1.c
    ${CR3_ROOT}/thirdparty/libjpeg/jquant2.c
    ${CR3_ROOT}/thirdparty/libjpeg/jutils.c
    ${CR3_ROOT}/thirdparty/libjpeg/jmemmgr.c
    ${CR3_ROOT}/thirdparty/libjpeg/jmemnobs.c
)

set(FREETYPE_SRC_FILES
    ${CR3_ROOT}/thirdparty/freetype/src/autofit/autofit.c
    ${CR3_ROOT}/thirdparty/freetype/src/bdf/bdf.c
    ${CR3_ROOT}/thirdparty/freetype/src/cff/cff.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftbase.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftbbox.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftbdf.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftbitmap.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftgasp.c
    ${CR3_ROOT}/thirdparty/freetype/src/cache/ftcache.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftglyph.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftgxval.c
    ${CR3_ROOT}/thirdparty/freetype/src/gzip/ftgzip.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftinit.c
    ${CR3_ROOT}/thirdparty/freetype/src/lzw/ftlzw.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftmm.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftpatent.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftotval.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftpfr.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftstroke.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftsynth.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftsystem.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/fttype1.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftwinfnt.c
    ${CR3_ROOT}/thirdparty/freetype/src/base/ftxf86.c
    ${CR3_ROOT}/thirdparty/freetype/src/winfonts/winfnt.c
    ${CR3_ROOT}/thirdparty/freetype/src/pcf/pcf.c
    ${CR3_ROOT}/thirdparty/freetype/src/pfr/pfr.c
    ${CR3_ROOT}/thirdparty/freetype/src/psaux/psaux.c
    ${CR3_ROOT}/thirdparty/freetype/src/pshinter/pshinter.c
    ${CR3_ROOT}/thirdparty/freetype/src/psnames/psmodule.c
    ${CR3_ROOT}/thirdparty/freetype/src/raster/raster.c
    ${CR3_ROOT}/thirdparty/freetype/src/sfnt/sfnt.c
    ${CR3_ROOT}/thirdparty/freetype/src/smooth/smooth.c
    ${CR3_ROOT}/thirdparty/freetype/src/truetype/truetype.c
    ${CR3_ROOT}/thirdparty/freetype/src/type1/type1.c
    ${CR3_ROOT}/thirdparty/freetype/src/cid/type1cid.c
    ${CR3_ROOT}/thirdparty/freetype/src/type42/type42.c
)

set(CHM_SRC_FILES
    ${CR3_ROOT}/thirdparty/chmlib/src/chm_lib.c
    ${CR3_ROOT}/thirdparty/chmlib/src/lzx.c
)

set(ANTIWORD_SRC_FILES
    ${CR3_ROOT}/thirdparty/antiword/asc85enc.c
    ${CR3_ROOT}/thirdparty/antiword/blocklist.c
    ${CR3_ROOT}/thirdparty/antiword/chartrans.c
    ${CR3_ROOT}/thirdparty/antiword/datalist.c
    ${CR3_ROOT}/thirdparty/antiword/depot.c
    ${CR3_ROOT}/thirdparty/antiword/doclist.c
    ${CR3_ROOT}/thirdparty/antiword/fail.c
    ${CR3_ROOT}/thirdparty/antiword/finddata.c
    ${CR3_ROOT}/thirdparty/antiword/findtext.c
    ${CR3_ROOT}/thirdparty/antiword/fontlist.c
    ${CR3_ROOT}/thirdparty/antiword/fonts.c
    ${CR3_ROOT}/thirdparty/antiword/fonts_u.c
    ${CR3_ROOT}/thirdparty/antiword/hdrftrlist.c
    ${CR3_ROOT}/thirdparty/antiword/imgexam.c
    ${CR3_ROOT}/thirdparty/antiword/listlist.c
    ${CR3_ROOT}/thirdparty/antiword/misc.c
    ${CR3_ROOT}/thirdparty/antiword/notes.c
    ${CR3_ROOT}/thirdparty/antiword/options.c
    ${CR3_ROOT}/thirdparty/antiword/out2window.c
    ${CR3_ROOT}/thirdparty/antiword/pdf.c
    ${CR3_ROOT}/thirdparty/antiword/pictlist.c
    ${CR3_ROOT}/thirdparty/antiword/prop0.c
    ${CR3_ROOT}/thirdparty/antiword/prop2.c
    ${CR3_ROOT}/thirdparty/antiword/prop6.c
    ${CR3_ROOT}/thirdparty/antiword/prop8.c
    ${CR3_ROOT}/thirdparty/antiword/properties.c
    ${CR3_ROOT}/thirdparty/antiword/propmod.c
    ${CR3_ROOT}/thirdparty/antiword/rowlist.c
    ${CR3_ROOT}/thirdparty/antiword/sectlist.c
    ${CR3_ROOT}/thirdparty/antiword/stylelist.c
    ${CR3_ROOT}/thirdparty/antiword/stylesheet.c
    ${CR3_ROOT}/thirdparty/antiword/summary.c
    ${CR3_ROOT}/thirdparty/antiword/tabstop.c
    ${CR3_ROOT}/thirdparty/antiword/unix.c
    ${CR3_ROOT}/thirdparty/antiword/utf8.c
    ${CR3_ROOT}/thirdparty/antiword/word2text.c
    ${CR3_ROOT}/thirdparty/antiword/worddos.c
    ${CR3_ROOT}/thirdparty/antiword/wordlib.c
    ${CR3_ROOT}/thirdparty/antiword/wordmac.c
    ${CR3_ROOT}/thirdparty/antiword/wordole.c
    ${CR3_ROOT}/thirdparty/antiword/wordwin.c
    ${CR3_ROOT}/thirdparty/antiword/xmalloc.c
)

set(COFFEECATCH_SRC_FILES
    src/main/cpp/coffeecatch/coffeecatch.c
    src/main/cpp/coffeecatch/coffeejni.c
)

set(JNI_SRC_FILES
    src/main/cpp/cr3engine.cpp
    src/main/cpp/cr3java.cpp
    src/main/cpp/docview.cpp
)

set(LOCAL_SRC_FILES
    ${JNI_SRC_FILES}
    ${CRENGINE_SRC_FILES}
    ${FREETYPE_SRC_FILES}
    ${PNG_SRC_FILES}
    ${JPEG_SRC_FILES}
    ${CHM_SRC_FILES}
    ${ANTIWORD_SRC_FILES}
)

if("${ANDROID_ABI}" STREQUAL "armeabi-v7a")
set(LOCAL_SRC_FILES ${LOCAL_SRC_FILES} ${COFFEECATCH_SRC_FILES})
endif("${ANDROID_ABI}" STREQUAL "armeabi-v7a")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             cr3engine-3-1-1

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ${LOCAL_SRC_FILES} )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       cr3engine-3-1-1

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       -lm -lz -ldl)
