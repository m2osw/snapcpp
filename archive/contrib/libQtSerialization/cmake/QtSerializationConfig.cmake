# - Try to find QtSerialization
#
# Once done this will define
#
# QTSERIALIZATION_FOUND        - System has QtSerialization
# QTSERIALIZATION_INCLUDE_DIRS - The QtSerialization include directories
# QTSERIALIZATION_LIBRARIES    - The libraries needed to use QtSerialization
# QTSERIALIZATION_DEFINITIONS  - Compiler switches required for using QtSerialization (none)

find_path( QTSERIALIZATION_INCLUDE_DIR QtSerialization/QSerialization.h
		   PATHS $ENV{QTSERIALIZATION_INCLUDE_DIR}
		   PATH_SUFFIXES QtSerialization
		 )
find_library( QTSERIALIZATION_LIBRARY QtSerialization
			PATHS $ENV{QTSERIALIZATION_LIBRARY}
		 )
mark_as_advanced( QTSERIALIZATION_INCLUDE_DIR QTSERIALIZATION_LIBRARY )

set( QTSERIALIZATION_INCLUDE_DIRS ${QTSERIALIZATION_INCLUDE_DIR} )
set( QTSERIALIZATION_LIBRARIES    ${QTSERIALIZATION_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set QTSERIALIZATION_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( QtSerialization DEFAULT_MSG QTSERIALIZATION_INCLUDE_DIR QTSERIALIZATION_LIBRARY )
