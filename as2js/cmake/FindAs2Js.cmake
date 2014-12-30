# - Try to find As2Js
#
# Once done this will define
#
# AS2JS_FOUND        - System has As2Js
# AS2JS_INCLUDE_DIRS - The As2Js include directories
# AS2JS_LIBRARIES    - The libraries needed to use As2Js (none)
# AS2JS_DEFINITIONS  - Compiler switches required for using As2Js (none)

find_path( AS2JS_INCLUDE_DIR as2js/as2js.h
    PATHS $ENV{AS2JS_INCLUDE_DIR}
    PATH_SUFFIXES asj2s
)
message( "Include Dir" )
message( ${AS2JS_INCLUDE_DIR} )

find_library( AS2JS_LIBRARY as2js
    PATHS $ENV{AS2JS_LIBRARY}
)
message( "Library Dir" )
message( ${AS2JS_LIBRARY} )

mark_as_advanced( AS2JS_INCLUDE_DIR AS2JS_LIBRARY )

set( AS2JS_INCLUDE_DIRS ${AS2JS_INCLUDE_DIR} )
set( AS2JS_LIBRARIES    ${AS2JS_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set AS2JS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( as2js DEFAULT_MSG AS2JS_INCLUDE_DIR AS2JS_LIBRARY )

# vim: ts=4 sw=4 et
