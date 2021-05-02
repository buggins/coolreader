
option(BUILD_TOOLS "Build tools" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fc-lang)

SET (CRENGINE_SOURCES
    src/cp_stats.cpp
    src/lvstring.cpp
    src/lvstring8collection.cpp
    src/lvstring32collection.cpp
    src/lvstring32hashedcollection.cpp
    src/serialbuf.cpp
    src/crlog.cpp
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
    src/xxhash.c
    src/textlang.cpp
    fc-lang/fc-lang-cat.c
)

if ( NOT ${GUI} STREQUAL FB2PROPS )
    SET (CRENGINE_SOURCES ${CRENGINE_SOURCES}
        src/lvbmpbuf.cpp   
        src/hyphman.cpp  
        src/crgui.cpp     
        src/crskin.cpp    
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
        src/odtfmt.cpp
        src/odxutil.cpp
        src/crconcurrent.cpp
        src/lvimg/lvcacheableobject.cpp
        src/lvimg/lvimagesource.cpp
        src/lvimg/crninepatchdecoder.cpp
        src/lvimg/lvjpegimagesource.cpp
        src/lvimg/lvpngimagesource.cpp
        src/lvimg/lvxpmimagesource.cpp
        src/lvimg/lvgifimagesource.cpp
        src/lvimg/lvgifframe.cpp
        src/lvimg/clzwdecoder.cpp
        src/lvimg/lvsvgimagesource.cpp
        src/lvimg/lvstretchimgsource.cpp
        src/lvimg/lvcolortransformimgsource.cpp
        src/lvimg/lvalphatransformimgsource.cpp
        src/lvimg/lvunpackedimgsource.cpp
        src/lvimg/lvdrawbufimgsource.cpp
        src/lvimg/lvimg.cpp
        src/lvfont/lvfont.cpp
        src/lvfont/lvfntman.cpp
        src/lvfont/lvfnt.cpp
        src/lvfont/lvembeddedfont.cpp
        src/lvfont/lvbasefont.cpp
        src/lvfont/lvbitmapfont.cpp
        src/lvfont/lvbitmapfontman.cpp
        src/lvfont/lvfontglyphcache.cpp
        src/lvfont/lvfontcache.cpp
        src/lvfont/lvfontboldtransform.cpp
        src/lvfont/lvfontdef.cpp
        src/lvfont/lvwin32font.cpp
        src/lvfont/lvwin32fontman.cpp
        src/lvfont/lvfreetypeface.cpp
        src/lvfont/lvfreetypefontman.cpp
        src/lvdrawbuf/lvbasedrawbuf.cpp
        src/lvdrawbuf/lvgraydrawbuf.cpp
        src/lvdrawbuf/lvcolordrawbuf.cpp
        src/lvdrawbuf/lvimagescaleddrawcallback.cpp
        src/lvdrawbuf/lvdrawbuf_utils.cpp
        #src/xutils.cpp
    )
endif (NOT ${GUI} STREQUAL FB2PROPS)

ADD_LIBRARY(crengine STATIC ${CRENGINE_SOURCES})

if(BUILD_TOOLS)
    add_subdirectory(Tools)
endif(BUILD_TOOLS)
