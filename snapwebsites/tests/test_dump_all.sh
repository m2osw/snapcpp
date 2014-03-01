#!/bin/sh

# This script is used partially as a debug opportunity of the Cassandra
# cluster you are running; if you load all the data from a database, it
# gives you the opportunity to find a table.row.cell that has problems.

set -e

COUNT=1000
SNAPDB="../BUILD/snapwebsites/src/snapdb --host 127.0.0.1 --count $COUNT"

tables=`$SNAPDB`

for t in $tables
do
	echo "***"
	echo "*** Table: $t"
	echo "***"

	rows=`$SNAPDB $t`
	for r in $rows
	do
		echo "-- Row: $r"

		# in most cases problems with occur here
		$SNAPDB $t $r || exit 1
	done
done

