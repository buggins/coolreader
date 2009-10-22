# -------------------------------------------------
# Project created by QtCreator 2009-05-12T11:47:00
# -------------------------------------------------
TARGET = cr3
TEMPLATE = app
DEFINES += USE_FREETYPE=1 \
    LDOM_USE_OWN_MEM_MAN=1 \
    COLOR_BACKBUFFER=1 \
    USE_DOM_UTF8_STORAGE=1
win32 { 
    DEFINES += _WIN32=1 \
        WIN32=1 \
        CR_EMULATE_GETTEXT=1
    LIBS += -lgdi32
}
!win32 { 
    DEFINES += _LINUX=1 \
        LINUX=1
    INCLUDEPATH += /usr/include/freetype2
}
debug:DEFINES += _DEBUG=1
SOURCES += main.cpp \
    mainwindow.cpp \
    ../crengine/src/cp_stats.cpp \
    ../crengine/src/wolutil.cpp \
    ../crengine/src/rtfimp.cpp \
    ../crengine/src/props.cpp \
    ../crengine/src/lvxml.cpp \
    ../crengine/src/lvtinydom.cpp \
    ../crengine/src/lvtextfm.cpp \
    ../crengine/src/lvstyles.cpp \
    ../crengine/src/lvstsheet.cpp \
    ../crengine/src/lvstring.cpp \
    ../crengine/src/lvstream.cpp \
    ../crengine/src/lvrend.cpp \
    ../crengine/src/lvpagesplitter.cpp \
    ../crengine/src/lvmemman.cpp \
    ../crengine/src/lvimg.cpp \
    ../crengine/src/lvfntman.cpp \
    ../crengine/src/lvfnt.cpp \
    ../crengine/src/lvdrawbuf.cpp \
    ../crengine/src/lvdocview.cpp \
    ../crengine/src/lvbmpbuf.cpp \
    ../crengine/src/lstridmap.cpp \
    ../crengine/src/hyphman.cpp \
    ../crengine/src/hist.cpp \
    ../crengine/src/crtxtenc.cpp \
    ../crengine/src/crskin.cpp \
    ../crengine/src/cri18n.cpp \
    ../crengine/src/crgui.cpp \
    cr3widget.cpp \
    crqtutil.cpp \
    tocdlg.cpp \
    recentdlg.cpp \
    settings.cpp \
    qtc-gdbmacros/gdbmacros.cpp \
    aboutdlg.cpp \
    filepropsdlg.cpp \
    addbookmarkdlg.cpp \
    bookmarklistdlg.cpp
HEADERS += mainwindow.h \
    ../crengine/include/rtfimp.h \
    ../crengine/include/rtfcmd.h \
    ../crengine/include/props.h \
    ../crengine/include/lvxml.h \
    ../crengine/include/lvtypes.h \
    ../crengine/include/lvtinydom.h \
    ../crengine/include/lvthread.h \
    ../crengine/include/lvtextfm.h \
    ../crengine/include/lvstyles.h \
    ../crengine/include/lvstsheet.h \
    ../crengine/include/lvstring.h \
    ../crengine/include/lvstream.h \
    ../crengine/include/lvrend.h \
    ../crengine/include/lvref.h \
    ../crengine/include/lvrefcache.h \
    ../crengine/include/lvptrvec.h \
    ../crengine/include/lvpagesplitter.h \
    ../crengine/include/lvmemman.h \
    ../crengine/include/lvimg.h \
    ../crengine/include/lvhashtable.h \
    ../crengine/include/lvfntman.h \
    ../crengine/include/lvfnt.h \
    ../crengine/include/lvdrawbuf.h \
    ../crengine/include/lvdocview.h \
    ../crengine/include/lvbmpbuf.h \
    ../crengine/include/lvarray.h \
    ../crengine/include/lstridmap.h \
    ../crengine/include/hyphman.h \
    ../crengine/include/hist.h \
    ../crengine/include/fb2def.h \
    ../crengine/include/dtddef.h \
    ../crengine/include/cssdef.h \
    ../crengine/include/crtxtenc.h \
    ../crengine/include/crtrace.h \
    ../crengine/include/crskin.h \
    ../crengine/include/crsetup.h \
    ../crengine/include/cri18n.h \
    ../crengine/include/crgui.h \
    ../crengine/include/crengine.h \
    ../crengine/include/cp_stats.h \
    ../crengine/include/wolutil.h \
    cr3widget.h \
    crqtutil.h \
    tocdlg.h \
    recentdlg.h \
    settings.h \
    aboutdlg.h \
    filepropsdlg.h \
    addbookmarkdlg.h \
    bookmarklistdlg.h
FORMS += mainwindow.ui \
    tocdlg.ui \
    tocdlg.ui \
    recentdlg.ui \
    settings.ui \
    aboutdlg.ui \
    filepropsdlg.ui \
    addbookmarkdlg.ui \
    bookmarklistdlg.ui
TRANSLATIONS += i18n/cr3_ru.ts \
    i18n/cr3_uk.ts \
    i18n/cr3_de.ts

RESOURCES += cr3res.qrc
!win32 { 
    unix:LIBS += -ljpeg
    win32:LIBS += libjpeg.lib
}
win32 { 
    INCLUDEPATH += ../thirdparty/libjpeg
    SOURCES += ../thirdparty/libjpeg/jcapimin.c \
        ../thirdparty/libjpeg/jcapistd.c \
        ../thirdparty/libjpeg/jccoefct.c \
        ../thirdparty/libjpeg/jccolor.c \
        ../thirdparty/libjpeg/jcdctmgr.c \
        ../thirdparty/libjpeg/jchuff.c \
        ../thirdparty/libjpeg/jcinit.c \
        ../thirdparty/libjpeg/jcmainct.c \
        ../thirdparty/libjpeg/jcmarker.c \
        ../thirdparty/libjpeg/jcmaster.c \
        ../thirdparty/libjpeg/jcomapi.c \
        ../thirdparty/libjpeg/jcparam.c \
        ../thirdparty/libjpeg/jcphuff.c \
        ../thirdparty/libjpeg/jcprepct.c \
        ../thirdparty/libjpeg/jcsample.c \
        ../thirdparty/libjpeg/jctrans.c \
        ../thirdparty/libjpeg/jdapimin.c \
        ../thirdparty/libjpeg/jdapistd.c \
        ../thirdparty/libjpeg/jdatadst.c \
        ../thirdparty/libjpeg/jdatasrc.c \
        ../thirdparty/libjpeg/jdcoefct.c \
        ../thirdparty/libjpeg/jdcolor.c \
        ../thirdparty/libjpeg/jddctmgr.c \
        ../thirdparty/libjpeg/jdhuff.c \
        ../thirdparty/libjpeg/jdinput.c \
        ../thirdparty/libjpeg/jdmainct.c \
        ../thirdparty/libjpeg/jdmarker.c \
        ../thirdparty/libjpeg/jdmaster.c \
        ../thirdparty/libjpeg/jdmerge.c \
        ../thirdparty/libjpeg/jdphuff.c \
        ../thirdparty/libjpeg/jdpostct.c \
        ../thirdparty/libjpeg/jdsample.c \
        ../thirdparty/libjpeg/jdtrans.c \
        ../thirdparty/libjpeg/jerror.c \
        ../thirdparty/libjpeg/jfdctflt.c \
        ../thirdparty/libjpeg/jfdctfst.c \
        ../thirdparty/libjpeg/jfdctint.c \
        ../thirdparty/libjpeg/jidctflt.c \
        ../thirdparty/libjpeg/jidctfst.c \
        ../thirdparty/libjpeg/jidctint.c \
        ../thirdparty/libjpeg/jidctred.c \
        ../thirdparty/libjpeg/jmemmgr.c \
        ../thirdparty/libjpeg/jquant1.c \
        ../thirdparty/libjpeg/jquant2.c \
        ../thirdparty/libjpeg/jutils.c \
        ../thirdparty/libjpeg/jmemnobs.c
}
!win32 { 
    unix:LIBS += -lpng
    win32:LIBS += libpng.lib
}
win32 { 
    INCLUDEPATH += ../thirdparty/libpng
    SOURCES += ../thirdparty/libpng/png.c \
        ../thirdparty/libpng/pngset.c \
        ../thirdparty/libpng/pngget.c \
        ../thirdparty/libpng/pngrutil.c \
        ../thirdparty/libpng/pngtrans.c \
        ../thirdparty/libpng/pngwutil.c \
        ../thirdparty/libpng/pngread.c \
        ../thirdparty/libpng/pngrio.c \
        ../thirdparty/libpng/pngwio.c \
        ../thirdparty/libpng/pngwrite.c \
        ../thirdparty/libpng/pngrtran.c \
        ../thirdparty/libpng/pngwtran.c \
        ../thirdparty/libpng/pngmem.c \
        ../thirdparty/libpng/pngerror.c \
        ../thirdparty/libpng/pngpread.c
}
!win32 { 
    unix:LIBS += -lfreetype
    win32:LIBS += libfreetype.lib
}
win32 { 
    DEFINES += FT2_BUILD_LIBRARY=1
    INCLUDEPATH += ../thirdparty/freetype/include
    SOURCES += ../thirdparty/freetype/src/autofit/autofit.c \
        ../thirdparty/freetype/src/bdf/bdf.c \
        ../thirdparty/freetype/src/cff/cff.c \
        ../thirdparty/freetype/src/base/ftbase.c \
        ../thirdparty/freetype/src/base/ftbbox.c \
        ../thirdparty/freetype/src/base/ftbdf.c \
        ../thirdparty/freetype/src/base/ftbitmap.c \
        ../thirdparty/freetype/src/base/ftgasp.c \
        ../thirdparty/freetype/src/cache/ftcache.c \
        ../thirdparty/freetype/builds/win32/ftdebug.c \
        ../thirdparty/freetype/src/base/ftglyph.c \
        ../thirdparty/freetype/src/base/ftgxval.c \
        ../thirdparty/freetype/src/gzip/ftgzip.c \
        ../thirdparty/freetype/src/base/ftinit.c \
        ../thirdparty/freetype/src/lzw/ftlzw.c \
        ../thirdparty/freetype/src/base/ftmm.c \
        ../thirdparty/freetype/src/base/ftotval.c \
        ../thirdparty/freetype/src/base/ftpfr.c \
        ../thirdparty/freetype/src/base/ftstroke.c \
        ../thirdparty/freetype/src/base/ftsynth.c \
        ../thirdparty/freetype/src/base/ftsystem.c \
        ../thirdparty/freetype/src/base/fttype1.c \
        ../thirdparty/freetype/src/base/ftwinfnt.c \
        ../thirdparty/freetype/src/base/ftxf86.c \
        ../thirdparty/freetype/src/pcf/pcf.c \
        ../thirdparty/freetype/src/pfr/pfr.c \
        ../thirdparty/freetype/src/psaux/psaux.c \
        ../thirdparty/freetype/src/pshinter/pshinter.c \
        ../thirdparty/freetype/src/psnames/psmodule.c \
        ../thirdparty/freetype/src/raster/raster.c \
        ../thirdparty/freetype/src/sfnt/sfnt.c \
        ../thirdparty/freetype/src/smooth/smooth.c \
        ../thirdparty/freetype/src/truetype/truetype.c \
        ../thirdparty/freetype/src/type1/type1.c \
        ../thirdparty/freetype/src/cid/type1cid.c \
        ../thirdparty/freetype/src/type42/type42.c \
        ../thirdparty/freetype/src/winfonts/winfnt.c
}
!win32 { 
    unix:LIBS += -lz
    win32:LIBS += libz.lib
}
win32 { 
    INCLUDEPATH += ../thirdparty/zlib
    SOURCES += ../thirdparty/zlib/adler32.c \
        ../thirdparty/zlib/compress.c \
        ../thirdparty/zlib/crc32.c \
        ../thirdparty/zlib/gzio.c \
        ../thirdparty/zlib/uncompr.c \
        ../thirdparty/zlib/deflate.c \
        ../thirdparty/zlib/trees.c \
        ../thirdparty/zlib/zutil.c \
        ../thirdparty/zlib/inflate.c \
        ../thirdparty/zlib/infback.c \
        ../thirdparty/zlib/inftrees.c \
        ../thirdparty/zlib/inffast.c
}
