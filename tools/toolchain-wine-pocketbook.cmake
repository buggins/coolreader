# CMake toolchain file for building Pocketbook Free SDK software using

# specify the cross compiler
SET(CMAKE_C_COMPILER   winegcc)
SET(CMAKE_CXX_COMPILER wineg++)

SET(CMAKE_FIND_ROOT_PATH  /usr/local/pocketbook)

SET(CMAKE_C_FLAGS "-g -mwindows -m32")
SET(CMAKE_CXX_FLAGS "-g -mwindows -m32")

ADD_DEFINITIONS( -DCR_EMULATE_GETTEXT=1)

