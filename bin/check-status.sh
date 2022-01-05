#!/bin/sh -e
#

LATEST=false
while test -n "$1"
do
	case "$1" in
		"--help"|"-h")
			echo "Usage: $0 [--opts]"
			echo "where --opts is one or more of the following:"
			echo "  --help | -h      print out this help screen"
			echo "  --latest | -l    pull the latest"
			exit 1
			;;

		"--latest"|"-l")
			shift
			LATEST=true
			;;

		*)
			echo "error: unknown option \"$1\"."
			exit 1
			;;

	esac
done


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
		if $LATEST
		then
			git checkout main
			git pull
		fi
		)
	fi
done


