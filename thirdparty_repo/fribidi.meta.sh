
# Metadata for deploy script

PN="fribidi"
PV="1.0.10"
SRCFILE="${PN}-${PV}.tar.xz"
SHA512="cb51920012c3c7507c17e2beb1dbbcfb8d7c6404e4cb54b260a332754a0d5b103d8834d77e8795651b3f38069c9bd2e9914c21b001411a72f9ffe1ec1ef2f360"

URL="https://github.com/fribidi/fribidi/releases/download/v${PV}/${SRCFILE}"

SOURCESDIR="${PN}-${PV}"

PATCHES="01-cmake-static.patch
		9e78abb7ead0781a6160be2e0d27d212ccd6f438.patch
		a11d8b3942546906b4a74d2061f50e05e4d33f18.patch"
