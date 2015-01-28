# - Try to find LibQREncode (libqrencode)
#
# Once done this will define
#
# LIBQRENCODE_FOUND        - System has libqrencode.so
# LIBQRENCODE_INCLUDE_DIR  - The qrencode include directories
# LIBQRENCODE_LIBRARY      - The libraries needed to use qrencode
# LIBQRENCODE_DEFINITIONS  - Compiler switches required for using qrencode (none)

find_path( LIBQRENCODE_INCLUDE_DIR qrencode.h
			PATHS $ENV{LIBQRENCODE_INCLUDE_DIR}
		 )
find_library( LIBQRENCODE_LIBRARY qrencode
			PATHS $ENV{LIBQRENCODE_LIBRARY}
		 )
mark_as_advanced( LIBQRENCODE_INCLUDE_DIR LIBQRENCODE_LIBRARY )

set( LIBQRENCODE_INCLUDE_DIRS ${LIBQRENCODE_INCLUDE_DIR} )
set( LIBQRENCODE_LIBRARIES    ${LIBQRENCODE_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBQRENCODE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibQRENCODE DEFAULT_MSG LIBQRENCODE_INCLUDE_DIR LIBQRENCODE_LIBRARY )
