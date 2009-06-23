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
    INCLUDEPATH += ../../freetype2/include \
        ../../zlib \
        ../../libjpeg \
        ../../libpng
    debug:LIBS += ../../freetype2/objs/freetype235MT_D.lib \
        ../../zlib/lib/zlibd.lib \
        ../../libpng/lib/libpngd.lib \
        ../../libjpeg/lib/libjpegd.lib
    else:LIBS += ../../freetype2/objs/freetype235MT.lib \
        ../../libpng/lib/libpng.lib \
        ../../zlib/lib/zlib.lib \
        ../../libjpeg/lib/libjpeg.lib
}
!win32 { 
    DEFINES += _LINUX=1 \
        LINUX=1
    INCLUDEPATH += /usr/include/freetype2
    LIBS += -ljpeg \
        -lfreetype
}
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
    recentdlg.cpp
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
    recentdlg.h
FORMS += mainwindow.ui \
    tocdlg.ui \
    tocdlg.ui \
    recentdlg.ui
