#!/bin/sh
# Statically check the code once in a while
# The result is saved in cppcheck.out (it can be quite large)
cppcheck \
	contrib/ \
	snapwebsites/ \
		>cppcheck.out 2>&1
