
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_fribidi

FRIBIDI_SRC_DIR := ../../../../thirdparty/fribidi
FRIBIDI_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/fribidi
FRIBIDI_CONFIG_DIR_P := $(LOCAL_PATH)

LOCAL_C_INCLUDES := \
	$(FRIBIDI_CONFIG_DIR_P) \
	$(FRIBIDI_CONFIG_DIR_P)/lib \
	$(FRIBIDI_SRC_DIR_P)/lib

LOCAL_CFLAGS += -DHAVE_CONFIG_H=1
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(FRIBIDI_SRC_DIR)/lib/fribidi.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-arabic.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-bidi.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-bidi-types.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets-cap-rtl.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets-cp1255.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets-cp1256.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets-iso8859-6.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets-iso8859-8.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-char-sets-utf8.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-deprecated.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-joining.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-joining-types.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-mirroring.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-brackets.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-run.c \
    $(FRIBIDI_SRC_DIR)/lib/fribidi-shape.c \

include $(BUILD_STATIC_LIBRARY)
