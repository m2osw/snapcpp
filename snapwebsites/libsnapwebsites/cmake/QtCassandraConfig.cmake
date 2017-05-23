# - Try to find QtCassandra
#
# Once done this will define
#
# QTCASSANDRA_FOUND        - System has QtCassandra
# QTCASSANDRA_INCLUDE_DIR  - The QtCassandra include directories
# QTCASSANDRA_LIBRARY      - The libraries needed to use QtCassandra (none)
# QTCASSANDRA_DEFINITIONS  - Compiler switches required for using QtCassandra (none)

find_path( QTCASSANDRA_INCLUDE_DIR QtCassandra/QCassandra.h
		   PATHS $ENV{QTCASSANDRA_INCLUDE_DIR}
		   PATH_SUFFIXES QtCassandra
		 )
find_library( QTCASSANDRA_LIBRARY QtCassandra
			PATHS $ENV{QTCASSANDRA_LIBRARY}
		 )
mark_as_advanced( QTCASSANDRA_INCLUDE_DIR QTCASSANDRA_LIBRARY )

# We don't set these, but piggy back on SNAPWEBSITES_<BLAH> variables.
# This is important because the linker requires QtCassandra *after*
# the snap library.
#
#set( QTCASSANDRA_INCLUDE_DIRS ${QTCASSANDRA_INCLUDE_DIR} )
#set( QTCASSANDRA_LIBRARIES    ${QTCASSANDRA_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set QTCASSANDRA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( QtCassandra DEFAULT_MSG QTCASSANDRA_INCLUDE_DIR QTCASSANDRA_LIBRARY )
