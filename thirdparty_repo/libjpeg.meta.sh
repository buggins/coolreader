
# Metadata for deploy script

PN="libjpeg"
PV="9.4.0"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="jpegsrc.v9d.tar.gz"
SHA512="c64d3ee269367351211c077a64b2395f2cfa49b9f8257fae62fa1851dc77933a44b436d8c70ceb52b73a5bedff6dbe560cc5d6e3ed5f2997d724e2ede9582bc3"

URL="http://www.ijg.org/files/${SRCFILE}"

SOURCESDIR="jpeg-9d"

PATCHES="01-cmake-static.patch
		02-preconfigured.patch
		03-fix-warns.patch"
