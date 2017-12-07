#!/bin/sh

DIR=`pwd`
BASENAME=`basename $DIR`
if test "$BASENAME" = "snapwebsites"
then
    cd ..
fi

# Generating the source must be done in the concerned folder
cd snapwebsites

# Verify that version is 4 numbers separated by 3 periods
VERSION=`dpkg-parsechangelog -S version`
VALID=`echo "$VERSION" | sed -e 's/[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+~.*//'`
if test -n "$VALID"
then
    echo "error: invalid version \"$VERSION\": must be 4 numbers separated by periods."
    exit 1
fi

debuild -S -sa

cd ..

dput ppa:snapcpp/ppa snapwebsites_${VERSION}_source.changes


# vim: ts=4 sw=4 et
