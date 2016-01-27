# - Try to find LibQxRunner (libqxrunner)
#
# Once done this will define
#
# LIBQXRUNNER_FOUND        - System has LibQxCppUnit
# LIBQXRUNNER_INCLUDE_DIR  - The LibQxCppUnit include directories
# LIBQXRUNNER_LIBRARY      - The libraries needed to use LibQxCppUnit (none)
# LIBQXRUNNER_DEFINITIONS  - Compiler switches required for using LibQxCppUnit (none)

find_path( LIBQXRUNNER_INCLUDE_DIR runner.h
			PATHS $ENV{LIBQXRUNNER_INCLUDE_DIR}
			PATH_SUFFIXES qxrunner
		 )
find_library( LIBQXRUNNER_LIBRARY qxrunnerd
			PATHS $ENV{LIBQXRUNNER_LIBRARY}
		 )
mark_as_advanced( LIBQXRUNNER_INCLUDE_DIR LIBQXRUNNER_LIBRARY )

set( LIBQXRUNNER_INCLUDE_DIRS ${LIBQXRUNNER_INCLUDE_DIR} )
set( LIBQXRUNNER_LIBRARIES    ${LIBQXRUNNER_LIBRARY} )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBQXRUNNER_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibQXRUNNER DEFAULT_MSG LIBQXRUNNER_INCLUDE_DIR LIBQXRUNNER_LIBRARY )
