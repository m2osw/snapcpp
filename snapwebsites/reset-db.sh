#!/bin/sh
../BUILD/snapwebsites/src/snapdb --drop-tables
../BUILD/snapwebsites/src/snaplayout layouts/bare-body-parser.xsl
../BUILD/snapwebsites/src/snaplayout layouts/bare-theme-parser.xsl
../BUILD/snapwebsites/src/snaplayout layouts/white-body-parser.xsl
../BUILD/snapwebsites/src/snaplayout layouts/white-theme-parser.xsl
../BUILD/snapwebsites/src/snapserver -d -c src/snapserver.conf --add-host
# ../BUILD/snapwebsites/src/snapserver -d --add-host halk
