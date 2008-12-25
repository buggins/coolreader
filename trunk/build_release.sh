rm -rf build/lbook
#make -f Makefile.lbook LBOOK=arm BUILD_LITE=1 V2HOME=${V2HOME} BUILD=Release SHARED=0 \
#&& make -f Makefile.lbook LBOOK=i386 BUILD_LITE=1 V2HOME=${V2HOME} BUILD=Release SHARED=0 \
make -f Makefile.lbook LBOOK=arm V2HOME=${V2HOME} BUILD=Release SHARED=0 \
&& make -f Makefile.lbook LBOOK=i386 V2HOME=${V2HOME} BUILD=Debug SHARED=0

#&& make BUILD=Release SHARED=0 \
