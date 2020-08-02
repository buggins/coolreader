# Custom lightweight cmake project to compile libunibreak as static library

project(libunibreak C)

cmake_minimum_required(VERSION 3.4)

include (CheckTypeSize)
include (CheckIncludeFile)
include (CheckFunctionExists)


CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H)

check_function_exists(memset HAVE_MEMSET)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(src)

set(LIBUNIBREAK_SOURCE
	src/unibreakbase.c
	src/unibreakdef.c
	src/linebreak.c
	src/linebreakdata.c
	src/linebreakdef.c
	src/emojidef.c
	src/graphemebreak.c
	src/wordbreak.c
)

add_library(unibreak STATIC ${LIBUNIBREAK_SOURCE})
