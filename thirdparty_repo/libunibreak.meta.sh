
# Metadata for deploy script

PN="libunibreak"
PV="7.0"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="4"
SRCFILE="${PN}-${PV}.tar.gz"
SHA512="2dec8aa5788486dcbe1f676d31a103f7a3f253a2bf4af7772b3f3019667b352aa04aae369e0838ce6d554b207a2d555738f3510bb1cfec65adcc8c4164aeb00b"

URL="https://github.com/adah1972/libunibreak/releases/download/libunibreak_${PV/./_}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="02-cmake-static.patch"
