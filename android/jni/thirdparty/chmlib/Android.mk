
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_chmlib

CHM_SRC_DIR := ../../../../thirdparty/chmlib
CHM_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/chmlib

LOCAL_C_INCLUDES := $(CHM_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(CHM_SRC_DIR)/src/chm_lib.c \
    $(CHM_SRC_DIR)/src/lzx.c

include $(BUILD_STATIC_LIBRARY)
