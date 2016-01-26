# - Try to find AdvGetOpt (advgetopt)
#
# Once done this will define
#
# ADVGETOPT_FOUND        - System has AdvGetOpt
# ADVGETOPT_INCLUDE_DIR  - The AdvGetOpt include directories
# ADVGETOPT_LIBRARY      - The libraries needed to use AdvGetOpt (none)
# ADVGETOPT_DEFINITIONS  - Compiler switches required for using AdvGetOpt (none)

find_path( ADVGETOPT_INCLUDE_DIR advgetopt/advgetopt.h
			PATHS $ENV{ADVGETOPT_INCLUDE_DIR}
			PATH_SUFFIXES advgetopt
		 )
find_library( ADVGETOPT_LIBRARY advgetopt
			PATHS $ENV{ADVGETOPT_LIBRARY}
		 )
mark_as_advanced( ADVGETOPT_INCLUDE_DIR ADVGETOPT_LIBRARY )

set( ADVGETOPT_INCLUDE_DIRS ${ADVGETOPT_INCLUDE_DIR} ${ADVGETOPT_INCLUDE_DIR}/advgetopt )
set( ADVGETOPT_LIBRARIES    ${ADVGETOPT_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set ADVGETOPT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( AdvGetOpt DEFAULT_MSG ADVGETOPT_INCLUDE_DIR ADVGETOPT_LIBRARY )

# vim: ts=4 sw=4 noexpandtab nocindent
