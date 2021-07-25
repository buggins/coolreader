
# Metadata for deploy script

PN="freetype"
PV="2.11.0"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="2"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="bf1991f3c382832586be1d21ae73c20840ee8546807ba60d0eb0215134545656c0c8de488f27357d4a4f6497d7cb540998cda98ec59061a3e640036fb209147d"
URL="https://download.savannah.gnu.org/releases/${PN}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES=
