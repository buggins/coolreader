
# Metadata for deploy script

PN="harfbuzz"
PV="14.2.1"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="481dca68900e57895d3671baf0595c2d3a26f4f08d0db5662e83089f0d0ff6dc0c28c64c2811eb0f812b0525ed428e90db44f091a88bef47ace2f97d8285b013"

URL="https://github.com/harfbuzz/harfbuzz/releases/download/${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES=
