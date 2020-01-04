
option(BUILD_TOOLS "Build tools" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fc-lang)

SET (CRENGINE_SOURCES 
src/cp_stats.cpp  
src/lvstring.cpp  
src/props.cpp
src/lstridmap.cpp  
src/rtfimp.cpp
src/cri18n.cpp    
src/lvmemman.cpp        
src/lvstyles.cpp  
src/crtxtenc.cpp  
src/lvtinydom.cpp 
src/lvstream.cpp  
src/lvxml.cpp     
src/lvstsheet.cpp 
src/txtselector.cpp
#src/xutils.cpp
src/crtest.cpp
fc-lang/fc-lang-cat.c
src/xxhash.c
)

if ( NOT ${GUI} STREQUAL FB2PROPS )
    SET (CRENGINE_SOURCES ${CRENGINE_SOURCES}
    src/lvbmpbuf.cpp   
    src/lvfnt.cpp      
    src/hyphman.cpp  
    src/lvembeddedfont.cpp
    src/lvfont.cpp
    src/lvfntman.cpp        
    src/crgui.cpp     
    src/lvimg.cpp           
    src/crskin.cpp    
    src/lvdrawbuf.cpp  
    src/lvdocview.cpp  
    src/lvpagesplitter.cpp  
    src/lvtextfm.cpp
    src/lvrend.cpp
    src/wolutil.cpp
    src/hist.cpp      
    src/chmfmt.cpp     
    src/epubfmt.cpp     
    src/pdbfmt.cpp     
    src/wordfmt.cpp
    src/lvopc.cpp
    src/docxfmt.cpp
    src/fb3fmt.cpp
    src/crconcurrent.cpp
    src/private/lvbasefont.cpp
    src/private/lvbitmapfont.cpp
    src/private/lvbitmapfontman.cpp
    src/private/lvfontglyphcache.cpp
    src/private/lvfontcache.cpp
    src/private/lvfontboldtransform.cpp
    src/private/lvfontdef.cpp
    src/private/lvwin32font.cpp
    src/private/lvwin32fontman.cpp
    src/private/lvfreetypeface.cpp
    src/private/lvfreetypefontman.cpp
    #src/xutils.cpp
    )
endif (NOT ${GUI} STREQUAL FB2PROPS)

ADD_LIBRARY(crengine STATIC ${CRENGINE_SOURCES})

if(BUILD_TOOLS)
    add_subdirectory(Tools)
endif(BUILD_TOOLS)
