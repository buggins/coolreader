
# Metadata for deploy script

PN="libjpeg"
PV="9.4.0"
SRCFILE="jpegsrc.v9d.tar.gz"
SHA512="6515a6f617fc9da7a3d7b4aecc7d78c4ee76159d36309050b7bf9f9672b4e29c2f34b1f4c3d7d65d7f6e2c104c49f53dd2e3b45eac22b1576d2cc54503faf333"

URL="http://www.ijg.org/files/${SRCFILE}"

SOURCESDIR="jpeg-9d"

PATCHES="01-cmake-static.patch
		02-preconfigured.patch
		03-fix-warns.patch"
