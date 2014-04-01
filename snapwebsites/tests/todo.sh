#!/bin/sh
#
# Generally this script is called using make as follow:
#
#    make -C YOUR_BUILD_DIR/snapwebsites/ snap_code_analysis
#

OUTPUT=$1/todo.txt

echo "TODO: entries that need to be fixed before release 1.0" >$OUTPUT
find . -type f -exec grep TODO {} \; | wc -l >>$OUTPUT

echo "XXX: entries that are likely to be addressed quickly" >>$OUTPUT
find . -type f -exec grep XXX {} \; | wc -l >>$OUTPUT

echo "TBD: questions that need testing to be answered" >>$OUTPUT
find . -type f -exec grep TBD {} \; | wc -l >>$OUTPUT

echo "FIXME: things that we do not control inside snapwebsites (Especially the controlled_vars of enumerations)" >>$OUTPUT
find . -type f -exec grep FIXME {} \; | wc -l >>$OUTPUT

echo "todo: long term, nice to have things defined in Doxygen" >>$OUTPUT
find . -type f -exec grep "todo:" {} \; | wc -l >>$OUTPUT

