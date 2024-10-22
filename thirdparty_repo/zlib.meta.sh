
# Metadata for deploy script

PN="zlib"
PV="1.3.1"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="1e8e70b362d64a233591906a1f50b59001db04ca14aaffad522198b04680be501736e7d536b4191e2f99767e7001ca486cd802362cca2be05d5d409b83ea732d"

URL="http://www.zlib.net/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static-only-no-install.patch"
