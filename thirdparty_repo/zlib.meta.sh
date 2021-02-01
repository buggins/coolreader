
# Metadata for deploy script

PN="zlib"
PV="1.2.11"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="b7f50ada138c7f93eb7eb1631efccd1d9f03a5e77b6c13c8b757017b2d462e19d2d3e01c50fad60a4ae1bc86d431f6f94c72c11ff410c25121e571953017cb67"

URL="http://www.zlib.net/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static-only-no-install.patch"
