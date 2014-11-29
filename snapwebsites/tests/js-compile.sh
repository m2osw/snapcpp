#!/bin/sh
#

OUTPUT="$1/js-compile"
mkdir -p $OUTPUT

#

OPTIONS="--warning_level VERBOSE"

       # At this point the extensions doesn't work right because the
       # expr is not defined in the jquery definitions
       #plugins/output/jquery-extensions.js

FILES="plugins/editor/editor.js
       plugins/form/form.js
       plugins/list/list/list.js
       plugins/listener/listener.js
       plugins/locale/locale-timezone.js
       plugins/output/output.js
       plugins/output/popup.js
       plugins/output/rotate-animation.js
       plugins/server_access/server-access.js"

CLOSURE_COMPILER=/home/snapwebsites/tmp/google-js-compiler/closure-compiler

for js in $FILES
do
    INLINE_OPTIONS_WITH_VARS=`sed \
        -e '0,/==ClosureCompiler==/ d' \
        -e '/==\/ClosureCompiler==/,$ d' \
        -e 's/.*@\(.*\)/--\1/' $js`

    INLINE_OPTIONS=`echo $INLINE_OPTIONS_WITH_VARS | sed \
        -e "s:\\\$CLOSURE_COMPILER:$CLOSURE_COMPILER:g"`

    cmd="java -jar ../tmp/google-js-compiler/compiler.jar --js_output_file $OUTPUT/`basename $js .js`.min.js $OPTIONS $INLINE_OPTIONS --js $js"
    echo $cmd
    $cmd
done

# TBD not too sure why but the compiler always returns an error...
exit 0
# vim: ts=4 sw=4 et
