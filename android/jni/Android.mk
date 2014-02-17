#include ../../Android.mk
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := cr3engine-3-1-1

# Generate CREngine blob with statically linked libjpeg, libpng, libfreetype, chmlib
# TODO: build libraries using separate makefiles

CRFLAGS = -DLINUX=1 -D_LINUX=1 -DFOR_ANDROID=1 -DCR3_PATCH -DFT2_BUILD_LIBRARY=1 \
     -DDOC_DATA_COMPRESSION_LEVEL=1 -DDOC_BUFFER_SIZE=0xA00000 \
     -DENABLE_CACHE_FILE_CONTENTS_VALIDATION=1 \
     -DLDOM_USE_OWN_MEM_MAN=0 \
     -DCR3_ANTIWORD_PATCH=1 -DENABLE_ANTIWORD=1 \
     -DMAX_IMAGE_SCALE_MUL=2

CR3_ROOT = $(LOCAL_PATH)/../..

LOCAL_C_INCLUDES := \
    -I $(CR3_ROOT)/crengine/include \
    -I $(CR3_ROOT)/thirdparty/libpng \
    -I $(CR3_ROOT)/thirdparty/freetype/include \
    -I $(CR3_ROOT)/thirdparty/libjpeg \
    -I $(CR3_ROOT)/thirdparty/antiword \
    -I $(CR3_ROOT)/thirdparty/chmlib/src


LOCAL_CFLAGS += $(CRFLAGS) $(CRENGINE_INCLUDES) -Wno-psabi -Wno-unused-variable -Wno-sign-compare -Wno-write-strings -Wno-main -Wno-unused-but-set-variable -Wno-unused-function -Wall


CRENGINE_SRC_FILES := \
    ../../crengine/src/cp_stats.cpp \
    ../../crengine/src/lvstring.cpp \
    ../../crengine/src/props.cpp \
    ../../crengine/src/lstridmap.cpp \
    ../../crengine/src/rtfimp.cpp \
    ../../crengine/src/lvmemman.cpp \
    ../../crengine/src/lvstyles.cpp \
    ../../crengine/src/crtxtenc.cpp \
    ../../crengine/src/lvtinydom.cpp \
    ../../crengine/src/lvstream.cpp \
    ../../crengine/src/lvxml.cpp \
    ../../crengine/src/chmfmt.cpp \
    ../../crengine/src/epubfmt.cpp \
    ../../crengine/src/pdbfmt.cpp \
    ../../crengine/src/wordfmt.cpp \
    ../../crengine/src/lvstsheet.cpp \
    ../../crengine/src/txtselector.cpp \
    ../../crengine/src/crtest.cpp \
    ../../crengine/src/lvbmpbuf.cpp \
    ../../crengine/src/lvfnt.cpp \
    ../../crengine/src/hyphman.cpp \
    ../../crengine/src/lvfntman.cpp \
    ../../crengine/src/lvimg.cpp \
    ../../crengine/src/crskin.cpp \
    ../../crengine/src/lvdrawbuf.cpp \
    ../../crengine/src/lvdocview.cpp \
    ../../crengine/src/lvpagesplitter.cpp \
    ../../crengine/src/lvtextfm.cpp \
    ../../crengine/src/lvrend.cpp \
    ../../crengine/src/wolutil.cpp \
    ../../crengine/src/crconcurrent.cpp \
    ../../crengine/src/hist.cpp
#    ../../crengine/src/cri18n.cpp
#    ../../crengine/src/crgui.cpp \

PNG_SRC_FILES := \
    ../../thirdparty/libpng/pngerror.c  \
    ../../thirdparty/libpng/pngget.c  \
    ../../thirdparty/libpng/pngpread.c \
    ../../thirdparty/libpng/pngrio.c \
    ../../thirdparty/libpng/pngrutil.c \
    ../../thirdparty/libpng/pngvcrd.c \
    ../../thirdparty/libpng/png.c \
    ../../thirdparty/libpng/pngwrite.c \
    ../../thirdparty/libpng/pngwutil.c \
    ../../thirdparty/libpng/pnggccrd.c \
    ../../thirdparty/libpng/pngmem.c \
    ../../thirdparty/libpng/pngread.c \
    ../../thirdparty/libpng/pngrtran.c \
    ../../thirdparty/libpng/pngset.c \
    ../../thirdparty/libpng/pngtrans.c \
    ../../thirdparty/libpng/pngwio.c \
    ../../thirdparty/libpng/pngwtran.c

JPEG_SRC_FILES := \
    ../../thirdparty/libjpeg/jcapimin.c \
    ../../thirdparty/libjpeg/jchuff.c \
    ../../thirdparty/libjpeg/jcomapi.c \
    ../../thirdparty/libjpeg/jctrans.c \
    ../../thirdparty/libjpeg/jdcoefct.c \
    ../../thirdparty/libjpeg/jdmainct.c \
    ../../thirdparty/libjpeg/jdpostct.c \
    ../../thirdparty/libjpeg/jfdctfst.c \
    ../../thirdparty/libjpeg/jidctred.c \
    ../../thirdparty/libjpeg/jutils.c \
    ../../thirdparty/libjpeg/jcapistd.c \
    ../../thirdparty/libjpeg/jcinit.c \
    ../../thirdparty/libjpeg/jcparam.c \
    ../../thirdparty/libjpeg/jdapimin.c \
    ../../thirdparty/libjpeg/jdcolor.c \
    ../../thirdparty/libjpeg/jdmarker.c \
    ../../thirdparty/libjpeg/jdsample.c \
    ../../thirdparty/libjpeg/jfdctint.c \
    ../../thirdparty/libjpeg/jmemmgr.c \
    ../../thirdparty/libjpeg/jccoefct.c \
    ../../thirdparty/libjpeg/jcmainct.c \
    ../../thirdparty/libjpeg/jcphuff.c \
    ../../thirdparty/libjpeg/jdapistd.c \
    ../../thirdparty/libjpeg/jddctmgr.c \
    ../../thirdparty/libjpeg/jdmaster.c \
    ../../thirdparty/libjpeg/jdtrans.c \
    ../../thirdparty/libjpeg/jidctflt.c \
    ../../thirdparty/libjpeg/jmemnobs.c \
    ../../thirdparty/libjpeg/jccolor.c \
    ../../thirdparty/libjpeg/jcmarker.c \
    ../../thirdparty/libjpeg/jcprepct.c \
    ../../thirdparty/libjpeg/jdatadst.c \
    ../../thirdparty/libjpeg/jdhuff.c \
    ../../thirdparty/libjpeg/jdmerge.c \
    ../../thirdparty/libjpeg/jerror.c \
    ../../thirdparty/libjpeg/jidctfst.c \
    ../../thirdparty/libjpeg/jquant1.c \
    ../../thirdparty/libjpeg/jcdctmgr.c \
    ../../thirdparty/libjpeg/jcmaster.c \
    ../../thirdparty/libjpeg/jcsample.c \
    ../../thirdparty/libjpeg/jdatasrc.c \
    ../../thirdparty/libjpeg/jdinput.c \
    ../../thirdparty/libjpeg/jdphuff.c \
    ../../thirdparty/libjpeg/jfdctflt.c \
    ../../thirdparty/libjpeg/jidctint.c \
    ../../thirdparty/libjpeg/jquant2.c

FREETYPE_SRC_FILES := \
    ../../thirdparty/freetype/src/autofit/autofit.c \
    ../../thirdparty/freetype/src/bdf/bdf.c \
    ../../thirdparty/freetype/src/cff/cff.c \
    ../../thirdparty/freetype/src/base/ftbase.c \
    ../../thirdparty/freetype/src/base/ftbbox.c \
    ../../thirdparty/freetype/src/base/ftbdf.c \
    ../../thirdparty/freetype/src/base/ftbitmap.c \
    ../../thirdparty/freetype/src/base/ftgasp.c \
    ../../thirdparty/freetype/src/cache/ftcache.c \
    ../../thirdparty/freetype/src/base/ftglyph.c \
    ../../thirdparty/freetype/src/base/ftgxval.c \
    ../../thirdparty/freetype/src/gzip/ftgzip.c \
    ../../thirdparty/freetype/src/base/ftinit.c \
    ../../thirdparty/freetype/src/lzw/ftlzw.c \
    ../../thirdparty/freetype/src/base/ftmm.c \
    ../../thirdparty/freetype/src/base/ftpatent.c \
    ../../thirdparty/freetype/src/base/ftotval.c \
    ../../thirdparty/freetype/src/base/ftpfr.c \
    ../../thirdparty/freetype/src/base/ftstroke.c \
    ../../thirdparty/freetype/src/base/ftsynth.c \
    ../../thirdparty/freetype/src/base/ftsystem.c \
    ../../thirdparty/freetype/src/base/fttype1.c \
    ../../thirdparty/freetype/src/base/ftwinfnt.c \
    ../../thirdparty/freetype/src/base/ftxf86.c \
    ../../thirdparty/freetype/src/winfonts/winfnt.c \
    ../../thirdparty/freetype/src/pcf/pcf.c \
    ../../thirdparty/freetype/src/pfr/pfr.c \
    ../../thirdparty/freetype/src/psaux/psaux.c \
    ../../thirdparty/freetype/src/pshinter/pshinter.c \
    ../../thirdparty/freetype/src/psnames/psmodule.c \
    ../../thirdparty/freetype/src/raster/raster.c \
    ../../thirdparty/freetype/src/sfnt/sfnt.c \
    ../../thirdparty/freetype/src/smooth/smooth.c \
    ../../thirdparty/freetype/src/truetype/truetype.c \
    ../../thirdparty/freetype/src/type1/type1.c \
    ../../thirdparty/freetype/src/cid/type1cid.c \
    ../../thirdparty/freetype/src/type42/type42.c

CHM_SRC_FILES := \
    ../../thirdparty/chmlib/src/chm_lib.c \
    ../../thirdparty/chmlib/src/lzx.c 

ANTIWORD_SRC_FILES := \
    ../../thirdparty/antiword/asc85enc.c \
    ../../thirdparty/antiword/blocklist.c \
    ../../thirdparty/antiword/chartrans.c \
    ../../thirdparty/antiword/datalist.c \
    ../../thirdparty/antiword/depot.c \
    ../../thirdparty/antiword/doclist.c \
    ../../thirdparty/antiword/fail.c \
    ../../thirdparty/antiword/finddata.c \
    ../../thirdparty/antiword/findtext.c \
    ../../thirdparty/antiword/fontlist.c \
    ../../thirdparty/antiword/fonts.c \
    ../../thirdparty/antiword/fonts_u.c \
    ../../thirdparty/antiword/hdrftrlist.c \
    ../../thirdparty/antiword/imgexam.c \
    ../../thirdparty/antiword/listlist.c \
    ../../thirdparty/antiword/misc.c \
    ../../thirdparty/antiword/notes.c \
    ../../thirdparty/antiword/options.c \
    ../../thirdparty/antiword/out2window.c \
    ../../thirdparty/antiword/pdf.c \
    ../../thirdparty/antiword/pictlist.c \
    ../../thirdparty/antiword/prop0.c \
    ../../thirdparty/antiword/prop2.c \
    ../../thirdparty/antiword/prop6.c \
    ../../thirdparty/antiword/prop8.c \
    ../../thirdparty/antiword/properties.c \
    ../../thirdparty/antiword/propmod.c \
    ../../thirdparty/antiword/rowlist.c \
    ../../thirdparty/antiword/sectlist.c \
    ../../thirdparty/antiword/stylelist.c \
    ../../thirdparty/antiword/stylesheet.c \
    ../../thirdparty/antiword/summary.c \
    ../../thirdparty/antiword/tabstop.c \
    ../../thirdparty/antiword/unix.c \
    ../../thirdparty/antiword/utf8.c \
    ../../thirdparty/antiword/word2text.c \
    ../../thirdparty/antiword/worddos.c \
    ../../thirdparty/antiword/wordlib.c \
    ../../thirdparty/antiword/wordmac.c \
    ../../thirdparty/antiword/wordole.c \
    ../../thirdparty/antiword/wordwin.c \
    ../../thirdparty/antiword/xmalloc.c

JNI_SRC_FILES := \
    cr3engine.cpp \
    cr3java.cpp \
    docview.cpp

LOCAL_SRC_FILES := \
    $(JNI_SRC_FILES) \
    $(CRENGINE_SRC_FILES) \
    $(FREETYPE_SRC_FILES) \
    $(PNG_SRC_FILES) \
    $(JPEG_SRC_FILES) \
    $(CHM_SRC_FILES) \
    $(ANTIWORD_SRC_FILES)

LOCAL_LDLIBS    := -lm -llog -lz -ldl -Wl,-Map=cr3engine.map
#-ljnigraphics

include $(BUILD_SHARED_LIBRARY)

