
# Metadata for deploy script

PN="harfbuzz"
PV="2.7.4"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="d2af6a768c397c664f654cf36140e7b5696b3b983f637454604570c348247f7ffea135048d9b02cf6593cbde728567e31bf82a39df5ff38d680c78dff24d4cf0"

URL="https://github.com/harfbuzz/harfbuzz/releases/download/${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-disable-install.patch"
