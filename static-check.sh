#!/bin/sh
# Statically check the code once in a while
# The result is saved in cppcheck.out (it can be quite large)
cppcheck \
	advgetopt/ \
	cassandra/ \
	cassview/ \
	controlled_vars/ \
	googlerank/ \
	iplock/ \
	launchpad/ \
	libQtCassandra/ \
	libQtSerialization/ \
	libtld/ \
	snapwebsites/ \
		>cppcheck.out 2>&1
