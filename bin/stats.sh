#!/bin/sh

cloc --autoconf --by-file-by-lang \
	cmake \
	contrib \
	snapwebsites \
		>statistics.txt

