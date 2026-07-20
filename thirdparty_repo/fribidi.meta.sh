
# Metadata for deploy script

PN="fribidi"
PV="1.0.16"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="1"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="e3a56f36155f6813e3609473639fc533de742309f561c463012dc90b412a1ac7694b765d92669b2cbfaee973ca0e92fa5e926e68a1a078921f26ef17d82ab651"

URL="https://github.com/fribidi/fribidi/releases/download/v${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static.patch"
