
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_libunibreak

LIBUNIBREAK_SRC_DIR := ../../../../thirdparty/libunibreak/src
LIBUNIBREAK_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/libunibreak/src

LOCAL_C_INCLUDES := $(LIBUNIBREAK_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(LIBUNIBREAK_SRC_DIR)/unibreakbase.c \
    $(LIBUNIBREAK_SRC_DIR)/unibreakdef.c \
    $(LIBUNIBREAK_SRC_DIR)/linebreak.c \
    $(LIBUNIBREAK_SRC_DIR)/linebreakdata.c \
    $(LIBUNIBREAK_SRC_DIR)/linebreakdef.c \
    $(LIBUNIBREAK_SRC_DIR)/emojidef.c \
    $(LIBUNIBREAK_SRC_DIR)/graphemebreak.c \
    $(LIBUNIBREAK_SRC_DIR)/wordbreak.c

include $(BUILD_STATIC_LIBRARY)
