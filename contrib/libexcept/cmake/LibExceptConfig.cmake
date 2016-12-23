# - Try to find LibExcept
#
# Once done this will define
#
# LIBEXCEPT_FOUND        - System has LibExcept
# LIBEXCEPT_INCLUDE_DIR  - The LibExcept include directories
# LIBEXCEPT_LIBRARY      - The libraries needed to use LibExcept (none)
# LIBEXCEPT_DEFINITIONS  - Compiler switches required for using LibExcept (none)

find_path( LIBEXCEPT_INCLUDE_DIR libexcept/exception.h
		   PATHS $ENV{LIBEXCEPT_INCLUDE_DIR}
		   PATH_SUFFIXES libexcept
		 )
find_library( LIBEXCEPT_LIBRARY except
		   PATHS $ENV{LIBEXCEPT_LIBRARY}
		 )
mark_as_advanced( LIBEXCEPT_INCLUDE_DIR LIBEXCEPT_LIBRARY )

set( LIBEXCEPT_INCLUDE_DIRS ${LIBEXCEPT_INCLUDE_DIR} )
set( LIBEXCEPT_LIBRARIES    ${LIBEXCEPT_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBEXCEPT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibExcept DEFAULT_MSG LIBEXCEPT_INCLUDE_DIR LIBEXCEPT_LIBRARY )
