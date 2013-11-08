find_file( log4cplus_INCLUDE_FILE log4cplus/configurator.h )
if( ${log4cplus_INCLUDE_FILE} STREQUAL "log4cplus_INCLUDE_FILE-NOTFOUND" )
	message( FATAL_ERROR
endif()

find_library( log4cplus_LIBRARY log4cplus )
if( NOT log4cplus_FOUND )
	message( FATAL_ERROR "Cannot find the log4cplus library. Make sure it is installed and try again." )
endif()
