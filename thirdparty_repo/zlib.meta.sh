
# Metadata for deploy script

PN="zlib"
PV="1.3.2"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="cf3d49fbabddc57cca99858feeca8f910e9de42a16014cddd406814d2d24ee33fee2af3805d7efbb1b04b05f55818092b000daf82502b675df65f2512c353f73"

URL="http://www.zlib.net/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static-only-no-install.patch"
