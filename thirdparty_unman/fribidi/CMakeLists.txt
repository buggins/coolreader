project(fribidi C)

cmake_minimum_required(VERSION 3.4)

set(FRIBIDI_MAJOR_VERSION 1)
set(FRIBIDI_MINOR_VERSION 0)
set(FRIBIDI_MICRO_VERSION 8)

set(FRIBIDI_VERSION "${FRIBIDI_MAJOR_VERSION}.${FRIBIDI_MINOR_VERSION}.${FRIBIDI_MICRO_VERSION}")

set(FRIBIDI_INTERFACE_VERSION 4)
set(FRIBIDI_INTERFACE_AGE 0)
set(FRIBIDI_BINARY_AGE 4)

set(PACKAGE_BUGREPORT "https://github.com/fribidi/fribidi/issues/new")
set(PACKAGE_NAME "GNU FriBidi")
set(PACKAGE_VERSION "${FRIBIDI_VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${CMAKE_PROJECT_NAME}")
set(PACKAGE_URL "http://fribidi.org/")

if (MSVC)
	set(FRIBIDI_BUILT_WITH_MSVC ON)
endif (MSVC)

include (CheckTypeSize)
include (CheckIncludeFile)
include (CheckFunctionExists)


# AC_HEADER_STDC
CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(wchar.h HAVE_WCHAR_H)

CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILE(asm/page.h HAVE_ASM_PAGE_H)
CHECK_INCLUDE_FILE(sys/times.h HAVE_SYS_TIMES_H)

CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(short SIZEOF_SHORT)
CHECK_TYPE_SIZE(void* SIZEOF_VOID_P)
CHECK_TYPE_SIZE(wchar_t SIZEOF_WCHAR_T)

check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(strdup HAVE_STRDUP)

set(HAVE_STRINGIZE 1)

option(NO_DEPRECATED "do not build deprecated functionality" ON)

if (NO_DEPRECATED)
	set (FRIBIDI_NO_DEPRECATED ON)
endif (NO_DEPRECATED)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file (fribidi-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/fribidi-config.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(lib)
add_definitions(-DHAVE_CONFIG_H=1)

set(FRIBIDI_SOURCE
	lib/fribidi.c
	lib/fribidi-arabic.c
	lib/fribidi-bidi.c
	lib/fribidi-bidi-types.c
	lib/fribidi-char-sets.c
	lib/fribidi-char-sets-cap-rtl.c
	lib/fribidi-char-sets-cp1255.c
	lib/fribidi-char-sets-cp1256.c
	lib/fribidi-char-sets-iso8859-6.c
	lib/fribidi-char-sets-iso8859-8.c
	lib/fribidi-char-sets-utf8.c
	lib/fribidi-deprecated.c
	lib/fribidi-joining.c
	lib/fribidi-joining-types.c
	lib/fribidi-mirroring.c
	lib/fribidi-brackets.c
	lib/fribidi-run.c
	lib/fribidi-shape.c
)

add_library(fribidi STATIC ${FRIBIDI_SOURCE})
