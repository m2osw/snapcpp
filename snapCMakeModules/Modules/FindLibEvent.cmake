# - Try to find libevent2
#
# Once done this will define
#
# LIBEVENT2_FOUND        - System has libevent2
# LIBEVENT2_INCLUDE_DIR  - The libevent2 include directories
# LIBEVENT2_LIBRARIES    - The libraries needed to use libevent2
# LIBEVENT2_DEFINITIONS  - Compiler switches required for using libevent2

find_path( LIBEVENT2_INCLUDE_DIR event2/event.h
			PATHS $ENV{LIBEVENT2_INCLUDE_DIR}
			PATH_SUFFIXES event2
		 )
find_library( LIBEVENT2_LIBRARY event
			PATHS $ENV{LIBEVENT2_LIBRARY}
		 )
mark_as_advanced( LIBEVENT2_INCLUDE_DIR LIBEVENT2_LIBRARY )

set( LIBEVENT2_INCLUDE_DIRS ${LIBEVENT2_INCLUDE_DIR} )
set( LIBEVENT2_LIBRARIES    ${LIBEVENT2_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibEVENT DEFAULT_MSG LIBEVENT2_INCLUDE_DIR LIBEVENT2_LIBRARY )
