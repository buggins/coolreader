
# Metadata for deploy script

PN="harfbuzz"
PV="2.8.0"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="0ece78fa5c18be1fd693f40493ff3689a297ef4d5ad82f31054e8f42c1ba4e8b93afe1ed1ccb9eb9a0f165f8d1343c12c0606f1768a4f85bb041ae65018e6d83"

URL="https://github.com/harfbuzz/harfbuzz/releases/download/${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-disable-install.patch"
