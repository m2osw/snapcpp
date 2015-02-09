#!/bin/sh
# Statically check the code once in a while
# The result is saved in cppcheck.out (it can be quite large)
cppcheck \
	advgetopt/ \
	as2js/ \
	cassview/ \
	controlled_vars/ \
	googlerank/ \
	iplock/ \
	launchpad/ \
	libQtCassandra/ \
	libQtSerialization/ \
	libtld/ \
	snapmanager/ \
	snapwatchdog/ \
	snapwebsites/ \
		>cppcheck.out 2>&1
