#!/bin/bash
# This script creates a source package for the project in the current directory.
# In order for this to work properly, there has to be a debian folder present,
# with proper files present.
#
# The parameter to the script is the platform on which to build (i.e. saucy, trusty, etc).
# The default is "saucy."
#

set -e

# Handle command line
#
if [ -z "$1" ]
then
	DIST=xenial
else
	DIST=$1
fi

if [ ! -e debian/changelog ]
then
	echo "No debian changelog found!"
	exit 1
fi

# Use the build server email and name. This will appear in the changelog.
#
if [ -z "${DEBEMAIL}" ]
then
    export DEBEMAIL="Build Server <build@m2osw.com>"
fi

# Clean up the old files
#
NAME=`dpkg-parsechangelog --show-field Source`
VERSION=`dpkg-parsechangelog --show-field Version`
FULLNAME=${NAME}_${VERSION}
echo "Building source package for '${FULLNAME}'"
rm -f ../${FULLNAME}.dsc ../${FULLNAME}_source.* ../${FULLNAME}.tar.gz

# Build the source package itself. The output will be placed in the parent directory
# of the CWD.
#
debuild -S -sa -nc -m"${DEBEMAIL}"

# vim: ts=4 sw=4 et
