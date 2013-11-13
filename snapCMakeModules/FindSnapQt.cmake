# - Try to find Qt4
#
# Once done this will define
#
# SNAPQT_FOUND        - System has Qt4
# SNAPQT_INCLUDE_DIRS - The Qt4 include directories
# SNAPQT_LIBRARIES     - The libraries needed to use Qt4 (none)
# SNAPQT_DEFINITIONS  - Compiler switches required for using Qt4 (none)

find_package( Qt4 ${SnapQt4_FIND_VERSION} QUIET REQUIRED ${SnapQt4_FIND_COMPONENTS} )
#include( ${QT_USE_FILE} )
#add_definitions( ${QT_DEFINITIONS} )

set( SNAPQT_INCLUDE_DIRS ${QT_INCLUDES}    CACHE STRING "" )
set( SNAPQT_LIBRARIES    ${QT_LIBRARIES}   CACHE STRING "" )
set( SNAPQT_DEFINITIONS  ${QT_DEFINITIONS} CACHE STRING "" )
set( SNAPQT_USE_FILE     ${QT_USE_FILE}    CACHE STRING "" )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LOG4CPLUS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( SnapQt DEFAULT_MSG SNAPQT_INCLUDE_DIRS SNAPQT_LIBRARIES )

mark_as_advanced( SNAPQT_INCLUDE_DIRS SNAPQT_LIBRARIES SNAPQT_DEFINITIONS SNAPQT_USE_FILE )

## Find Qt4 libraries on the local system
##
#macro( SnapIncludeQt4 )
#    find_package( Qt4 4.8.1 QUIET REQUIRED ${ARGN} )
#    include( ${QT_USE_FILE} )
#    add_definitions( ${QT_DEFINITIONS} )
#endmacro()

# vim: ts=4 sw=4 noet
