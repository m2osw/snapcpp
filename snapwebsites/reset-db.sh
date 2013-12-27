#!/bin/sh
snapdb --drop-tables
snaplayout layouts/bare-body-parser.xsl
snaplayout layouts/bare-theme-parser.xsl
snaplayout layouts/white-body-parser.xsl
snaplayout layouts/white-theme-parser.xsl
snapserver -d -c /etc/snapserver.conf --add-host
# snapserver -d --add-host halk
