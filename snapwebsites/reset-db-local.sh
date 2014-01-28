#!/bin/sh
#
# To be run when the server is not installed globally, for developers.
# This assumes that the snap* commands are stored under the snapcpp build
# dir (../BUILD).
#
# Warning: do not run this on a production server, unless you want to
# erase the entire database! This is intended for testing only.
#
echo
echo "Reseting your Snap! database..."
echo
SOURCEDIR=.
BUILDDIR=../BUILD/snapwebsites/src
LAYOUTDIR=${SOURCEDIR}/layouts
${BUILDDIR}/snapdb --drop-tables
${BUILDDIR}/snaplayout ${LAYOUTDIR}/bare-content.xml
${BUILDDIR}/snaplayout ${LAYOUTDIR}/bare-body-parser.xsl
${BUILDDIR}/snaplayout ${LAYOUTDIR}/bare-theme-parser.xsl
${BUILDDIR}/snaplayout ${LAYOUTDIR}/bare-style.css
${BUILDDIR}/snaplayout ${LAYOUTDIR}/white-content.xml
${BUILDDIR}/snaplayout ${LAYOUTDIR}/white-body-parser.xsl
${BUILDDIR}/snaplayout ${LAYOUTDIR}/white-theme-parser.xsl
${BUILDDIR}/snapserver -d -c ${SOURCEDIR}/src/snapserver.conf --add-host

echo
echo "Done!"
echo
echo "After a reset, remember that all your data is lost. So you will"
echo "have to register a new user and make him root again with a command"
echo "that looks like this (change the -c and -p parameters as required):"
echo ${BUILDDIR}/snapbackend -d -c ${SOURCEDIR}/src/snapserver.conf -a makeroot -p ROOT_USER_EMAIL=you@example.com
