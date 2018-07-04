#!/bin/sh
#
# This script is used to upload the bundles-*.xml file from
#
#     BUILD/dist/shared/snapwebsites/bundles/*
#
# To our build server. Change the upload parameters to use your own server.

BUILD_SERVER=build.m2osw.com
# TODO: get an HTTPS instead of HTTP
GENERATE_DIRECTORY_XML=http://build-server.m2osw.com/generate-directory-xml.php

# Get the BUILD path
#
BUILD_PATH=`pwd`
while true
do
    if [ -d "${BUILD_PATH}/BUILD" ]
    then
        BUILD_PATH="${BUILD_PATH}/BUILD"
        break
    fi
    if [ -z "${BUILD_PATH}" -o "${BUILD_PATH}" = "/" ]
    then
        # could not find the build path...
        #
        echo "error: BUILD folder not found, did you start this script outside of the snapcpp environment?"
        exit 1
    fi
    BUILD_PATH=`dirname ${BUILD_PATH}`
done

# Got build path, now we know where the bundles are
#
BUNDLES_PATH="${BUILD_PATH}/dist/share/snapwebsites/bundles"

SNAP_BUNDLES=/tmp/snap-bundles.tar.gz

cd ${BUNDLES_PATH}
tar czf ${SNAP_BUNDLES} bundle-*.xml
scp ${SNAP_BUNDLES} ${BUILD_SERVER}:.
rm ${SNAP_BUNDLES}

wget -O /dev/null "${GENERATE_DIRECTORY_XML}?user=${USER}"

# vim: ts=4 sw=4 et
