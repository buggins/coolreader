# CMake toolchain file for building Jinke V3 software using gcc 3.3.4

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /opt/arm-9tdmi-linux-gnu/gcc-4.0.0-glibc-2.3.5/bin/arm-9tdmi-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /opt/arm-9tdmi-linux-gnu/gcc-4.0.0-glibc-2.3.5/bin/arm-9tdmi-linux-gnu-g++)
SET(CMAKE_STRIP /opt/arm-9tdmi-linux-gnu/gcc-4.0.0-glibc-2.3.5/bin/arm-9tdmi-linux-gnu-strip)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /opt/arm-9tdmi-linux-gnu/gcc-4.0.0-glibc-2.3.5/arm-9tdmi-linux-gnu)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
