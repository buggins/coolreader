
# Metadata for deploy script

PN="libpng"
PV="1.6.37"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="59e8c1059013497ae616a14c3abbe239322d3873c6ded0912403fc62fb260561768230b6ab997e2cccc3b868c09f539fd13635616b9fa0dd6279a3f63ec7e074"

URL="https://download.sourceforge.net/${PN}/${PN}-${PV}.tar.xz"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static-r1.patch"
