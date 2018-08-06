#!/bin/sh

BUILDDIR_TMPL="clang-bldXXXXX"
MAKEOPTS="-j5"
REPORTS_SUBDIR="clang-analyzer-reports"
CLEAR_OLDREPORTS=yes
CLEAR_BUILDDIR=yes

die()
{
	echo $*
	exit 1
}

srcdir=`pwd`
builddir=`mktemp --tmpdir=/tmp -d "${BUILDDIR_TMPL}"`

test -f CMakeLists.txt || die "This script must be called only from top project directory!"
test -d "${builddir}" && rm -rf "${builddir}"
test -d "${builddir}" && die "Failed to delete ${builddir}!"


test "x${CLEAR_OLDREPORTS}" = "xyes" && rm -rf "${srcdir}/${REPORTS_SUBDIR}"/*
mkdir "${builddir}"
cd "${builddir}" || die "Can't change dir to ${builddir}"

scan-build cmake -DGUI=QT5 "${srcdir}"
scan-build -o "${srcdir}/${REPORTS_SUBDIR}" make ${MAKEOPTS}

test "x${CLEAR_BUILDDIR}" = "xyes" && rm -rf "${builddir}"

cd "${srcdir}"
