#!/bin/sh -e
#

VERBOSE=0
while test -n "$1"
do
	case "$1" in
	"--help"|"-h")
		echo "Usage: $0 [--opts]"
		echo "where --opts is one or more of the following:"
		echo "  -h | --help       print out this help screen"
		echo "  -v | --verbose    print out additional messages"
		exit 1
		;;

	"--verbose"|"-v")
		VERBOSE=1
		;;

	*)
		echo "error: unknown option \"$1\"."
		exit 1
		;;

	esac
done

# Go to the root folder
#
if test -d ../contrib
then
	cd ..
elif test -d ../../contrib
then
	cd ../..
elif ! test -d contrib
then
	echo "error: could not find the contrib/... directory."
	exit 1
fi

# first check .cpp file, they should all include <poison.h>
# except in libexcept which is a lower dependency project than snapdev
EXIT_CODE=0
for f in `find . \
	   -path ./BUILD -prune -false \
	-o -path ./contrib/zipios -prune -false \
	-o -path ./tmp -prune -false \
	-o -path ./.git -prune -false \
	-o -path ./snapwebsites -prune -false \
	-o \( -name '*.cpp' \
		-a ! -name 'bloomfilter.cpp' \
		-a ! -name 'forge_tcp.cpp' \
		-a ! -name 'hide_warnings.cpp' \
		-a ! -name 'license_gpl2.cpp' \
		-a ! -name 'mmap.cpp' \) | sort`
do
	if test "$VERBOSE" = "1"
	then
		echo "--- checking \"$f\""
	fi
	DIR=`dirname -- $f`
	PROJECT=`basename -- ${DIR}`
	if test "${PROJECT}" = "libexcept"
	then
		continue
	fi

	if grep --silent '^#include    <snapdev/poison.h>$' "${f}"
	then
		continue
	fi

	echo "error: file \"${f}\" does not #include <snapdev/poison.h>"
	EXIT_CODE=1
done

