# - Try to find SnapWebsites
#
# Once done this will define
#
# SNAPWEBSITES_FOUND        - System has SnapWebsites
# SNAPWEBSITES_INCLUDE_DIR  - The SnapWebsites include directories
# SNAPWEBSITES_LIBRARY      - The libraries needed to use SnapWebsites (none)
# SNAPWEBSITES_DEFINITIONS  - Compiler switches required for using SnapWebsites (none)

find_path( SNAPWEBSITES_INCLUDE_DIR snapwebsites/snap_version.h
		   PATHS $ENV{SNAPWEBSITES_INCLUDE_DIR}
		   PATH_SUFFIXES snapwebsites
		 )
find_library( SNAPWEBSITES_LIBRARY libsnapwebsites
			PATHS $ENV{SNAPWEBSITES_LIBRARY}
		 )
mark_as_advanced( SNAPWEBSITES_INCLUDE_DIR SNAPWEBSITES_LIBRARY )

set( SNAPWEBSITES_INCLUDE_DIRS ${SNAPWEBSITES_INCLUDE_DIR} )
set( SNAPWEBSITES_LIBRARIES    ${SNAPWEBSITES_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set SNAPWEBSITES_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( SnapWebsites DEFAULT_MSG SNAPWEBSITES_INCLUDE_DIR SNAPWEBSITES_LIBRARY )
