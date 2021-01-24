set -e

git clone https://github.com/fribidi/c2man.git
cd c2man

./Configure -dE
mkdir -p $TRAVIS_BUILD_DIR/c2man-install
echo "binexp=$TRAVIS_BUILD_DIR/c2man-install" >> config.sh
echo "installprivlib=$TRAVIS_BUILD_DIR/c2man-install" >> config.sh
echo "mansrc=$TRAVIS_BUILD_DIR/c2man-install" >> config.sh
sh config_h.SH
sh flatten.SH
sh Makefile.SH

make depend
make
make install
