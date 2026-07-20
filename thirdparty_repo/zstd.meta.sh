
# Metadata for deploy script

PN="zstd"
PV="1.5.7"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.gz"
SHA512="b4de208f179b68d4c6454139ca60d66ed3ef3893a560d6159a056640f83d3ee67cdf6ffb88971cdba35449dba4b597eaa8b4ae908127ef7fd58c89f40bf9a705"

URL="https://github.com/facebook/zstd/releases/download/v${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

TAR_EXCLUDES="--exclude=${SOURCESDIR}/tests"

PATCHES="01-disable-install.patch"
