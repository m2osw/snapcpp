#!/bin/sh
#
# This script is expected to pull all the sub-modules from the remote
# repository. If anything was not committed then the command will fail.
# The script understands the difference between contribs and main
# submodules (which is why a simple `git submodule` recursive wouldn't work)
# and it pulls the latest for each sub-module instead of whatever the main
# module thinks is current (since at times it's not 100% up to date).

# WARNING: snapbuilder is part of the top module (snapcpp) and plays the
#          role of updating that module to the latest
SUBMODULES="
	advgetopt
	as2js
	basic-xml
	cmake
	commonmarkcpp
	communicatord
	cppthread
	csspp
	edhttp
	eventdispatcher
	fastjournal
	fluid-settings
	ftmesh
	iplock
	ipmgr
	libaddr
	libexcept
	libmimemail
	libtld
	libutf8
	murmur3
	prinbee
	safepasswords
	serverplugins
	sitter
	snapbuilder
	snapcatch2
	snapdev
	snaplogger
	snaprfs
	snapwebsites
	versiontheca
	zipios"

for submodule in $SUBMODULES
do
	if test -d ../$submodule
	then
		echo "Retrieve ../$submodule (main)"
		(
			cd ../$submodule
			git checkout main
			git pull origin main
		)
	elif test -d contrib/$submodule
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

