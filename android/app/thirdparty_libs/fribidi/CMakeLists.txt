project(fribidi C)

cmake_minimum_required(VERSION 3.4)

set(FRIBIDI_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../../../../thirdparty/fribidi")
set(FRIBIDI_CONFIG_DIR "${PROJECT_SOURCE_DIR}")

include_directories(${FRIBIDI_SOURCE_DIR})
include_directories(${FRIBIDI_CONFIG_DIR})
include_directories(${FRIBIDI_CONFIG_DIR}/lib)
add_definitions(-DHAVE_CONFIG_H=1)

set(FRIBIDI_SOURCE
		${FRIBIDI_SOURCE_DIR}/lib/fribidi.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-arabic.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-bidi.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-bidi-types.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets-cap-rtl.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets-cp1255.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets-cp1256.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets-iso8859-6.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets-iso8859-8.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-char-sets-utf8.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-deprecated.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-joining.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-joining-types.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-mirroring.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-brackets.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-run.c
		${FRIBIDI_SOURCE_DIR}/lib/fribidi-shape.c
)

add_library(fribidi STATIC ${FRIBIDI_SOURCE})
