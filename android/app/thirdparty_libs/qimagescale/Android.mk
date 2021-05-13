
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_qimagescale

QIMAGESCALE_SRC_DIR := ../../../../thirdparty_unman/qimagescale
QIMAGESCALE_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty_unman/qimagescale

LOCAL_C_INCLUDES := $(QIMAGESCALE_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(QIMAGESCALE_SRC_DIR)/qimagescale.cpp

include $(BUILD_STATIC_LIBRARY)
