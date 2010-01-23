# CMake toolchain file for building MIPS software on OI environment

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /usr/bin/mipsel-ip-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/mipsel-ip-linux-gnu-g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /usr/mipsel-ip-linux-gnu)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
