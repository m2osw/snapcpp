# - Try to find ControlledVars (controlled_vars)
#
# Once done this will define
#
# CONTROLLEDVARS_FOUND        - System has ControlledVars
# CONTROLLEDVARS_INCLUDE_DIR  - The ControlledVars include directories
# CONTROLLEDVARS_LIBRARY      - The libraries needed to use ControlledVars (none)
# CONTROLLEDVARS_DEFINITIONS  - Compiler switches required for using ControlledVars (none)

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
	set( CONTROLLEDVARS_INCLUDE_DIR ${controlled_vars_BINARY_DIR} )
else()
	find_path( CONTROLLEDVARS_INCLUDE_DIR controlled_vars/controlled_vars_auto_init.h
			   PATHS $ENV{CONTROLLEDVARS_INCLUDE_DIR}
			   PATH_SUFFIXES controlled_vars
			 )
	mark_as_advanced( CONTROLLEDVARS_INCLUDE_DIR )
endif()

set( CONTROLLEDVARS_INCLUDE_DIRS ${CONTROLLEDVARS_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set CONTROLLEDVARS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( ControlledVars DEFAULT_MSG CONTROLLEDVARS_INCLUDE_DIR )
