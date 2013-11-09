find_path( log4cplus_INCLUDE_PATH log4cplus )
if( ${log4cplus_INCLUDE_PATH} STREQUAL "log4cplus_INCLUDE_PATH-NOTFOUND" )
	message( FATAL_ERROR "Cannot find log4cplus header path." )
endif()

find_library( log4cplus_LIBRARY log4cplus )
if( ${log4cplus_LIBRARY} STREQUAL "log4cplus_LIBRARY-NOTFOUND" )
	message( FATAL_ERROR "Cannot find the log4cplus library. Make sure it is installed and try again." )
endif()
