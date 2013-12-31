#!/bin/sh
#
# To be run when the server is not installed globally, for developers.
# This assumes that the snap* commands are stored under the snapcpp build
# dir (../BUILD).
#
# Warning: do not run this on a production server, unless you want to
# erase the entire database! This is intended for testing only.
#
SOURCEDIR=.
BUILDDIR=../BUILD/snapwebsites/src
LAYOUTDIR=${SOURCEDIR}/layouts
${BUILDDIR}/snapdb --drop-tables
${BUILDDIR}/snaplayout ${LAYOUTDIR}/bare-body-parser.xsl
${BUILDDIR}/snaplayout ${LAYOUTDIR}/bare-theme-parser.xsl
${BUILDDIR}/snaplayout ${LAYOUTDIR}/white-body-parser.xsl
${BUILDDIR}/snaplayout ${LAYOUTDIR}/white-theme-parser.xsl
${BUILDDIR}/snapserver -d -c ${SOURCEDIR}/src/snapserver.conf --add-host

