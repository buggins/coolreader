
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := cr3engine-3-2-X

# Generate CREngine blob with statically linked libjpeg, libpng, freetype, harfbuzz, chmlib

CRFLAGS := -DLINUX=1 -D_LINUX=1 -DFOR_ANDROID=1 -DCR3_PATCH \
     -DFT2_BUILD_LIBRARY=1 -DFT_CONFIG_MODULES_H=\<builds/android/include/config/ftmodule.h\> -DFT_CONFIG_OPTIONS_H=\<builds/android/include/config/ftoption.h\> \
     -DDOC_DATA_COMPRESSION_LEVEL=1 -DDOC_BUFFER_SIZE=0x1000000 \
     -DENABLE_CACHE_FILE_CONTENTS_VALIDATION=1 \
     -DLDOM_USE_OWN_MEM_MAN=0 \
     -DCR3_ANTIWORD_PATCH=1 -DENABLE_ANTIWORD=1 \
     -DMAX_IMAGE_SCALE_MUL=2

CR3_ROOT := $(LOCAL_PATH)/../..

LOCAL_C_INCLUDES := \
    $(CR3_ROOT)/crengine/include \
    $(CR3_ROOT)/thirdparty/libpng \
    $(CR3_ROOT)/thirdparty/freetype/include \
    $(CR3_ROOT)/thirdparty/freetype \
    $(CR3_ROOT)/thirdparty/harfbuzz/src \
    $(CR3_ROOT)/thirdparty/libjpeg \
    $(CR3_ROOT)/thirdparty/antiword \
    $(CR3_ROOT)/thirdparty/chmlib/src


LOCAL_CFLAGS += $(CRFLAGS)

LOCAL_CFLAGS += -Wall -Wno-unused-variable -Wno-sign-compare -Wno-write-strings -Wno-main -Wno-unused-function

LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_CFLAGS += -g -O1 -fexceptions

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

JNI_SRC_FILES := \
    cr3engine.cpp \
    cr3java.cpp \
    docview.cpp

COFFEECATCH_SRC_FILES := \
    coffeecatch/coffeecatch.c \
    coffeecatch/coffeejni.c

LOCAL_SRC_FILES := \
    $(JNI_SRC_FILES) \
    $(CRENGINE_SRC_FILES)

ifeq ($(TARGET_ARCH_ABI),armeabi)
LOCAL_SRC_FILES += \
    $(COFFEECATCH_SRC_FILES)
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES += \
    $(COFFEECATCH_SRC_FILES)
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES += \
    $(COFFEECATCH_SRC_FILES)
endif
ifeq ($(TARGET_ARCH_ABI),mips)
LOCAL_SRC_FILES += \
    $(COFFEECATCH_SRC_FILES)
endif
ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_SRC_FILES += \
    $(COFFEECATCH_SRC_FILES)
endif

LOCAL_STATIC_LIBRARIES := \
    local_png \
    local_jpeg \
    local_freetype \
    local_harfbuzz \
    local_chmlib \
    local_antiword

LOCAL_LDLIBS    := -lm -llog -lz -ldl
# 
#LOCAL_LDLIBS    += -Wl,-Map=cr3engine.map
#-ljnigraphics

include $(BUILD_SHARED_LIBRARY)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(MY_LOCAL_PATH)/../app/thirdparty_libs/Android.mk
