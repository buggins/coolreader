
# Metadata for deploy script

PN="freetype"
PV="2.10.4"
# package revision: when patchset is changed (but not version), increase it
# when version changed, reset to "1".
REV="2"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="827cda734aa6b537a8bcb247549b72bc1e082a5b32ab8d3cccb7cc26d5f6ee087c19ce34544fa388a1eb4ecaf97600dbabc3e10e950f2ba692617fee7081518f"

URL="https://download.savannah.gnu.org/releases/${PN}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-disable-install.patch
    0001-srd-base-ftlcdfil.c-FT_Library_SetLcdGeometry-Fix-re.patch"
