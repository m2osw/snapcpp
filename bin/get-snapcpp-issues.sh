#!/bin/sh
#

if test -d snapwebsites
then
	cd snapwebsites
fi

scp build.m2osw.com:/home/build/snapcpp-issues.txt snapbase/doc/.
