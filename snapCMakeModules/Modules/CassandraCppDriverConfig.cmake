# - Try to find CassandraCppDriver (libprocps3)
#
# Once done this will define
#
# CASSANDRACPPDRIVER_FOUND        - System has CassandraCppDriver
# CASSANDRACPPDRIVER_INCLUDE_DIR  - The CassandraCppDriver include directories
# CASSANDRACPPDRIVER_LIBRARY      - The libraries needed to use CassandraCppDriver (none)
# CASSANDRACPPDRIVER_DEFINITIONS  - Compiler switches required for using CassandraCppDriver (none)

find_path( CASSANDRACPPDRIVER_INCLUDE_DIR cassandra.h
			PATHS $ENV{CASSANDRACPPDRIVER_INCLUDE_DIR}
			PATH_SUFFIXES proc
		 )
find_library( CASSANDRACPPDRIVER_LIBRARY cassandra
			PATHS $ENV{CASSANDRACPPDRIVER_LIBRARY}
		 )
mark_as_advanced( CASSANDRACPPDRIVER_INCLUDE_DIR CASSANDRACPPDRIVER_LIBRARY )

set( CASSANDRACPPDRIVER_INCLUDE_DIRS ${CASSANDRACPPDRIVER_INCLUDE_DIR} )
set( CASSANDRACPPDRIVER_LIBRARIES    ${CASSANDRACPPDRIVER_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set CASSANDRACPPDRIVER_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( CASSANDRACPPDRIVER DEFAULT_MSG CASSANDRACPPDRIVER_INCLUDE_DIR CASSANDRACPPDRIVER_LIBRARY )
