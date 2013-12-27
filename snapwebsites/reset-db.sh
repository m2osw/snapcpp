#!/bin/sh
LAYOUTDIR=/usr/share/snapwebsites/layouts
#snapdb --host 10.1.1.1 --drop-tables
snaplayout ${LAYOUTDIR}/bare-body-parser.xsl
snaplayout ${LAYOUTDIR}/bare-theme-parser.xsl
snaplayout ${LAYOUTDIR}/white-body-parser.xsl
snaplayout ${LAYOUTDIR}/white-theme-parser.xsl
snapserver -d -c /etc/snapwebsites/snapserver.conf --add-host
# snapserver -d --add-host halk
