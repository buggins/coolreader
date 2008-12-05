#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4 ltmain.sh

touch README

cd crengine && NOCONFIGURE=1 sh autogen.sh && cd ..

echo "Running aclocal..." ; aclocal $ACLOCAL_FLAGS || exit 1
#echo "Running autoheader..." ; autoheader || exit 1
echo "Running autoconf..." ; autoconf || exit 1
echo "Running libtoolize..." ; (libtoolize --copy --automake || glibtoolize --automake) || exit 1
echo "Running automake..." ; automake --add-missing --copy --gnu || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
