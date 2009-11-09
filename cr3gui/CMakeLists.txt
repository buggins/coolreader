PROJECT(cr3)

FIND_PATH(FT_INCLUDE_PATH freetype/freetype.h
 /usr/include/freetype2
 /usr/local/include/freetype2
 )
include_directories(crengine/include)
include_directories(tinydict)
include_directories(${FT_INCLUDE_PATH})

add_definitions(-DLINUX=1 -D_LINUX=1 
-DLDOM_USE_OWN_MEM_MAN=1 
-DCOLOR_BACKBUFFER=0
-DCR_USE_XCB=1
)
SET (TINYDICT_SOURCES
tinydict/tinydict.cpp
)
ADD_LIBRARY(tinydict STATIC ${TINYDICT_SOURCES})

SET (CRENGINE_SOURCES 
crengine/src/cp_stats.cpp  crengine/src/hyphman.cpp    crengine/src/lvfntman.cpp        crengine/src/lvstring.cpp  crengine/src/props.cpp
crengine/src/crgui.cpp     crengine/src/lstridmap.cpp  crengine/src/lvimg.cpp           crengine/src/lvstsheet.cpp crengine/src/rtfimp.cpp
crengine/src/cri18n.cpp    crengine/src/lvbmpbuf.cpp   crengine/src/lvmemman.cpp        crengine/src/lvstyles.cpp  
crengine/src/crskin.cpp    crengine/src/lvdocview.cpp  crengine/src/lvpagesplitter.cpp  crengine/src/lvtextfm.cpp
crengine/src/crtxtenc.cpp  crengine/src/lvdrawbuf.cpp  
crengine/src/lvrend.cpp    crengine/src/lvtinydom.cpp 
crengine/src/wolutil.cpp
crengine/src/hist.cpp      crengine/src/lvfnt.cpp      
crengine/src/lvstream.cpp  crengine/src/lvxml.cpp     
crengine/src/xutils.cpp
)
ADD_LIBRARY(crengine STATIC ${CRENGINE_SOURCES})

SET (CR3_SOURCES 
src/bgfit.cpp src/citedlg.cpp 
src/cr3xcb.cpp src/fsmenu.cpp    
src/mainwnd.cpp src/recentdlg.cpp 
src/selnavig.cpp src/tocdlg.cpp 
src/bmkdlg.cpp  src/cr3_def_cover.cpp 
src/cr3main.cpp src/dictdlg.cpp 
src/linksdlg.cpp  src/numedit.cpp 
src/scrkbd.cpp src/settings.cpp 
src/viewdlg.cpp
)

ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
SET (EXTRA_LIBS xcb xcb-aux xcb-image xcb-keysyms fontconfig freetype jpeg png z)
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS})

