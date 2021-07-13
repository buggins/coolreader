
# Metadata for deploy script

PN="zstd"
PV="1.5.0"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.gz"
SHA512="b322fc1b89a556827b7fece2fb0f34c83bf65bb85b2468c791d6d9178a65c81e21f4171b7533cbf12bc1dfb2fd323d3e8c34d86167b157645c27f65186eec659"

URL="https://github.com/facebook/zstd/releases/download/v${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-disable-install.patch"
