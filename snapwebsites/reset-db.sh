#!/bin/sh
#
# To be run when the server is installed globally, for production.
# This assumes that the snap* commands are in your path, and
# that the configuration files are stored under "/etc/snapwebsites".
#
# Warning: do not run this on a production server, unless you want to
# erase the entire database! This is intended for testing only.
#
LAYOUTDIR=/usr/share/snapwebsites/layouts
snapdb --drop-tables
snaplayout ${LAYOUTDIR}/bare-body-parser.xsl
snaplayout ${LAYOUTDIR}/bare-theme-parser.xsl
snaplayout ${LAYOUTDIR}/white-body-parser.xsl
snaplayout ${LAYOUTDIR}/white-theme-parser.xsl
snapserver -d --add-host
