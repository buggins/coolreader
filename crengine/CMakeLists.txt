
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
    src/lvstsheet.cpp
    src/txtselector.cpp
    #src/xutils.cpp
    src/crtest.cpp
    src/textlang.cpp
    src/lvstream/lvdefstreambuffer.cpp
    src/lvstream/lvfilemappedstream.cpp
    src/lvstream/lvnamedstream.cpp
    src/lvstream/lvfilestream.cpp
    src/lvstream/lvdirectorycontainer.cpp
    src/lvstream/lvcachedstream.cpp
    src/lvstream/lvzipdecodestream.cpp
    src/lvstream/lvziparc.cpp
    src/lvstream/lvrararc.cpp
    src/lvstream/lvmemorystream.cpp
    src/lvstream/lvtcrstream.cpp
    src/lvstream/lvblockwritestream.cpp
    src/lvstream/lvstream.cpp
    src/lvstream/lvbase64stream.cpp
    src/lvstream/lvstreamutils.cpp
    src/lvxml/lvfileparserbase.cpp
    src/lvxml/lvtextfilebase.cpp
    src/lvxml/lvtextparser.cpp
    src/lvxml/lvtextlinequeue.cpp
    src/lvxml/pmltextimport.cpp
    src/lvxml/lvtextrobustparser.cpp
    src/lvxml/lvtextbookmarkparser.cpp
    src/lvxml/lvxmlparser.cpp
    src/lvxml/lvhtmlparser.cpp
    src/lvxml/fb2coverpageparsercallback.cpp
    src/lvxml/lvxmlutils.cpp
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
        src/lvdrawbuf/lvinkmeasurementdrawbuf.cpp
        src/lvdrawbuf/lvimagescaleddrawcallback.cpp
        src/lvdrawbuf/lvdrawbuf_utils.cpp
        #src/xutils.cpp
    )
endif (NOT ${GUI} STREQUAL FB2PROPS)

ADD_LIBRARY(crengine STATIC ${CRENGINE_SOURCES})

if(BUILD_TOOLS)
    add_subdirectory(Tools)
endif(BUILD_TOOLS)
