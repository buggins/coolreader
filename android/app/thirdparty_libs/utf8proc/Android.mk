
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_utf8proc

UTF8PROC_SRC_DIR := ../../../../thirdparty/$(REPO_UTF8PROC_SRCDIR)
UTF8PROC_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/$(REPO_UTF8PROC_SRCDIR)

LOCAL_C_INCLUDES := $(UTF8PROC_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(UTF8PROC_SRC_DIR)/utf8proc.c

include $(BUILD_STATIC_LIBRARY)
