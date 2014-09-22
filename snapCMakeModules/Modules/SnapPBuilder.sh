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
set -e
if [ -z "$1" ]
then
    echo "usage: $0 distribution"
    exit 1
fi

if [ ! -e debian/changelog ]
then
	echo "No debian changelog found!"
	exit 1
fi

NAME=`dpkg-parsechangelog --show-field Source`
VERSION=`dpkg-parsechangelog --show-field Version`

pbuilder-dist $1 build ../${NAME}_${VERSION}.dsc

# vim: ts=4 sw=4 et
