#!/bin/sh
#
for f in ../contrib/*
do
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


