
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_harfbuzz

HARFBUZZ_SRC_DIR := ../../../../thirdparty/harfbuzz
HARFBUZZ_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/harfbuzz
FREETYPE_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty/freetype
HARFBUZZ_CONFIG_DIR_P := $(LOCAL_PATH)

LOCAL_C_INCLUDES := \
	$(HARFBUZZ_CONFIG_DIR_P) \
	$(HARFBUZZ_SRC_DIR_P) \
	$(HARFBUZZ_SRC_DIR_P)/src/hb-ucdn \
	$(FREETYPE_SRC_DIR_P) \
	$(FREETYPE_SRC_DIR_P)/include

LOCAL_CFLAGS += -DHAVE_CONFIG_H=1
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries

LOCAL_SRC_FILES := \
    $(HARFBUZZ_SRC_DIR)/src/hb-aat-layout.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-aat-map.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-blob.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-buffer.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-buffer-serialize.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-common.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-face.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-fallback-shape.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-font.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ft.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-map.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-color.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-face.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-font.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-layout.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-map.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-math.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-name.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-name-language.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-arabic.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-default.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-hangul.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-hebrew.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-indic.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-indic-table.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-khmer.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-myanmar.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-thai.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-use.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-use-table.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-complex-vowel-constraints.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-fallback.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape-normalize.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-shape.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-tag.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ot-var.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-set.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-shape.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-shape-plan.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-shaper.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-static.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ucdn.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-unicode.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-warning.cc \
    $(HARFBUZZ_SRC_DIR)/src/hb-ucdn/ucdn.c
#    $(HARFBUZZ_SRC_DIR)/src/hb-subset-glyf.cc
#    $(HARFBUZZ_SRC_DIR)/src/hb-subset-input.cc
#    $(HARFBUZZ_SRC_DIR)/src/hb-subset.cc
#    $(HARFBUZZ_SRC_DIR)/src/hb-subset-plan.cc

include $(BUILD_STATIC_LIBRARY)
