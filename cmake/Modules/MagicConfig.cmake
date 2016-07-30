# - Try to find the magic library and headers
#
# The magic library is used to check the type of a file from its contents.
#
# Once done this will define
#
# MAGIC_FOUND        - System has Magick
# MAGIC_INCLUDE_DIRS - The magick include directories
# MAGIC_LIBRARIES    - The libraries needed to use magick
# MAGIC_DEFINITIONS  - Compiler switches required for using magick

find_file( MAGIC magic.h )
if( ${MAGIC} STREQUAL "MAGIC-NOTFOUND" )
    message( FATAL_ERROR "Please install libmagic-dev!" )
endif()
get_filename_component( MAGIC_INCLUDE_PATH ${MAGIC} PATH )

find_library( MAGIC_LIBRARY magic )
if( ${MAGIC_LIBRARY} STREQUAL "MAGIC_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Please install libmagic-dev!" )
endif()



find_path( MAGIC_INCLUDE_DIR magic.h
                        PATHS $ENV{MAGIC_INCLUDE_DIR}
         )

find_library( MAGIC_LIBRARY event
                      PATHS $ENV{MAGIC_LIBRARY}
            )

mark_as_advanced( MAGIC_INCLUDE_DIR MAGIC_LIBRARY )

set( MAGIC_INCLUDE_DIRS ${MAGIC_INCLUDE_DIR} )
set( MAGIC_LIBRARIES    ${MAGIC_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set MAGIC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( MAGIC DEFAULT_MSG MAGIC_INCLUDE_DIR MAGIC_LIBRARY )

# vim: ts=4 sw=4 et
