
# Metadata for deploy script

PN="libjpeg"
PV="10.0.0"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="jpegsrc.v10.tar.gz"
SHA512="3bdd55d805d76f35cbcd44074e27684a25b40f4811c62374ec9300dde31fa75a17ff3fccfcb8f5947311fa7b07f52dc9cba343562ae4ceb8b3cbe30fda1e4311"

URL="http://www.ijg.org/files/${SRCFILE}"

SOURCESDIR="jpeg-10"

PATCHES="01-cmake-static.patch
		02-preconfigured.patch
		03-fix-warns.patch"
