#!/bin/bash

top_srcdir=`pwd`

thirdparty_dir="${top_srcdir}/thirdparty"
repo_dir="${top_srcdir}/thirdparty_repo"
repo_tmpdir="${top_srcdir}/thirdparty_tmp"

cleanup()
{
	:
}

die()
{
	echo $*
	cd "${pwd1}"
	cleanup
	exit 1
}

_get_tar_args()
{
	local args=
	case "${1}" in
	*.tar.gz)
		args="z"
		;;
	*.tar.bz2)
		args="j"
		;;
	*.tar.xz)
		args="J"
		;;
	esac
	echo "${args}"
}

deploy_package()
{
	local pkgname="${1}"
	local metafile="${repo_dir}/${pkgname}.meta.sh"

	source "${metafile}"

	local patchesdir="${repo_dir}/patches/${PN}"
	local pkg_datadir="${repo_tmpdir}/${PN}"

	local pwd1=`pwd`

	if [ ! -d "${pkg_datadir}" ]
	then
		mkdir "${pkg_datadir}" || die "Failed to create package tmpdir!"
	fi
	cd "${pkg_datadir}" || die "chdir failed!"

	# Check consistency
	if [ ! -d "${thirdparty_dir}/${SOURCESDIR}" ]
	then
		rm -f .unpacked .prepared
	fi
	if [ ! -f "${SRCFILE}" ]
	then
		rm -f .downloaded .verified
	fi

	if [ ! -f .downloaded ]
	then
		if [ -f "${SRCFILE}" ]
		then
			rm -f "${SRCFILE}"
		fi
		curl -L -O ${URL} || die "Failed to fetch sources!"
		if [ ! -f "${SRCFILE}" ]
		then
			die "Something wrong... source file not found!"
		fi
		echo "1" > .downloaded
		echo "Downloaded OK."
	fi

	if [ ! -f .verified ]
	then
		# make sha512 sum file
		echo "${SHA512} *${SRCFILE}" > "${SRCFILE}.sha512"
		sha512sum -c "${SRCFILE}.sha512" || if [ "x" = "x" ]; then rm -f .downloaded; die "Failed to verify checksum!"; fi
		echo "1" > .verified
		rm -f "${SRCFILE}.sha512"
		echo "Checksum OK."
	fi

	local tar_args=`_get_tar_args "${SRCFILE}"`
	if [ ! -f .unpacked ]
	then
		cd "${thirdparty_dir}" || die "chdir to thirdparty_dir failed!"
		tar -x${tar_args}f "${pkg_datadir}/${SRCFILE}" || die "Failed to unpack sources!"
		cd "${pkg_datadir}" || die "chdir failed!"
		echo "1" > .unpacked
		echo "Unpacked OK."
	fi

	if [ ! -f .prepared ]
	then
		cd "${thirdparty_dir}/${SOURCESDIR}" || die "chdir to srcdir failed!"
		for p in ${PATCHES}
		do
			patch -p1 -i "${patchesdir}/${p}" || die "Failed to patch!"
		done
		cd "${pkg_datadir}" || die "chdir failed!"
		echo "1" > .prepared
		echo "Prepared OK."
	fi

	# clean vars
	unset -v PN
	unset -v PV
	unset -v SRCFILE
	unset -v SHA512
	unset -v URL
	unset -v SOURCEDIR
	unset -v PATCHES

	cd "${pwd1}"
}

# check current directory
if [ ! -f ./thirdparty-deploy.sh ]
then
	die "Error! You must call this script only in top_srcdir!"
fi

if [ ! -d "${thirdparty_dir}" ]
then
	mkdir "${thirdparty_dir}" || die "Failed to create thirdparty_dir!"
fi

if [ ! -d "${repo_tmpdir}" ]
then
	mkdir "${repo_tmpdir}" || die "Failed to create repo tmpdir!"
fi

deploy_package libunibreak
deploy_package libpng
deploy_package libjpeg
deploy_package freetype
deploy_package harfbuzz
deploy_package fribidi
deploy_package zlib
