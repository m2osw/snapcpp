# File:         FindSnapBuild.cmake
# Object:       Provide functions to build projects.
#
# Copyright:    Copyright (c) 2011-2014 Made to Order Software Corp.
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

find_program( MAKE_SOURCE_SCRIPT SnapBuildMakeSourcePackage.sh PATHS ${CMAKE_MODULE_PATH} )
find_program( MAKE_DPUT_SCRIPT   SnapBuildDputPackage.sh       PATHS ${CMAKE_MODULE_PATH} )
find_program( INC_DEPS_SCRIPT    SnapBuildIncDeps.pl           PATHS ${CMAKE_MODULE_PATH} )
find_program( PBUILDER_SCRIPT    SnapPBuilder.sh			   PATHS ${CMAKE_MODULE_PATH} )

function( ConfigureMakeProjectInternal )
	set( options        USE_CONFIGURE_SCRIPT NOINC_DEBVERS )
	set( oneValueArgs   PROJECT_NAME TARGET_NAME DISTFILE_PATH )
	set( multiValueArgs CONFIG_ARGS DEPENDS )
	cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
	#
	if( NOT ARG_PROJECT_NAME )
		message( FATAL_ERROR "You must specify PROJECT_NAME!" )
	endif()
	if( NOT ARG_TARGET_NAME )
		message( FATAL_ERROR "You must specify TARGET_NAME!" )
	endif()

	set( SRC_DIR        ${CMAKE_SOURCE_DIR}/${ARG_PROJECT_NAME} )
	set( BUILD_DIR      ${CMAKE_BINARY_DIR}/${ARG_TARGET_NAME}  )
	set( SNAP_DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Destination installation folder." )
	if( ARG_DISTFILE_PATH )
		set( RM_DIR ${SRC_DIR}   )
	else()
		set( RM_DIR ${BUILD_DIR} )
	endif()

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

	#set_property( GLOBAL PROPERTY ${ARG_PROJECT_NAME}_DEPENDS_LIST ${ARG_DEPENDS} )
	add_custom_target( ${ARG_TARGET_NAME}-depends DEPENDS ${ARG_DEPENDS} )

	if( ARG_USE_CONFIGURE_SCRIPT )
		set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${BUILD_DIR}/configure --prefix=${SNAP_DIST_DIR} ${ARG_CONFIG_ARGS}
				1> ${BUILD_DIR}/configure.log
				2> ${BUILD_DIR}/configure.err
			DEPENDS ${ARG_TARGET_NAME}-depends
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_TARGET_NAME} configure script..."
			)
	else()
		set( BUILD_TYPE ${CMAKE_BUILD_TYPE} )
		if( NOT BUILD_TYPE )
			set( BUILD_TYPE Release )
		endif()
		set( COMMAND_LIST
			${THE_CMAKE_COMMAND}
				-DCMAKE_BUILD_TYPE=${BUILD_TYPE}
				-DCMAKE_INSTALL_PREFIX:PATH="${SNAP_DIST_DIR}"
				-DCMAKE_PREFIX_PATH:PATH=${SNAP_DIST_DIR}
				${ARG_CONFIG_ARGS}
				${SRC_DIR}
	    )
		set( CMD_FILE ${BUILD_DIR}/configure.cmd )
		file( REMOVE ${CMD_FILE} )
		file( APPEND ${CMD_FILE} "cd " ${BUILD_DIR} "\n" )
		file( APPEND ${CMD_FILE} "rm -f CMakeCache.txt\n" )
		foreach( line ${COMMAND_LIST} )
			file( APPEND ${CMD_FILE} ${line} " \\\n" )
		endforeach()
		#
		set( CONFIGURE_TARGETS ${BUILD_DIR}/CMakeCache.txt  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${COMMAND_LIST}
				1> ${BUILD_DIR}/configure.log
				2> ${BUILD_DIR}/configure.err
			DEPENDS ${ARG_TARGET_NAME}-depends
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_TARGET_NAME} CMake configuration..."
			)
	endif()

	set( THE_CMAKE_BUILD_TOOL ${CMAKE_BUILD_TOOL} )
	if( ${CMAKE_VERSION} VERSION_GREATER 3.0.0 )
		set( MAKEFLAGS "-j1" CACHE STRING "Number of jobs make should run. CMake vers 3+ only!" )
		if( "${MAKEFLAGS}" STREQUAL "-j1" OR "${MAKEFLAGS}" STREQUAL "" )
			set( THE_CMAKE_BUILD_TOOL ${CMAKE_COMMAND} -E env MAKEFLAGS=\"${MAKEFLAGS}\" ${CMAKE_BUILD_TOOL} )
		endif()
	endif()
	add_custom_target(
		${ARG_TARGET_NAME}-make
		COMMAND ${THE_CMAKE_BUILD_TOOL}
			1> ${BUILD_DIR}/make.log
			2> ${BUILD_DIR}/make.err
		DEPENDS ${CONFIGURE_TARGETS}
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Building ${ARG_TARGET_NAME}"
		)

	add_custom_target(
		${ARG_TARGET_NAME}-install
		COMMAND ${CMAKE_BUILD_TOOL} install
			1>> ${BUILD_DIR}/make.log
			2>> ${BUILD_DIR}/make.err
		DEPENDS ${ARG_TARGET_NAME}-make
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Installing ${ARG_TARGET_NAME}"
		)

	# RDB: Tue Jan 26 10:13:24 PST 2016
	# Adding ability to take the DEBUILD_EMAIL var from the environment as a default
	#
	set( DEBUILD_EMAIL_DEFAULT "Build Server <build@m2osw.com>" )
	if( DEFINED ENV{DEBEMAIL} )
		set( DEBUILD_EMAIL_DEFAULT $ENV{DEBEMAIL} )
	endif()

	# RDB: Thu Jun 26 13:45:46 PDT 2014
	# Adding "debuild" target.
	#
	set( DEBUILD_PLATFORM "trusty"                   CACHE STRING "Name of the Debian/Ubuntu platform to build against." )
	set( DEBUILD_EMAIL    "${DEBUILD_EMAIL_DEFAULT}" CACHE STRING "Email address of the package signer."                 )
	set( EMAIL_ADDY       "${DEBUILD_EMAIL}" )
	separate_arguments( EMAIL_ADDY )
	#
	unset( PBUILDER_DEPS )
	unset( DPUT_DEPS     )
	if( ARG_DEPENDS )
		foreach( DEP ${ARG_DEPENDS} )
			list( APPEND PBUILDER_DEPS ${DEP}-pbuilder )
			list( APPEND DPUT_DEPS     ${DEP}-dput     )
		endforeach()
		add_custom_target(
			${ARG_TARGET_NAME}-incdeps
			COMMAND ${INC_DEPS_SCRIPT} ${CMAKE_SOURCE_DIR} ${ARG_TARGET_NAME} ${ARG_DEPENDS}
			WORKING_DIRECTORY ${SRC_DIR}
			COMMENT "Incrementing dependencies for debian package ${ARG_TARGET_NAME}"
			)
	else()
		add_custom_target( ${ARG_TARGET_NAME}-incdeps )
	endif()
	unset( NOINC_DEBVERS )
	if( ARG_NOINC_DEBVERS )
		set( NOINC_DEBVERS "--noinc" )
	endif()
	add_custom_target(
		${ARG_TARGET_NAME}-debuild
		COMMAND env DEBEMAIL="${EMAIL_ADDY}" ${MAKE_SOURCE_SCRIPT} ${NOINC_DEBVERS} ${DEBUILD_PLATFORM}
			1> ${BUILD_DIR}/debuild.log
		WORKING_DIRECTORY ${SRC_DIR}
		DEPENDS ${ARG_TARGET_NAME}-incdeps
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
	add_custom_target(
		${ARG_TARGET_NAME}-pbuilder
		COMMAND ${PBUILDER_SCRIPT} ${DEBUILD_PLATFORM} 
			1> ${BUILD_DIR}/pbuilder.log
		DEPENDS ${PBUILDER_DEPS}
		WORKING_DIRECTORY ${SRC_DIR}
		COMMENT "Building debian package ${ARG_TARGET_NAME} with pbuilder-dist."
		)

	add_custom_target(
		${ARG_TARGET_NAME}-clean
		COMMAND rm -rf ${RM_DIR}
		)

	add_custom_target(
		${ARG_TARGET_NAME}
		DEPENDS ${ARG_TARGET_NAME}-install
		)
endfunction()

function( ConfigureMakeProject )
	set( options
		USE_CONFIGURE_SCRIPT
		NOINC_DEBVERS
	)
	set( oneValueArgs
		PROJECT_NAME
		DISTFILE_PATH
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
	if( ARG_NOINC_DEBVERS )
		set( CONF_NOINC_OPTION "NOINC_DEBVERS" )
	endif()

	ConfigureMakeProjectInternal(
		${CONF_SCRIPT_OPTION}
		${CONF_NOINC_OPTION}
		PROJECT_NAME  ${ARG_PROJECT_NAME}
		TARGET_NAME   ${ARG_PROJECT_NAME}
		DISTFILE_PATH ${ARG_DISTFILE_PATH}
		CONFIG_ARGS   ${ARG_CONFIG_ARGS}
		DEPENDS       ${ARG_DEPENDS}
	)

	ConfigureMakeProjectInternal(
		${CONF_SCRIPT_OPTION}
		${CONF_NOINC_OPTION}
		PROJECT_NAME  ${ARG_PROJECT_NAME}
		TARGET_NAME   ${ARG_PROJECT_NAME}-nodeps
		DISTFILE_PATH ${ARG_DISTFILE_PATH}
		CONFIG_ARGS   ${ARG_CONFIG_ARGS}
	)

	set_property( GLOBAL APPEND PROPERTY BUILD_TARGETS    ${ARG_PROJECT_NAME}          )
	set_property( GLOBAL APPEND PROPERTY CLEAN_TARGETS    ${ARG_PROJECT_NAME}-clean    )
	set_property( GLOBAL APPEND PROPERTY PACKAGE_TARGETS  ${ARG_PROJECT_NAME}-debuild  )
	set_property( GLOBAL APPEND PROPERTY DPUT_TARGETS     ${ARG_PROJECT_NAME}-dput     )
	set_property( GLOBAL APPEND PROPERTY PBUILDER_TARGETS ${ARG_PROJECT_NAME}-pbuilder )
endfunction()

# vim: ts=4 sw=4 noexpandtab
