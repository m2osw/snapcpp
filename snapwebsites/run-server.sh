#!/bin/sh
#
# To be run when the server is installed globally, for production.
# This assumes that the snap* commands are in your path, and
# that the configuration files are stored under "/etc/snapwebsites".
#
# Warning: do not run this on a production server, unless you want to
# erase the entire database! This is intended for testing only.
#
HOSTNAME=127.0.0.1
PORT=9160
LAYOUTDIR=/usr/share/snapwebsites/layouts
BINDIR=./src
${BINDIR}/snapserver -d -c ./snapserver.conf

# To setup the bare theme until the default theme works as expected
# (but we cannot run it at this point yet because the database was not
# fully initialized to support such.)
#${BUILDDIR}/snaplayout --set-theme http://www.example.com layout '"bare";'
#${BUILDDIR}/snaplayout --set-theme http://www.example.com theme '"bare";'

