#!/bin/bash
#
# This script uploads a source package to the lp servers, using "ppa:snapcpp/ppa".
#
# The parameter to the script is the package name which will be built. It is assumed that
# the debian subfolder is one level down, in the folder name provided. Package will
# be determined using dpkg-parsechangelog.
#

# Handle command line
#
#if [ -z "$1" ]
#then
#    echo "usage: $0 package-dir"
#    exit 1
#fi
#
#PKGDIR=$1
#if [ ! -e "${PKGDIR}" ]
#then
#    echo "Directory '${PKGDIR}' does not exist!"
#    exit 1
#fi
#cd ${PKGDIR}

set -e

if [ ! -e debian/changelog ]
then
	echo "No debian changelog found!"
	exit 1
fi

VERSION=`dpkg-parsechangelog --show-field Version`
NAME=`dpkg-parsechangelog --show-field Source`

dput ppa:snapcpp/ppa ../${NAME}_${VERSION}_source.changes

# vim: ts=4 sw=4 et
