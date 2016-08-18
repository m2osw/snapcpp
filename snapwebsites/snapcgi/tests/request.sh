#!/bin/sh
#

# Prepare the request
#
export REQUEST_METHOD=GET
export REMOTE_ADDR=192.168.2.45
export HTTP_HOST=csnap.m2osw.com
export REQUEST_URI=/
export HTTP_USER_AGENT="Mozilla 5.1"

# Run the CGI under full control
#
../../BUILD/snapwebsites/snapcgi/src/snap.cgi
