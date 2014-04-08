#!/bin/sh

OUTPUT="$1/js-lint"
mkdir -p $OUTPUT

#
# Disable:
#   0002 -- Missing space before "("
#   0110 -- Line too long (over 80 characters)
#   0131 -- Single-quoted string preferred over double-quoted string.
#

OPTIONS="--disable 0002,0100,0110,0120,0131,0222 --jslint_error=blank_lines_at_top_level --jslint_error=unused_private_members"

for j in `find . -type f -name '*.js'`
do
	# Save the output in our analysis output directory
	echo "gjslint $OPTIONS $j >$OUTPUT/`basename $j .js`.txt 2>&1"
	if    gjslint $OPTIONS $j >$OUTPUT/`basename $j .js`.txt 2>&1
	then
		# if no errors, delete the file... (no need!)
		rm $OUTPUT/`basename $j .js`.txt
	fi
done

echo JavaScript lint done.

