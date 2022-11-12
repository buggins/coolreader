
# Metadata for deploy script

PN="zlib"
PV="1.2.13"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="9e7ac71a1824855ae526506883e439456b74ac0b811d54e94f6908249ba8719bec4c8d7672903c5280658b26cb6b5e93ecaaafe5cdc2980c760fa196773f0725"

URL="http://www.zlib.net/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static-only-no-install.patch"
