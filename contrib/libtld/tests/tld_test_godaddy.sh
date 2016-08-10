#!/bin/sh
set -e
VALIDATE=
if test -z "$1"
then
	for v in . .. ../.. build/src BUILD/src ../BUILD/src ../build/src ../BUILD/libtld/src ../build/libtld/src
	do
		if test -x $v/validate_tld
		then
			VALIDATE=$v/validate_tld
			break
		fi
	done
else
	if test -x $1/validate_tld
	then
		VALIDATE=$1/validate_tld
	fi
fi
if test -z "$VALIDATE"
then
	echo "error: could not find validate_tld"
	exit 1;
fi
for f in `sed -e '/^#/ d' godaddy-tlds.txt`
do
	#echo check $f
	$VALIDATE http://www.m2osw$f
done
