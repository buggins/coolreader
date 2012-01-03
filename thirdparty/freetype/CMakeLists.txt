cmake_minimum_required(VERSION 2.6)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
ADD_DEFINITIONS(-DFT2_BUILD_LIBRARY=1)

SET (FREETYPE_SOURCES 
  src/autofit/autofit.c 
  src/bdf/bdf.c 
  src/cff/cff.c 
  src/base/ftbase.c 
  src/base/ftbbox.c 
  src/base/ftbdf.c 
  src/base/ftbitmap.c 
  src/base/ftgasp.c 
  src/cache/ftcache.c 
  src/base/ftglyph.c 
  src/base/ftgxval.c 
  src/gzip/ftgzip.c 
  src/base/ftinit.c 
  src/lzw/ftlzw.c 
  src/base/ftmm.c 
  src/base/ftotval.c 
  src/base/ftpfr.c 
  src/base/ftstroke.c 
  src/base/ftsynth.c 
  src/base/ftsystem.c 
  src/base/fttype1.c 
  src/base/ftwinfnt.c 
  src/base/ftxf86.c 
  src/pcf/pcf.c 
  src/pfr/pfr.c 
  src/psaux/psaux.c 
  src/pshinter/pshinter.c 
  src/psnames/psmodule.c 
  src/raster/raster.c 
  src/sfnt/sfnt.c 
  src/smooth/smooth.c 
  src/truetype/truetype.c 
  src/type1/type1.c 
  src/cid/type1cid.c 
  src/type42/type42.c 
  src/bzip2/ftbzip2.c
  src/winfonts/winfnt.c
)

if (WIN32)
SET (FREETYPE_SOURCES ${FREETYPE_SOURCES}
  builds/win32/ftdebug.c 
#  src/winfonts/winfnt.c
)
endif(WIN32)

if (DEFINED FREETYPE_DYNAMIC)
ADD_LIBRARY(freetype SHARED ${FREETYPE_SOURCES})
TARGET_LINK_LIBRARIES(freetype)
else()
ADD_LIBRARY(freetype STATIC ${FREETYPE_SOURCES})
endif (DEFINED FREETYPE_DYNAMIC)


