
# Metadata for deploy script

PN="zlib"
PV="1.2.12"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="12940e81e988f7661da52fa20bdc333314ae86a621fdb748804a20840b065a1d6d984430f2d41f3a057de0effc6ff9bcf42f9ee9510b88219085f59cbbd082bd"

URL="http://www.zlib.net/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static-only-no-install.patch"
