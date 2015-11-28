# - Try to find log4cplus
#
# Once done this will define
#
# LOG4CPLUS_FOUND         - System has log4cplus
# LOG4CPLUS_INCLUDE_DIRS  - The log4cplus include directories
# LOG4CPLUS_LIBRARIES     - The libraries needed to use log4cplus
# LOG4CPLUS_DEFINITIONS  - Compiler switches required for using log4cplus (none)

find_path( LOG4CPLUS_INCLUDE_DIR log4cplus/logger.h
                           HINTS $ENV{LOG4CPLUS_INCLUDE_DIR}
                   PATH_SUFFIXES log4cplus
)
find_library( LOG4CPLUS_LIBRARY log4cplus
                          HINTS $ENV{LOG4CPLUS_LIBRARY}
)
mark_as_advanced( LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY )

set( LOG4CPLUS_INCLUDE_DIRS ${LOG4CPLUS_INCLUDE_DIR} )
set( LOG4CPLUS_LIBRARIES    ${LOG4CPLUS_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LOG4CPLUS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( log4cplus DEFAULT_MSG LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY )

# vim: ts=4 sw=4 et
