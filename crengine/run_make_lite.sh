#build for V2 LBook reader
#V2HOME=/home/lve/src/v2sdk-1.0/

SDK=${SDK:-/home/lve/src/v3sdk-1.0}
TARGET="arm-9tdmi-linux-gnu"
GCC_PATH=${SDK}/${TARGET}/gcc-3.3.4-glibc-2.2.5/bin
echo GCC_PATH is ${GCC_PATH}
if test ! -x "${GCC_PATH}/${TARGET}-gcc"; then
    echo "Can't find cross toolchain"
    exit 1
fi
#INCLUDES="-I ${SDK}/include/arm/zlib -I ${SDK}/include"
PATH=${GCC_PATH}:${PATH} make -f Makefile.lbook BUILD=Release LBOOK=arm BUILD_LITE=1 V2HOME=${SDK}
PATH=${GCC_PATH}:${PATH} make -f Makefile.lbook BUILD=Debug LBOOK=i386 BUILD_LITE=1 V2HOME=${SDK}
#PATH=${GCC_PATH}:${PATH} make  -f Makefile.lbook LBOOK=i386 BUILD_LITE=1 V2HOME=${SDK} all
#make -f Makefile.lbook LBOOK=arm BUILD_LITE=1 V2HOME=${V2HOME} all
#make -f Makefile.lbook LBOOK=i386 BUILD_LITE=1 V2HOME=${V2HOME} all
