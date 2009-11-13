PROJECT(cr3)
cmake_minimum_required(VERSION 2.6)

if (NOT DEFINED GUI)
  message("GUI type is not specified!")
  message("Using ${GUI} as default")
  message("Add cmake parameter -D GUI={QT|WX|CRGUI_XCB|CRGUI_NANOX|CRGUI_JINKE_PLUGIN|CRGUI_WIN32} to use another GUI frontend")
  SET(GUI QT)
else ()
  message("Using GUI frontend ${GUI}")
endif (NOT DEFINED GUI)

if ( WIN32 )
  ADD_DEFINITIONS( -DWIN32=1 -D_WIN32=1 )
else()
  ADD_DEFINITIONS( -DLINUX=1 -D_LINUX=1 )
endif( WIN32 )

SET (CRGUI_DEFS -DCOLOR_BACKBUFFER=0 -DLDOM_USE_OWN_MEM_MAN=1 )
SET (DESKTOP_DEFS -DCOLOR_BACKBUFFER=1 -DLDOM_USE_OWN_MEM_MAN=1 -DUSE_FREETYPE=1 )

FIND_PACKAGE(ZLIB)
if (NOT ZLIB_FOUND)
  message("System ZLIB not found, will build local one")
  #to disable #include <qconfig.h>
  ADD_DEFINITIONS(-DCR3_PATCH=1)
  ADD_SUBDIRECTORY(thirdparty/zlib)
  SET(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlib)
  SET(ZLIB_LIBRARIES z)
endif (NOT ZLIB_FOUND)

FIND_PACKAGE(PNG)
if (NOT PNG_FOUND)
  message("System LIBPNG not found, will build local one")
  #to disable #include <qconfig.h>
  ADD_DEFINITIONS(-DCR3_PATCH=1)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
  ADD_SUBDIRECTORY(thirdparty/libpng)
  SET(PNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng)
  SET(PNG_LIBRARIES png)
endif (NOT PNG_FOUND)

FIND_PACKAGE(JPEG)
if (NOT JPEG_FOUND)
  message("System LIBJPEG not found, will build local one")
  ADD_SUBDIRECTORY(thirdparty/libjpeg)
  SET(JPEG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libjpeg)
  SET(JPEG_LIBRARIES jpeg)
endif (NOT JPEG_FOUND)

FIND_PACKAGE(Freetype)
if (NOT FREETYPE_FOUND)
  message("System FREETYPE not found, will build local one")
  ADD_SUBDIRECTORY(thirdparty/freetype)
  SET(FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/freetype/include)
  SET(FREETYPE_LIBRARIES jpeg)
endif (NOT FREETYPE_FOUND)


INCLUDE_DIRECTORIES( 
  ${PNG_INCLUDE_DIR} 
  ${JPEG_INCLUDE_DIR} 
  ${ZLIB_INCLUDE_DIR} 
  ${FREETYPE_INCLUDE_DIRS}
)
SET(STD_LIBS 
  ${JPEG_LIBRARIES} 
  ${FREETYPE_LIBRARIES} 
  ${PNG_LIBRARIES} 
  ${ZLIB_LIBRARIES} 
)

include_directories(crengine/include)
include_directories(tinydict)
include_directories(${FT_INCLUDE_PATH})

message("Will make tinydict library - .dict format support")
ADD_SUBDIRECTORY(tinydict)

if ( ${GUI} STREQUAL CRGUI_XCB )
  message("Will make CR3GUI for XCB")
  ADD_DEFINITIONS( -DCR_USE_XCB=1 ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(crengine)
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI} STREQUAL CRGUI_NANOX )
  message("Will make CR3GUI for NanoX")
  ADD_DEFINITIONS( -DLBOOK=1 -DWITH_JINKE=1 -DCR_USE_JINKE=1 -DJINKE_VIEWER ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(crengine)
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )
  message("Will make CR3GUI plugin for Jinke Viewer")
  ADD_DEFINITIONS( -DLBOOK=1 -DWITH_JINKE=1 -DWITH_JINKE_APP=1 -DCR_USE_JINKE=1 ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(crengine)
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI} STREQUAL CRGUI_WIN32 )
  message("Will make CR3GUI for Win32")
  ADD_DEFINITIONS( ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(crengine)
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI} STREQUAL QT )
  message("Will make CR3/QT")
  ADD_DEFINITIONS( ${DESKTOP_DEFS} )
  ADD_SUBDIRECTORY(crengine)
  ADD_SUBDIRECTORY(cr3qt)
elseif ( ${GUI} STREQUAL "WX" )
  message("Will make CR3/WX")
  ADD_DEFINITIONS( ${DESKTOP_DEFS} -DCR_WX_SUPPORT=1 )
  SET(wxWidgets_USE_LIBS base core )
  FIND_PACKAGE(wxWidgets)
  IF (wxWidgets_FOUND)
    INCLUDE(${wxWidgets_USE_FILE})
    include_directories( ${wxWidgets_INCLUDE_DIRS} )
    message( "WX include: ${wxWidgets_INCLUDE_DIRS}" )
  ELSE (wxWidgets_FOUND)
    message("wxWidgets library not found!")
  ENDIF (wxWidgets_FOUND)
  ADD_SUBDIRECTORY(crengine)
  ADD_SUBDIRECTORY(cr3wx)
else ( ${GUI} STREQUAL CRGUI_XCB )
  message("Unknown GUI type ${GUI}")
endif ( ${GUI} STREQUAL CRGUI_XCB )
