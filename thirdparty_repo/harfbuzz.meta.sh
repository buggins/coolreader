
# Metadata for deploy script

PN="harfbuzz"
PV="2.8.2"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="47f672967ba9e9a1ea21164b9780cbaaad2f4f619811e84d1b6bdb598e5ad60c96e08fadd270460a4ce8e2fb4603fcdd881ba55f2826abc2a4ab439efdc856dc"

URL="https://github.com/harfbuzz/harfbuzz/releases/download/${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-disable-install.patch"
