
# Metadata for deploy script

PN="libunibreak"
PV="4.3"
SRCFILE="${PN}-${PV}.tar.gz"
SHA512="4b53fd169912033403b6ca09047b7b928211fab3607ef26070ab731054138b9a291f7d138d3a479f9cde8edb0fabf8da114da68aee32e60cddf45cc3baae1170"

URL="https://github.com/adah1972/libunibreak/releases/download/libunibreak_${PV/./_}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-add_lb_get_char_class.patch
		02-cmake-static.patch"
