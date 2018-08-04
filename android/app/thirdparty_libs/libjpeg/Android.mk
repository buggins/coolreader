
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_jpeg

JPEG_SRC_DIR := ../../../../thirdparty/libjpeg
JPEG_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/libjpeg

LOCAL_C_INCLUDES := $(JPEG_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(JPEG_SRC_DIR)/jaricom.c \
    $(JPEG_SRC_DIR)/jcapimin.c \
    $(JPEG_SRC_DIR)/jcapistd.c \
    $(JPEG_SRC_DIR)/jcarith.c \
    $(JPEG_SRC_DIR)/jccoefct.c \
    $(JPEG_SRC_DIR)/jccolor.c \
    $(JPEG_SRC_DIR)/jcdctmgr.c \
    $(JPEG_SRC_DIR)/jchuff.c \
    $(JPEG_SRC_DIR)/jcinit.c \
    $(JPEG_SRC_DIR)/jcmainct.c \
    $(JPEG_SRC_DIR)/jcmarker.c \
    $(JPEG_SRC_DIR)/jcmaster.c \
    $(JPEG_SRC_DIR)/jcomapi.c \
    $(JPEG_SRC_DIR)/jcparam.c \
    $(JPEG_SRC_DIR)/jcprepct.c \
    $(JPEG_SRC_DIR)/jcsample.c \
    $(JPEG_SRC_DIR)/jctrans.c \
    $(JPEG_SRC_DIR)/jdapimin.c \
    $(JPEG_SRC_DIR)/jdapistd.c \
    $(JPEG_SRC_DIR)/jdarith.c \
    $(JPEG_SRC_DIR)/jdatadst.c \
    $(JPEG_SRC_DIR)/jdatasrc.c \
    $(JPEG_SRC_DIR)/jdcoefct.c \
    $(JPEG_SRC_DIR)/jdcolor.c \
    $(JPEG_SRC_DIR)/jddctmgr.c \
    $(JPEG_SRC_DIR)/jdhuff.c \
    $(JPEG_SRC_DIR)/jdinput.c \
    $(JPEG_SRC_DIR)/jdmainct.c \
    $(JPEG_SRC_DIR)/jdmarker.c \
    $(JPEG_SRC_DIR)/jdmaster.c \
    $(JPEG_SRC_DIR)/jdmerge.c \
    $(JPEG_SRC_DIR)/jdpostct.c \
    $(JPEG_SRC_DIR)/jdsample.c \
    $(JPEG_SRC_DIR)/jdtrans.c \
    $(JPEG_SRC_DIR)/jerror.c \
    $(JPEG_SRC_DIR)/jfdctflt.c \
    $(JPEG_SRC_DIR)/jfdctfst.c \
    $(JPEG_SRC_DIR)/jfdctint.c \
    $(JPEG_SRC_DIR)/jidctflt.c \
    $(JPEG_SRC_DIR)/jidctfst.c \
    $(JPEG_SRC_DIR)/jidctint.c \
    $(JPEG_SRC_DIR)/jquant1.c \
    $(JPEG_SRC_DIR)/jquant2.c \
    $(JPEG_SRC_DIR)/jutils.c \
    $(JPEG_SRC_DIR)/jmemmgr.c \
    $(JPEG_SRC_DIR)/jmemnobs.c

include $(BUILD_STATIC_LIBRARY)
