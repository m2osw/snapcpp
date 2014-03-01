#!/bin/sh
#
SNAPINIT=/usr/bin/snapinit
SNAPFLAGS="--detach --binary_path ./src --logfile snapinit.log -c my_server.conf --all"

if [ -z "$1" ]
then
	echo "usage $0 [start|stop|restart]"
	exit 1
fi

./src/snapinit ${SNAPFLAGS} $1

