#!/bin/sh
#
# This script is expected to pull all the sub-modules from the remote
# repository. If anything was not committed then the command will fail.
# The script understands the difference between contribs and main
# submodules (which is why a simple `git submodule` recursive wouldn't work)
# and it pulls the latest for each sub-module instead of whatever the main
# module thinks is current (since at times it's not 100% up to date).

SUBMODULES="
	advgetopt
	as2js
	cassandra-cpp-driver-snap
	cmake
	commonmarkcpp
	cppthread
	csspp
	eventdispatcher
	fastjournal
	ftmesh
	iplock
	ipmgr
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

for submodule in $SUBMODULES
do
	if test -d contrib/$submodule
	then
		echo "Retrieve contrib/$submodule (main)"
		(
			cd contrib/$submodule
			git checkout main
			git pull origin main
		)
	else
		echo "Retrieve $submodule (main)"
		(
			cd $submodule
			git checkout main
			git pull origin main
		)
	fi
done

