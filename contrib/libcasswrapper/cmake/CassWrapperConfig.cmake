# - Try to find CassWrapper
#
# Once done this will define
#
# CASSWRAPPER_FOUND        - System has CassWrapper
# CASSWRAPPER_INCLUDE_DIR  - The CassWrapper include directories
# CASSWRAPPER_LIBRARY      - The libraries needed to use CassWrapper (none)
# CASSWRAPPER_DEFINITIONS  - Compiler switches required for using CassWrapper (none)

find_path( CASSWRAPPER_INCLUDE_DIR casswrapper/session.h
		   PATHS $ENV{CASSWRAPPER_INCLUDE_DIR}
		   PATH_SUFFIXES casswrapper
		 )
	 find_library( CASSWRAPPER_LIBRARY casswrapper
			PATHS $ENV{CASSWRAPPER_LIBRARY}
		 )
mark_as_advanced( CASSWRAPPER_INCLUDE_DIR CASSWRAPPER_LIBRARY )

set( CASSWRAPPER_INCLUDE_DIRS ${CASSWRAPPER_INCLUDE_DIR} )
set( CASSWRAPPER_LIBRARIES    ${CASSWRAPPER_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set CASSWRAPPER_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( CassWrapper DEFAULT_MSG CASSWRAPPER_INCLUDE_DIR CASSWRAPPER_LIBRARY )
