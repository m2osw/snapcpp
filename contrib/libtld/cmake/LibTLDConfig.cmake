# - Try to find LibTLD (libtld)
#
# Once done this will define
#
# LIBTLD_FOUND        - System has LibTLD
# LIBTLD_INCLUDE_DIR  - The LibTLD include directories
# LIBTLD_LIBRARY      - The libraries needed to use LibTLD (none)
# LIBTLD_DEFINITIONS  - Compiler switches required for using LibTLD (none)

find_path( LIBTLD_INCLUDE_DIR libtld/tld.h
			PATHS $ENV{LIBTLD_INCLUDE_DIR}
			PATH_SUFFIXES libtld
		 )
find_library( LIBTLD_LIBRARY tld
			PATHS $ENV{LIBTLD_LIBRARY}
		 )
mark_as_advanced( LIBTLD_INCLUDE_DIR LIBTLD_LIBRARY )

set( LIBTLD_INCLUDE_DIRS ${LIBTLD_INCLUDE_DIR} )
set( LIBTLD_LIBRARIES    ${LIBTLD_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBTLD_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibTLD DEFAULT_MSG LIBTLD_INCLUDE_DIR LIBTLD_LIBRARY )
