#!/bin/sh -e

DIR=`pwd`
BASENAME=`basename $DIR`
if test "$BASENAME" = "snapwebsites"
then
    cd ..
fi

if test "$1" = "-h" -o "$1" = "--help"
then
    echo
    echo "Usage: $0 [project-name]"
    echo
    echo "  Where project-name defaults to \"snapwebsites\""
    echo "  and it can be set to any one of our contrib as well"
    echo
    echo "Valid project-names are:"
    echo
    ls contrib/ | tr '[:upper:]' '[:lower:]' | grep -v cmakelists.txt | sed -e '$a\ \ snapcmakemodules' -e '$a\ \ snapwebsites' -e 's/^/  /' | sort
    echo
    exit 1;
fi

if test -n "$1"
then
    MODULE="$1"
else
    MODULE=snapwebsites
fi

TMP=../tmp

# Generating the source must be done in the concerned folder
case $MODULE in
"snapwebsites")
    cd snapwebsites
    ;;
"snapcmakemodules")
    cd cmake
    ;;
*)
    cd contrib/$MODULE
    TMP=../../tmp
    ;;
esac

# Verify that version is 4 numbers separated by 3 periods
# The "~xenial" part has to be updated for each platform
# (add (xenial|<another>|<another>|...) to support multiple versions.)
#
VERSION=`dpkg-parsechangelog -S version`
VALID=`echo "$VERSION" | sed -e 's/[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+~xenial//'`
if test -n "$VALID"
then
    echo "error: invalid version \"$VERSION\": must be 4 numbers separated by periods."
    exit 1
fi

debuild -S -sa

# To send the source to Launchpad, we need to be at the same level as those
# files
cd ..

dput ppa:snapcpp/ppa ${MODULE}_${VERSION}_source.changes

# Get rid of those files from our source tree
mkdir -p $TMP/sources
mv ${MODULE}_${VERSION}* $TMP/sources/.

# vim: ts=4 sw=4 et
