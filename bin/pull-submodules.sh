#!/bin/sh
#
# This script is expected to pull all the sub-modules from the remote
# repository. If anything was not commited then the command will fail.
# The script knows of the master and main top repositories. It also
# understands the difference between contribs and main submodules.
# (which is why a simple git submodule recursive wouldn't work).

SUBMODULE_MASTER="
	advgetopt
	as2js
	cassandra-cpp-driver-snap
	cmake
	cppthread
	csspp
	eventdispatcher
	iplock
	libaddr
	libcassvalue
	libcasswrapper
	libexcept
	libmurmur3
	libQtSerialization
	libtld
	libutf8
	libuv1-snap
	log4cplus12
	snapcatch2
	snapdev
	snaplogger
	snaprfs
	snapwebsites
	zipios"

SUBMODULE_MAIN="fastjournal"

pull() {
	for submodule in $1
	do
		if test -d contrib/$submodule
		then
			(
				cd contrib/$submodule
				git pull origin $2
			)
		else
			(
				cd $submodule
				git pull origin $2
			)
		fi
	done
}

pull $SUBMODULE_MASTER master
pull $SUBMODULE_MAIN main

