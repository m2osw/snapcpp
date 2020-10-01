# - Try to find LibFastJournal
#
# Once done this will define
#
# LIBFASTJOURNAL_FOUND        - System has LibFastJournal
# LIBFASTJOURNAL_INCLUDE_DIR  - The LibFastJournal include directories
# LIBFASTJOURNAL_LIBRARY      - The libraries needed to use LibFastJournal (none)
# LIBFASTJOURNAL_DEFINITIONS  - Compiler switches required for using LibFastJournal (none)

find_path( LIBFASTJOURNAL_INCLUDE_DIR libfastjournal/libfastjournal.h
		   PATHS $ENV{LIBFASTJOURNAL_INCLUDE_DIR}
		 )
find_library( LIBFASTJOURNAL_LIBRARY fastjournal
		   PATHS $ENV{LIBFASTJOURNAL_LIBRARY}
		 )
mark_as_advanced( LIBFASTJOURNAL_INCLUDE_DIR LIBFASTJOURNAL_LIBRARY )

set( LIBFASTJOURNAL_INCLUDE_DIRS ${LIBFASTJOURNAL_INCLUDE_DIR} )
set( LIBFASTJOURNAL_LIBRARIES    ${LIBFASTJOURNAL_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set LIBFASTJOURNAL_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( LibFASTJOURNAL DEFAULT_MSG LIBFASTJOURNAL_INCLUDE_DIR LIBFASTJOURNAL_LIBRARY )
