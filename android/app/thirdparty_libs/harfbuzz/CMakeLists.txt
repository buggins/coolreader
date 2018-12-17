
project(harfbuzz)

set(HARFBUZZ_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/harfbuzz)
set(HARFBUZZ_CONFIG_DIR ${PROJECT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${HARFBUZZ_CONFIG_DIR})
include_directories(${HARFBUZZ_SRC_DIR}/src/hb-ucdn)

add_definitions(-DHAVE_CONFIG_H=1)

set(HARFBUZZ_SRC_FILES
    ${HARFBUZZ_SRC_DIR}/src/hb-aat-layout.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-aat-map.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-blob.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-buffer.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-buffer-serialize.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-common.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-face.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-fallback-shape.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-font.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ft.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-map.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-color.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-face.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-font.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-layout.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-map.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-math.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-name.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-name-language.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-arabic.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-default.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-hangul.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-hebrew.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-indic.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-indic-table.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-khmer.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-myanmar.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-thai.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-use.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-use-table.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-complex-vowel-constraints.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-fallback.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape-normalize.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-shape.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-tag.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ot-var.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-set.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-shape.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-shape-plan.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-shaper.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-static.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-ucdn.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-unicode.cc
    ${HARFBUZZ_SRC_DIR}/src/hb-warning.cc

    ${HARFBUZZ_SRC_DIR}/src/hb-ucdn/ucdn.c
)
#    ${HARFBUZZ_SRC_DIR}/src/hb-subset-glyf.cc
#    ${HARFBUZZ_SRC_DIR}/src/hb-subset-input.cc
#    ${HARFBUZZ_SRC_DIR}/src/hb-subset.cc
#    ${HARFBUZZ_SRC_DIR}/src/hb-subset-plan.cc

add_library(harfbuzz STATIC ${HARFBUZZ_SRC_FILES})
