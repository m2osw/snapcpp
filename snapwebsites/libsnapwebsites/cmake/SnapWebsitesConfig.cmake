# - Try to find SnapWebsites
#
# Once done this will define
#
# SNAPWEBSITES_FOUND        - System has SnapWebsites
# SNAPWEBSITES_INCLUDE_DIR  - The SnapWebsites include directories
# SNAPWEBSITES_LIBRARY      - The libraries needed to use SnapWebsites (none)
# SNAPWEBSITES_DEFINITIONS  - Compiler switches required for using SnapWebsites (none)

# Search include directory
find_path( SNAPWEBSITES_INCLUDE_DIR snapwebsites/snap_version.h
    PATHS $ENV{SNAPWEBSITES_INCLUDE_DIR}
    PATH_SUFFIXES snapwebsites
)

# Search library
find_library( SNAPWEBSITES_LIBRARY snapwebsites
    PATHS $ENV{SNAPWEBSITES_LIBRARY}
)

# Mark as important
mark_as_advanced( SNAPWEBSITES_INCLUDE_DIR SNAPWEBSITES_LIBRARY )

# Define the plurial versions
set( SNAPWEBSITES_INCLUDE_DIRS ${SNAPWEBSITES_INCLUDE_DIR} ${SNAPWEBSITES_INCLUDE_DIR}/snapwebsites )
set( SNAPWEBSITES_LIBRARIES    ${SNAPWEBSITES_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set SNAPWEBSITES_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( SnapWebsites DEFAULT_MSG SNAPWEBSITES_INCLUDE_DIR SNAPWEBSITES_LIBRARY )

# Make sure default DTD files are pointed to...
#
set( DTD_SOURCE_PATH /usr/share/snapwebsites/dtd CACHE PATH "Default DTD source files." )
#
set( CMAKE_MODULE_PATH ${SnapWebsites_DIR} ${CMAKE_MODULE_PATH} )
include( SnapCssLint   )
include( SnapJsLint    )
include( SnapXmlLint   )
include( SnapZipLayout )

# vim: ts=4 sw=4 et
