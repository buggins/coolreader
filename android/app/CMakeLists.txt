# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

project(cr3engine)

set(CR3_ROOT ${PROJECT_SOURCE_DIR}/../..)

include_directories(${CR3_ROOT}/crengine/include)
include_directories(${CR3_ROOT}/crengine/fc-lang)
include_directories(${CR3_ROOT}/thirdparty/libpng)
include_directories(${CR3_ROOT}/thirdparty/freetype)
include_directories(${CR3_ROOT}/thirdparty/freetype/include)
include_directories(${CR3_ROOT}/thirdparty/harfbuzz/src)
include_directories(${CR3_ROOT}/thirdparty/libjpeg)
include_directories(${CR3_ROOT}/thirdparty/antiword)
include_directories(${CR3_ROOT}/thirdparty/chmlib/src)

add_definitions(-DLINUX=1 -D_LINUX=1 -DFOR_ANDROID=1 -DCR3_PATCH)
add_definitions(-DFT_CONFIG_MODULES_H=<builds/android/include/config/ftmodule.h> -DFT_CONFIG_OPTIONS_H=<builds/android/include/config/ftoption.h>)
add_definitions(-DDOC_DATA_COMPRESSION_LEVEL=1 -DDOC_BUFFER_SIZE=0x1000000)
add_definitions(-DENABLE_CACHE_FILE_CONTENTS_VALIDATION=1)
add_definitions(-DLDOM_USE_OWN_MEM_MAN=0)
add_definitions(-DCR3_ANTIWORD_PATCH=1 -DENABLE_ANTIWORD=1)
add_definitions(-DMAX_IMAGE_SCALE_MUL=2)

#set(LOCAL_CFLAGS "-Wno-psabi -Wno-unused-variable -Wno-sign-compare -Wno-write-strings -Wno-main -Wno-unused-but-set-variable -Wno-unused-function -Wall")
# Option '-Wl,--no-merge-exidx-entries' removed as incompatible with clang.
set(LOCAL_CFLAGS "${LOCAL_CFLAGS} -funwind-tables")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LOCAL_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LOCAL_CFLAGS}")

set(CMAKE_C_FLAGS_DEBUG             "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE           "-O2 -g0 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO    "-O2 -g1 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG           ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_RELEASE         ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  ${CMAKE_C_FLAGS_RELWITHDEBINFO})

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
    ${CR3_ROOT}/crengine/fc-lang/fc-lang-cat.c
)
#    ${CR3_ROOT}/crengine/src/cri18n.cpp
#    ${CR3_ROOT}/crengine/src/crgui.cpp

set(COFFEECATCH_SRC_FILES
    ${CR3_ROOT}/android/jni/coffeecatch/coffeecatch.c
    ${CR3_ROOT}/android/jni/coffeecatch/coffeejni.c
)

set(JNI_SRC_FILES
    ${CR3_ROOT}/android/jni/cr3engine.cpp
    ${CR3_ROOT}/android/jni/cr3java.cpp
    ${CR3_ROOT}/android/jni/docview.cpp
)

set(LOCAL_SRC_FILES
    ${JNI_SRC_FILES}
    ${CRENGINE_SRC_FILES}
)

# Add coffeecatch only on supported architectures.
if("${ANDROID_ABI}" STREQUAL "armeabi-v7a" OR "${ANDROID_ABI}" STREQUAL "arm64-v8a" OR "${ANDROID_ABI}" STREQUAL "mips" OR "${ANDROID_ABI}" STREQUAL "x86")
    set(LOCAL_SRC_FILES ${LOCAL_SRC_FILES} ${COFFEECATCH_SRC_FILES})
endif("${ANDROID_ABI}" STREQUAL "armeabi-v7a" OR "${ANDROID_ABI}" STREQUAL "arm64-v8a" OR "${ANDROID_ABI}" STREQUAL "mips" OR "${ANDROID_ABI}" STREQUAL "x86")

add_subdirectory(thirdparty_libs)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             cr3engine-3-2-X

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
                       cr3engine-3-2-X

                       # thirdparty static libs
                       png
                       jpeg
                       freetype
                       harfbuzz
                       chmlib
                       antiword

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       -lm -lz -ldl)
