#!/bin/bash
# This script creates a source package for the project in the current directory.
# In order for this to work properly, there has to be a debian folder present,
# with proper files present.
#
# The parameter to the script is the platform on which to build (i.e. saucy, trusty, etc).
# The default is "saucy."
#

# Handle command line
#
unset NOINC_VERSION
if [ "$1" == "--noinc" ]
then
    NOINC_VERSION=1
    shift
fi

if [ -z "$1" ]
then
	DIST=saucy
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

if [ -z "$NOINC_VERSION" ]
then
    # Get the current version from the change log, get rid of everything past the tilde "~",
    # and increment the version number.
    #
    CURVER=`dpkg-parsechangelog --show-field Version | sed 's/~.*$//'`
    INCVER=`echo ${CURVER} | perl -pe 's/^((\d+\.)*)(\d+)(.*)$/$1.($3+1).$4/e'`

    # Write a new changelog entry, but using "~dist".
    #
    dch --newversion ${INCVER}~${DIST} --urgency high --distribution ${DIST} Nightly build.
    if [ "$?" != 0 ]
    then
        echo "Error running dch! Aborting..."
        exit 1;
    fi
fi

# Build the source package itself. The output will be placed in the parent directory
# of the CWD.
#
debuild -S -sa
if [ "$?" != 0 ]
then
	echo "Error running debuild! Aborting..."
    exit 1
fi

exit 0

# vim: ts=4 sw=4 et
