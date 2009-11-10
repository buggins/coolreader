PROJECT(cr3)

if (NOT DEFINED ${GUI})
  if ( WIN32 )
    SET(GUI, CRGUI_WIN32)
    ADD_DEFINITIONS( -DWIN32=1 -D_WIN32=1 )
  else( WIN32 )
    SET(GUI, CRGUI_XCB)
    ADD_DEFINITIONS( -DLINUX=1 -D_LINUX=1 )
  endif( WIN32 )
endif (NOT DEFINED ${GUI})

FIND_PATH(FT_INCLUDE_PATH freetype/freetype.h
 /usr/include/freetype2
 /usr/local/include/freetype2
 )
include_directories(crengine/include)
include_directories(tinydict)
include_directories(${FT_INCLUDE_PATH})

add_definitions(
-DLINUX=1 -D_LINUX=1 
)

ADD_SUBDIRECTORY(tinydict)
ADD_SUBDIRECTORY(crengine)

SET (CRGUI_DEFS -DCOLOR_BACKBUFFER=0 -DLDOM_USE_OWN_MEM_MAN=1 )
if ( WIN32 )
  ADD_DEFINITIONS( -DWIN32=1 -D_WIN32=1 )
else( WIN32 )
  ADD_DEFINITIONS( -DLINUX=1 -D_LINUX=1 )
endif( WIN32 )

if ( ${GUI}==CRGUI_XCB )
  ADD_DEFINITIONS( -DCR_USE_XCB=1 ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI}==CRGUI_NANOX )
  ADD_DEFINITIONS( -DWITH_JINKE=1 -DCR_USE_JINKE=1 ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI}==CRGUI_JINKE_PLUGIN )
  ADD_DEFINITIONS( -DWITH_JINKE=1 -DWITH_JINKE_APP=1 -DCR_USE_JINKE=1 ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI}==CRGUI_WIN32 )
  ADD_DEFINITIONS( ${CRGUI_DEFS} )
  ADD_SUBDIRECTORY(cr3gui)
elseif ( ${GUI}==QT )
  ADD_SUBDIRECTORY(cr3qt)
elseif ( ${GUI}==WX )
  ADD_SUBDIRECTORY(cr3wx)
endif ( ${GUI}==CRGUI_XCB )
