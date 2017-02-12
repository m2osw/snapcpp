#!/bin/sh
set -e
if test "$1" = "-d"
then
	. dev/version
	rm -f ../../../BUILD/contrib/csspp/doc/csspp-doc-${VERSION}.tar.gz
	make -C ../../../BUILD/contrib/csspp/ csspp_Documentation
elif test "$1" = "-t"
then
	make -C ../../../BUILD/contrib/csspp/
	shift
	TEST="$1"
	if test -n "$TEST"
	then
		shift
		echo run with \"$TEST\"
	fi
	../../../BUILD/contrib/csspp/tests/csspp_tests --scripts scripts --version-script ../../../BUILD/contrib/csspp/scripts "$TEST" $*
elif test "$1" = "-i"
then
	make -C ../../../BUILD/contrib/csspp install
elif test "$1" = "-c"
then
	if test -z "$2"
	then
		echo "the -c option requires a second option with the name of the tag"
		exit 1
	fi
	make -C ../../../BUILD/contrib/csspp install
	../../../BUILD/contrib/csspp/tests/csspp_tests --scripts ../../../BUILD/dist/lib/csspp/scripts --show-errors "[$2]" 2>&1 | less
else
	make -C ../../../BUILD/contrib/csspp
fi
