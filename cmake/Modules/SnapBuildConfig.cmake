# File:         FindSnapBuild.cmake
# Object:       Provide functions to build projects.
#
# Copyright:    Copyright (c) 2011-2017 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# If you have included Snap's Third Party package, then this module will
# point to that instead of trying to locate it on the system.
#
include( CMakeParseArguments )

find_program( FIND_DEPS_SCRIPT   SnapFindDeps.pl			   PATHS ${CMAKE_MODULE_PATH} )
find_program( INC_VERS_SCRIPT    SnapBuildIncVers.pl		   PATHS ${CMAKE_MODULE_PATH} )
find_program( INC_DEPS_SCRIPT    SnapBuildIncDeps.pl		   PATHS ${CMAKE_MODULE_PATH} )
find_program( MAKE_SOURCE_SCRIPT SnapBuildMakeSourcePackage.sh PATHS ${CMAKE_MODULE_PATH} )
find_program( MAKE_DPUT_SCRIPT   SnapBuildDputPackage.sh	   PATHS ${CMAKE_MODULE_PATH} )
find_program( MAKE_DEPS_SCRIPT   SnapMakeDepsCache.pl		   PATHS ${CMAKE_MODULE_PATH} )
find_program( PBUILDER_SCRIPT    SnapPBuilder.sh			   PATHS ${CMAKE_MODULE_PATH} )
find_program( REPREPRO_SCRIPT    SnapReprepro.sh			   PATHS ${CMAKE_MODULE_PATH} )


# RDB: Sun Jan  8 12:42:10 PST 2017
# Moved this to the top so it's done only once
#
set( THE_CMAKE_BUILD_TOOL ${CMAKE_BUILD_TOOL} )
if( ${CMAKE_VERSION} VERSION_GREATER 3.0.0 )
	execute_process( 
		COMMAND nproc
		#OUTPUT_VARIABLE NUM_PROCS
		OUTPUT_VARIABLE NUM_JOBS
	)
	string(STRIP NUM_JOBS NUM_JOBS)
	# http://stackoverflow.com/questions/2499070/gnu-make-should-the-number-of-jobs-equal-the-number-of-cpu-cores-in-a-system#2499574
	# The question is if we want to try num procs + 1.
	#
	#math( EXPR NUM_JOBS "${NUM_PROCS} + 1" )
	set( MAKEFLAGS "-j${NUM_JOBS}" CACHE STRING "Number of jobs make should run. CMake vers 3+ only!" )
	if( NOT "${MAKEFLAGS}" STREQUAL "-j1" OR "${MAKEFLAGS}" STREQUAL "" )
		set( THE_CMAKE_BUILD_TOOL ${CMAKE_COMMAND} -E env MAKEFLAGS=\"${MAKEFLAGS}\" ${CMAKE_BUILD_TOOL} )
	endif()
endif()

# RDB: Tue Jan 26 10:13:24 PST 2016
# Adding ability to take the DEBUILD_EMAIL var from the environment as a default
#
# RDB: Sun Jul 31 18:44:37 PDT 2016
# Moved this to the top of the file.
#
set( DEBUILD_EMAIL_DEFAULT "Build Server <build@m2osw.com>" )
if( DEFINED ENV{DEBEMAIL} )
	set( DEBUILD_EMAIL_DEFAULT $ENV{DEBEMAIL} )
endif()
set( DEBUILD_PLATFORM "xenial"                         CACHE STRING   "Name of the Debian/Ubuntu platform to build against." )
set( DEBUILD_EMAIL    "${DEBUILD_EMAIL_DEFAULT}"       CACHE STRING   "Email address of the package signer."                 )
set( DEP_CACHE_FILE   "${CMAKE_BINARY_DIR}/deps.cache" CACHE INTERNAL "Cache file for dependencies."                         )
set( REPO_ROOT_DIR    "/var/www/debian/nightly"        CACHE PATH     "Root level where the repository will go."             )

message( STATUS "Scanning all projects..." )
execute_process( 
	COMMAND ${MAKE_DEPS_SCRIPT} ${CMAKE_SOURCE_DIR} ${DEP_CACHE_FILE}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

option( INCREMENT_BUILD_NUMBERS "Increment the build number in the version of all debian/changelog files." OFF )
if( ${INCREMENT_BUILD_NUMBERS} )
	add_custom_target(
		snap-incvers
		COMMAND ${INC_VERS_SCRIPT} ${DEP_CACHE_FILE} ${DEBUILD_PLATFORM}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Incrementing build version for all debian packages."
	)
else()
	add_custom_target(
		snap-incvers
		COMMENT "Incrementing build version number skipped."
	)
endif()

option( INCREMENT_DEPENDENCIES "Increment ALL dependencies in all packages." OFF )
if( ${INCREMENT_DEPENDENCIES} )
	add_custom_target(
		snap-incdeps
		COMMAND ${INC_DEPS_SCRIPT} ${DEP_CACHE_FILE} ${DEBUILD_PLATFORM}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		DEPENDS snap-incvers
		COMMENT "Incrementing dependencies for all debian packages."
	)
else()
	add_custom_target(
		snap-incdeps
		DEPENDS snap-incvers
		COMMENT "Incrementing dependencies skipped."
	)
endif()

function( ConfigureMakeProjectInternal )
	set( options        USE_CONFIGURE_SCRIPT IGNORE_DEPS )
	set( oneValueArgs   PROJECT_NAME TARGET_NAME DISTFILE_PATH COMPONENT )
	set( multiValueArgs CONFIG_ARGS )
	cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
	#
	if( NOT ARG_PROJECT_NAME )
		message( FATAL_ERROR "You must specify PROJECT_NAME!" )
	endif()
	if( NOT ARG_TARGET_NAME )
		message( FATAL_ERROR "You must specify TARGET_NAME!" )
	endif()
    if( NOT ARG_COMPONENT )
        message( FATAL_ERROR "You must specify COMPONENT (main, config, non-free, etc)!" )
    endif()

	set( SRC_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_PROJECT_NAME} )
	set( BUILD_DIR      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_PROJECT_NAME} )
	set( SNAP_DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Destination installation folder." )

	if( NOT EXISTS ${SRC_DIR} AND NOT ARG_DISTFILE_PATH )
		message( FATAL_ERROR "No source directory '${SRC_DIR}'!" )
	endif()

	if( NOT EXISTS ${SRC_DIR} AND ARG_DISTFILE_PATH )
		message( STATUS "Unpacking ${ARG_PROJECT_NAME} source distribution file into local build area." )
		file( MAKE_DIRECTORY ${SRC_DIR} )
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E tar xzf ${ARG_DISTFILE_PATH}
			WORKING_DIRECTORY ${SRC_DIR}
		)
		execute_process(
			COMMAND chmod a+x ${BUILD_DIR}/configure
			WORKING_DIRECTORY ${BUILD_DIR}
		)
	endif()

	if( NOT EXISTS ${BUILD_DIR} )
		file( MAKE_DIRECTORY ${BUILD_DIR} )
	endif()

	set( THE_CMAKE_COMMAND ${CMAKE_COMMAND} )
	if( ${CMAKE_VERSION} VERSION_GREATER 3.0.0 )
		option( USE_DISTCC "Use distcc to distribute the build. CMake vers 3+ only!" OFF )
		if( ${USE_DISTCC} )
			set( THE_CMAKE_COMMAND ${CMAKE_COMMAND} -E env CC=\"distcc gcc\" CXX=\"distcc g++\" ${CMAKE_COMMAND} )
		endif()
	endif()

	# RDB: Sun Jul 31 22:29:18 PDT 2016
	#
	# Now detect automatically all dependencies to the current project.
	# This uses the information in each debian/control file to sort out the
	# build dependencies.
	#
	if( ARG_IGNORE_DEPS )
		unset( DEPENDS_LIST )
	else()
		get_property( DEPENDS_LIST
			GLOBAL PROPERTY ${ARG_PROJECT_NAME}_DEPENDS_LIST
		)
	endif()
	if( ARG_USE_CONFIGURE_SCRIPT )
		set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${BUILD_DIR}/configure --prefix=${SNAP_DIST_DIR} ${ARG_CONFIG_ARGS}
				1> ${BUILD_DIR}/configure.log
				2> ${BUILD_DIR}/configure.err
			DEPENDS ${DEPENDS_LIST}
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_TARGET_NAME} configure script..."
		)
	else()
		set( SNAP_CMAKE_GENERATOR "CodeBlocks - Unix Makefiles" CACHE STRING "CMake generator to use to configure all projects. Defaults to CodeBlocks, which is qtcreator friendly." )
		set( BUILD_TYPE ${CMAKE_BUILD_TYPE} )
		if( NOT BUILD_TYPE )
			set( BUILD_TYPE Release )
		endif()
		unset( SANITIZE_ADDRESS_OPT )
		if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
			option( SANITIZE_ADDRESS "Sanitize addresses for ${PROJECT_NAME}. Debug mode only." OFF )
			if( ${SANITIZE_ADDRESS} )
				set( SANITIZE_ADDRESS_OPT "-DSANTIZE_ADDRESS:BOOL=TRUE" )
			endif()
		endif()
		set( COMMAND_LIST
			${THE_CMAKE_COMMAND}
				-G "${SNAP_CMAKE_GENERATOR}"
				-DCMAKE_BUILD_TYPE=${BUILD_TYPE}
				-DCMAKE_INSTALL_PREFIX:PATH="${SNAP_DIST_DIR}"
				-DCMAKE_PREFIX_PATH:PATH="${SNAP_DIST_DIR}"
				${SANITIZE_ADDRESS_OPT}
				${ARG_CONFIG_ARGS}
				${SRC_DIR}
		)
		set( CMD_FILE ${BUILD_DIR}/configure.cmd )
		file( REMOVE ${CMD_FILE} )
		file( APPEND ${CMD_FILE} "cd " ${BUILD_DIR} "\n" )
		file( APPEND ${CMD_FILE} "rm -f CMakeCache.txt\n" )
		foreach( line ${COMMAND_LIST} )
			if( "${line}" STREQUAL "${SNAP_CMAKE_GENERATOR}")
				file( APPEND ${CMD_FILE} "\"${SNAP_CMAKE_GENERATOR}\" \\\n" )
			else()
				file( APPEND ${CMD_FILE} ${line} " \\\n" )
			endif()
		endforeach()
		#
		set( CONFIGURE_TARGETS ${BUILD_DIR}/CMakeCache.txt  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${COMMAND_LIST}
				1> ${BUILD_DIR}/configure.log
				2> ${BUILD_DIR}/configure.err
			DEPENDS ${DEPENDS_LIST}
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_TARGET_NAME} CMake configuration..."
			)
	endif()

	set( MAKE_OUTPUT ${BUILD_DIR}/make.completed )
	option( BUILD_ONCE "Build projects once depending on 'make.completed' file" ON )
	unset( MAKE_OUTPUT_COMMAND )
	if( ${BUILD_ONCE} )
		set( MAKE_OUTPUT_COMMAND COMMAND echo > ${MAKE_OUTPUT} )
	endif()
	add_custom_command(
		OUTPUT ${MAKE_OUTPUT}
		COMMAND ${THE_CMAKE_BUILD_TOOL}
			1> ${BUILD_DIR}/make.log
			2> ${BUILD_DIR}/make.err
		COMMAND ${CMAKE_BUILD_TOOL} install
			1>> ${BUILD_DIR}/make.log
			2>> ${BUILD_DIR}/make.err
		${MAKE_OUTPUT_COMMAND}
		DEPENDS ${CONFIGURE_TARGETS}
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Building ${ARG_TARGET_NAME}"
	)
	add_custom_target(
		${ARG_TARGET_NAME}-make
		DEPENDS ${MAKE_OUTPUT}
		WORKING_DIRECTORY ${BUILD_DIR}
	)

	#set( MAKE_TARGET ${BUILD_DIR}/MAKE_BUILD_SUCCESSFUL  )
	#add_custom_command(
	#    OUTPUT ${MAKE_TARGET}
	#    COMMAND echo > ${MAKE_TARGET}
	#    DEPENDS ${ARG_TARGET_NAME}-make
	#    WORKING_DIRECTORY ${BUILD_DIR}
	#    COMMENT "Target ${ARG_TARGET_NAME} successfully built!" 
	#    )

	unset( PBUILDER_DEPS )
	unset( DPUT_DEPS     )
	if( DEPENDS_LIST )
		foreach( DEP ${DEPENDS_LIST} )
			list( APPEND PBUILDER_DEPS ${DEP}-pbuilder )
			list( APPEND DPUT_DEPS     ${DEP}-dput     )
		endforeach()
	endif()
	#
	set( EMAIL_ADDY "${DEBUILD_EMAIL}" )
	separate_arguments( EMAIL_ADDY )
	#
	if( ARG_IGNORE_DEPS )
		unset( SNAPINCDEPS )
	else()
		set( SNAPINCDEPS snap-incdeps )
	endif()
	add_custom_target(
		${ARG_TARGET_NAME}-debuild
		COMMAND env DEBEMAIL="${EMAIL_ADDY}" ${MAKE_SOURCE_SCRIPT} ${DEBUILD_PLATFORM}
			1> ${BUILD_DIR}/debuild.log
		WORKING_DIRECTORY ${SRC_DIR}
		DEPENDS ${SNAPINCDEPS}
		COMMENT "Building debian source package for ${ARG_TARGET_NAME}"
		)
	add_custom_target(
		${ARG_TARGET_NAME}-dput
		COMMAND ${MAKE_DPUT_SCRIPT}
			1> ${BUILD_DIR}/dput.log
		DEPENDS ${DPUT_DEPS}
		WORKING_DIRECTORY ${SRC_DIR}
		COMMENT "Dputting debian package ${ARG_TARGET_NAME} to launchpad."
		)
	set( PBUILD_OUTPUT ${BUILD_DIR}/pbuild.completed )
	add_custom_command(
        OUTPUT ${PBUILD_OUTPUT}
		COMMAND ${PBUILDER_SCRIPT} ${DEBUILD_PLATFORM} ${ARG_COMPONENT} ${MAKEFLAGS}
			1> ${BUILD_DIR}/pbuilder.log
		COMMAND echo > ${PBUILD_OUTPUT}
		DEPENDS ${PBUILDER_DEPS}
		WORKING_DIRECTORY ${SRC_DIR}
		COMMENT "Building debian package ${ARG_TARGET_NAME} with pbuilder-dist."
		)
	add_custom_target(
		${ARG_TARGET_NAME}-pbuilder
		DEPENDS ${PBUILD_OUTPUT}
		WORKING_DIRECTORY ${SRC_DIR}
		)

	add_custom_target(
		${ARG_TARGET_NAME}-clean
		COMMAND rm -f ${BUILD_DIR}/*.completed
		)

	add_custom_target(
		${ARG_TARGET_NAME}
		DEPENDS ${ARG_TARGET_NAME}-make
		)
endfunction()


#function( GetComponent PROJECT_NAME )
#	get_filename_component( FULL_PATH     ${PROJECT_NAME}	ABSOLUTE  )
#	get_filename_component( COMPONENT_DIR ${FULL_PATH}      DIRECTORY )
#	if( ${COMPONENT_DIR} STREQUAL ${CMAKE_SOURCE_DIR} )
#		message( FATAL_ERROR "You need to specify the component as I cannot deduce it via subdirs." )
#	endif()
#	get_filename_component( COMPONENT     ${COMPONENT_DIR}    NAME      )
#	set( COMPONENT ${COMPONENT} PARENT_SCOPE )
#endfunction()

function( ConfigureMakeProject )
	set( options
		USE_CONFIGURE_SCRIPT
	)
	set( oneValueArgs
		PROJECT_NAME
		DISTFILE_PATH
        COMPONENT
	)
	set( multiValueArgs
		CONFIG_ARGS
		DEPENDS
	)
	cmake_parse_arguments( ARG
		"${options}"
		"${oneValueArgs}"
		"${multiValueArgs}"
		${ARGN}
	)

	if( ARG_USE_CONFIGURE_SCRIPT )
		set( CONF_SCRIPT_OPTION "USE_CONFIGURE_SCRIPT" )
	endif()
    if( NOT ARG_COMPONENT )
        message( FATAL_ERROR "You must specify COMPONENT (main, config, non-free, etc)!" )
    endif()

	message( STATUS "Searching dependencies for project '${ARG_PROJECT_NAME}'")
	execute_process( 
		COMMAND ${FIND_DEPS_SCRIPT} ${DEP_CACHE_FILE} ${ARG_PROJECT_NAME}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		OUTPUT_VARIABLE DEPENDS_LIST
		)
	separate_arguments( DEPENDS_LIST )
	set_property(
		GLOBAL PROPERTY ${ARG_PROJECT_NAME}_DEPENDS_LIST
		${DEPENDS_LIST}
		)

	ConfigureMakeProjectInternal(
		${CONF_SCRIPT_OPTION}
		PROJECT_NAME  ${ARG_PROJECT_NAME}
		TARGET_NAME   ${ARG_PROJECT_NAME}
		DISTFILE_PATH ${ARG_DISTFILE_PATH}
		CONFIG_ARGS   ${ARG_CONFIG_ARGS}
        COMPONENT     ${ARG_COMPONENT}
	)

	ConfigureMakeProjectInternal(
		${CONF_SCRIPT_OPTION}
		PROJECT_NAME  ${ARG_PROJECT_NAME}
		TARGET_NAME   ${ARG_PROJECT_NAME}-nodeps
		DISTFILE_PATH ${ARG_DISTFILE_PATH}
		CONFIG_ARGS   ${ARG_CONFIG_ARGS}
        COMPONENT     ${ARG_COMPONENT}
		IGNORE_DEPS
	)

	set_property( GLOBAL APPEND PROPERTY BUILD_TARGETS    ${ARG_PROJECT_NAME}          )
	set_property( GLOBAL APPEND PROPERTY CLEAN_TARGETS    ${ARG_PROJECT_NAME}-clean    )
	set_property( GLOBAL APPEND PROPERTY PACKAGE_TARGETS  ${ARG_PROJECT_NAME}-debuild  )
	set_property( GLOBAL APPEND PROPERTY DPUT_TARGETS     ${ARG_PROJECT_NAME}-dput     )
	set_property( GLOBAL APPEND PROPERTY PBUILDER_TARGETS ${ARG_PROJECT_NAME}-pbuilder )
endfunction()

function( BuildRepro COMPONENT )
	get_property( PBUILDER_TARGETS GLOBAL PROPERTY PBUILDER_TARGETS )

	set( SRC_DIR   ${CMAKE_CURRENT_SOURCE_DIR} )
	set( BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR} )

	add_custom_target(
		${COMPONENT}-reprepro
		COMMAND ${REPREPRO_SCRIPT}
			${DEBUILD_PLATFORM} ${COMPONENT} ${REPO_ROOT_DIR}
			1> ${BUILD_DIR}/${COMPONENT}-reprepro.log
		DEPENDS ${PBUILDER_TARGETS}
		WORKING_DIRECTORY ${SRC_DIR}
		COMMENT "Building debian source repro for component ${COMPONENT}."
		)
	set_property( GLOBAL APPEND PROPERTY REPREPRO_TARGETS ${COMPONENT}-reprepro )
endfunction()


function( CreateTargets COMPONENT )
	get_property( BUILD_TARGETS    GLOBAL PROPERTY BUILD_TARGETS    )
	get_property( CLEAN_TARGETS    GLOBAL PROPERTY CLEAN_TARGETS    )
	get_property( PACKAGE_TARGETS  GLOBAL PROPERTY PACKAGE_TARGETS  )
	get_property( DPUT_TARGETS     GLOBAL PROPERTY DPUT_TARGETS     )
	get_property( PBUILDER_TARGETS GLOBAL PROPERTY PBUILDER_TARGETS )
	get_property( REPREPRO_TARGETS GLOBAL PROPERTY REPREPRO_TARGETS )

	if( ${COMPONENT} STREQUAL "top" )
		unset( COMP_SUFFIX )
	else()
		set( COMP_SUFFIX "-${COMPONENT}" )
	endif()

	add_custom_target( build${COMP_SUFFIX} ALL
		DEPENDS ${BUILD_TARGETS}
	)
	add_custom_target( clean-all${COMP_SUFFIX}
		COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
		DEPENDS ${CLEAN_TARGETS}
	)
	add_custom_target( debuild${COMP_SUFFIX}
		DEPENDS ${PACKAGE_TARGETS}
	)
	add_custom_target( dput${COMP_SUFFIX}
		DEPENDS ${DPUT_TARGETS}
	)
	add_custom_target( pbuilder${COMP_SUFFIX}
		DEPENDS ${PBUILDER_TARGETS}
	)
	add_custom_target( reprepro${COMP_SUFFIX}
		DEPENDS ${REPREPRO_TARGETS}
	)
endfunction()

# vim: ts=4 sw=4 noexpandtab
