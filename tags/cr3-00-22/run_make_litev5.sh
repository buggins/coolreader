#build for V5 LBook reader
#V2HOME=/home/lve/src/v2sdk-1.0/

SDK=/home/lve/src/v5sdk
TARGET="arm-9tdmi-linux-gnu"
GCC_PATH=${SDK}/toolchain/gcc-4.0.0-glibc-2.3.5/${TARGET}/bin
echo GCC_PATH is ${GCC_PATH}
if test ! -x "${GCC_PATH}/${TARGET}-gcc"; then
    echo "Can't find cross toolchain"
    exit 1
fi
#INCLUDES="-I ${SDK}/include/arm/zlib -I ${SDK}/include"
echo building ARM
PATH=${GCC_PATH}:${PATH} make -f Makefile.lbookv5 LBOOK=i386 BUILD_LITE=1 V2HOME=${SDK}
#PATH=${GCC_PATH}:${PATH} make -f Makefile.lbookv5 LBOOK=i386 BUILD=Debug BUILD_LITE=1 V2HOME=${SDK} lbook-i386
#PATH=${GCC_PATH}:${PATH} make -f Makefile.lbookv5 LBOOK=i386 BUILD_LITE=1 V2HOME=${SDK} all
#make -f Makefile.lbook LBOOK=arm BUILD_LITE=1 V2HOME=${V2HOME} all
#make -f Makefile.lbook LBOOK=i386 BUILD_LITE=1 V2HOME=${V2HOME} all
