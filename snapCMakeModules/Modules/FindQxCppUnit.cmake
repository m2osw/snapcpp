# - Try to find LibQxCppUnit (libqxcppunit)
#
# Once done this will define
#
# LIBQXCPPUNIT_FOUND        - System has LibQxCppUnit
# LIBQXCPPUNIT_INCLUDE_DIR  - The LibQxCppUnit include directories
# LIBQXCPPUNIT_LIBRARY      - The libraries needed to use LibQxCppUnit (none)
# LIBQXCPPUNIT_DEFINITIONS  - Compiler switches required for using LibQxCppUnit (none)

find_path( LIBQXCPPUNIT_INCLUDE_DIR testrunner.h
			PATHS $ENV{LIBQXCPPUNIT_INCLUDE_DIR}
			PATH_SUFFIXES qxcppunit
)
find_library( LIBQXCPPUNIT_LIBRARY qxcppunitd
			PATHS $ENV{LIBQXCPPUNIT_LIBRARY}
)
mark_as_advanced( LIBQXCPPUNIT_INCLUDE_DIR LIBQXCPPUNIT_LIBRARY )

set( LIBQXCPPUNIT_INCLUDE_DIRS ${LIBQXCPPUNIT_INCLUDE_DIR} )
set( LIBQXCPPUNIT_LIBRARIES    ${LIBQXCPPUNIT_LIBRARY} )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBQXCPPUNIT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibQXCPPUNIT DEFAULT_MSG LIBQXCPPUNIT_INCLUDE_DIR LIBQXCPPUNIT_LIBRARY )
