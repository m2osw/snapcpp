#!/bin/sh
#
for f in ../contrib/*
do
	# The dev/coverage of zipios still creates this folder...
	#
	if test "$f" = "BUILD"
	then
		continue
	fi

	if test -d $f
	then
		(
		cd $f
		echo "----------------"
		pwd
		git status .
		)
	fi
done


