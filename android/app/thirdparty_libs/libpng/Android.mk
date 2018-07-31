
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_png

PNG_SRC_DIR := ../../../../thirdparty/libpng
PNG_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/libpng
PNG_CONFIG_DIR_P := $(LOCAL_PATH)

LOCAL_C_INCLUDES := $(PNG_CONFIG_DIR_P) $(PNG_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(PNG_SRC_DIR)/png.c \
    $(PNG_SRC_DIR)/pngerror.c \
    $(PNG_SRC_DIR)/pngget.c \
    $(PNG_SRC_DIR)/pngmem.c \
    $(PNG_SRC_DIR)/pngpread.c \
    $(PNG_SRC_DIR)/pngread.c \
    $(PNG_SRC_DIR)/pngrio.c \
    $(PNG_SRC_DIR)/pngrtran.c \
    $(PNG_SRC_DIR)/pngrutil.c \
    $(PNG_SRC_DIR)/pngset.c \
    $(PNG_SRC_DIR)/pngtrans.c \
    $(PNG_SRC_DIR)/pngwio.c \
    $(PNG_SRC_DIR)/pngwrite.c \
    $(PNG_SRC_DIR)/pngwtran.c \
    $(PNG_SRC_DIR)/pngwutil.c

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_SRC_FILES += \
    $(PNG_SRC_DIR)/arm/arm_init.c \
    $(PNG_SRC_DIR)/arm/filter_neon.S \
    $(PNG_SRC_DIR)/arm/filter_neon_intrinsics.c
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES += \
    $(PNG_SRC_DIR)/arm/arm_init.c \
    $(PNG_SRC_DIR)/arm/filter_neon.S \
    $(PNG_SRC_DIR)/arm/filter_neon_intrinsics.c
endif

ifeq ($(TARGET_ARCH_ABI),x86_64)
LOCAL_SRC_FILES += \
    $(PNG_SRC_DIR)/intel/intel_init.c \
    $(PNG_SRC_DIR)/intel/filter_sse2_intrinsics.c
endif

include $(BUILD_STATIC_LIBRARY)
