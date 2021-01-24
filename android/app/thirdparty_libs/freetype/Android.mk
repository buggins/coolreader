
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_freetype

FREETYPE_SRC_DIR := ../../../../thirdparty/freetype-2.10.4
FREETYPE_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/freetype-2.10.4
HARFBUZZ_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/harfbuzz-2.7.4
FREETYPE_CONFIG_DIR_P := $(LOCAL_PATH)
PNG_PRIV_CONFIG_DIR_P := $(LOCAL_PATH)/../libpng/lib
PNG_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/libpng-1.6.37

LOCAL_C_INCLUDES := \
        $(FREETYPE_CONFIG_DIR_P) \
        $(FREETYPE_SRC_DIR_P) \
        $(FREETYPE_SRC_DIR_P)/include \
        $(HARFBUZZ_SRC_DIR_P)/src \
        $(PNG_SRC_DIR_P) \
        $(PNG_PRIV_CONFIG_DIR_P)

LOCAL_CFLAGS += -DFT2_BUILD_LIBRARY=1 -DFT_CONFIG_MODULES_H=\<android/config/ftmodule.h\> -DFT_CONFIG_OPTIONS_H=\<android/config/ftoption.h\>
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(FREETYPE_SRC_DIR)/src/autofit/autofit.c \
    $(FREETYPE_SRC_DIR)/src/base/ftbase.c \
    $(FREETYPE_SRC_DIR)/src/base/ftdebug.c \
    $(FREETYPE_SRC_DIR)/src/base/ftinit.c \
    $(FREETYPE_SRC_DIR)/src/base/ftfntfmt.c \
    $(FREETYPE_SRC_DIR)/src/base/ftsystem.c \
    $(FREETYPE_SRC_DIR)/src/base/ftglyph.c \
    $(FREETYPE_SRC_DIR)/src/base/ftbitmap.c \
    $(FREETYPE_SRC_DIR)/src/base/ftlcdfil.c \
    $(FREETYPE_SRC_DIR)/src/base/ftsynth.c \
    $(FREETYPE_SRC_DIR)/src/bdf/bdf.c \
    $(FREETYPE_SRC_DIR)/src/cache/ftcache.c \
    $(FREETYPE_SRC_DIR)/src/cff/cff.c \
    $(FREETYPE_SRC_DIR)/src/cid/type1cid.c \
    $(FREETYPE_SRC_DIR)/src/gzip/ftgzip.c \
    $(FREETYPE_SRC_DIR)/src/lzw/ftlzw.c \
    $(FREETYPE_SRC_DIR)/src/pcf/pcf.c \
    $(FREETYPE_SRC_DIR)/src/pfr/pfr.c \
    $(FREETYPE_SRC_DIR)/src/psaux/psaux.c \
    $(FREETYPE_SRC_DIR)/src/pshinter/pshinter.c \
    $(FREETYPE_SRC_DIR)/src/psnames/psnames.c \
    $(FREETYPE_SRC_DIR)/src/raster/raster.c \
    $(FREETYPE_SRC_DIR)/src/sfnt/sfnt.c \
    $(FREETYPE_SRC_DIR)/src/smooth/smooth.c \
    $(FREETYPE_SRC_DIR)/src/truetype/truetype.c \
    $(FREETYPE_SRC_DIR)/src/type1/type1.c \
    $(FREETYPE_SRC_DIR)/src/type42/type42.c \
    $(FREETYPE_SRC_DIR)/src/winfonts/winfnt.c
#   $(FREETYPE_SRC_DIR)/src/gxvalid/gxvalid.c
#   $(FREETYPE_SRC_DIR)/src/otvalid/otvalid.c

include $(BUILD_STATIC_LIBRARY)
