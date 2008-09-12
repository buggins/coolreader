#build for V2 LBook reader
#V2HOME=/home/lve/src/v2sdk-1.0/
make LBOOK=arm BUILD_LITE=1 V2HOME=${V2HOME} all
make LBOOK=i386 BUILD_LITE=1 V2HOME=${V2HOME} all
