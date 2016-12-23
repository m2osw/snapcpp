# - Try to find CassValue
#
# Once done this will define
#
# CASSVALUE_FOUND        - System has CassValue
# CASSVALUE_INCLUDE_DIR  - The CassValue include directories
# CASSVALUE_LIBRARY      - The libraries needed to use CassValue (none)
# CASSVALUE_DEFINITIONS  - Compiler switches required for using CassValue (none)

find_path( CASSVALUE_INCLUDE_DIR cassvalue/value.h
		   PATHS $ENV{CASSVALUE_INCLUDE_DIR}
		   PATH_SUFFIXES cassvalue
		 )
	 find_library( CASSVALUE_LIBRARY cassvalue
			PATHS $ENV{CASSVALUE_LIBRARY}
		 )
mark_as_advanced( CASSVALUE_INCLUDE_DIR CASSVALUE_LIBRARY )

set( CASSVALUE_INCLUDE_DIRS ${CASSVALUE_INCLUDE_DIR} )
set( CASSVALUE_LIBRARIES    ${CASSVALUE_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set CASSVALUE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( CassValue DEFAULT_MSG CASSVALUE_INCLUDE_DIR CASSVALUE_LIBRARY )
