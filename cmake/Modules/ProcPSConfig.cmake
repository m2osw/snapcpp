# - Try to find LibProcPS (libprocps3)
#
# Once done this will define
#
# LIBPROCPS_FOUND        - System has LibProcPS
# LIBPROCPS_INCLUDE_DIR  - The LibProcPS include directories
# LIBPROCPS_LIBRARY      - The libraries needed to use LibProcPS (none)
# LIBPROCPS_DEFINITIONS  - Compiler switches required for using LibProcPS (none)

find_path( LIBPROCPS_INCLUDE_DIR readproc.h
			PATHS $ENV{LIBPROCPS_INCLUDE_DIR}
			PATH_SUFFIXES proc
		 )
find_library( LIBPROCPS_LIBRARY procps
			PATHS $ENV{LIBPROCPS_LIBRARY}
		 )
mark_as_advanced( LIBPROCPS_INCLUDE_DIR LIBPROCPS_LIBRARY )

set( LIBPROCPS_INCLUDE_DIRS ${LIBPROCPS_INCLUDE_DIR} )
set( LIBPROCPS_LIBRARIES    ${LIBPROCPS_LIBRARY} )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBPROCPS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibPROCPS DEFAULT_MSG LIBPROCPS_INCLUDE_DIR LIBPROCPS_LIBRARY )
