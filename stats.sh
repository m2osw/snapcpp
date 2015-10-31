#!/bin/sh

cloc --autoconf --by-file-by-lang \
	advgetopt \
	as2js \
	cassview \
	controlled_vars \
	csspp \
	googlerank \
	iplock \
	libQtCassandra \
	libQtSerialization \
	libtld \
	snapCMakeModules \
	snapmanager \
	snapwatchdog \
	snapwebsites \
	zipios \
		>statistics.txt

