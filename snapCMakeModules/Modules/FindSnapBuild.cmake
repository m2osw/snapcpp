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

function( ConfigureMakeProject )
	set( options        USE_CONFIGURE_SCRIPT NOINC_DEBVERS )
	set( oneValueArgs   PROJECT_NAME VERSION DISTFILE_PATH )
	set( multiValueArgs CONFIG_ARGS DEPENDS )
	cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
	#
	if( NOT ARG_PROJECT_NAME )
		message( FATAL_ERROR "You must specify PROJECT_NAME!" )
	endif()

	if( ARG_VERSION )
		set( SRC_DIR   ${CMAKE_BINARY_DIR}/${ARG_PROJECT_NAME}-${ARG_VERSION}                                    )
		set( BUILD_DIR ${CMAKE_BINARY_DIR}/${ARG_PROJECT_NAME}-${ARG_VERSION}/${ARG_PROJECT_NAME}-${ARG_VERSION} )
	else()
		set( SRC_DIR   ${CMAKE_SOURCE_DIR}/${ARG_PROJECT_NAME} )
		set( BUILD_DIR ${CMAKE_BINARY_DIR}/${ARG_PROJECT_NAME} )
	endif()
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

	#set_property( GLOBAL PROPERTY ${ARG_PROJECT_NAME}_DEPENDS_LIST ${ARG_DEPENDS} )
	add_custom_target( ${ARG_PROJECT_NAME}-depends DEPENDS ${ARG_DEPENDS} )

	if( ARG_USE_CONFIGURE_SCRIPT )
		set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${BUILD_DIR}/configure --prefix=${SNAP_DIST_DIR} ${ARG_CONFIG_ARGS}
				1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.log
				2> ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.err
			DEPENDS ${ARG_PROJECT_NAME}-depends
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_PROJECT_NAME} configure script..."
			)
	else()
		set( COMMAND_LIST
			${CMAKE_COMMAND}
				-DCMAKE_INSTALL_PREFIX:PATH=${SNAP_DIST_DIR}
				-DCMAKE_MODULE_PATH:PATH=${SNAP_DIST_DIR}/share/cmake-2.8/Modules
				${ARG_CONFIG_ARGS}
				${SRC_DIR}
	    )
		set( CMD_FILE ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.cmd )
		file( REMOVE ${CMD_FILE} )
		foreach( line ${COMMAND_LIST} )
			file( APPEND ${CMD_FILE} ${line} "\n" )
		endforeach()
		#
		set( CONFIGURE_TARGETS ${BUILD_DIR}/CMakeCache.txt  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${COMMAND_LIST}
				1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.log
				2> ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.err
			DEPENDS ${ARG_PROJECT_NAME}-depends
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_PROJECT_NAME} CMake configuration..."
			)
	endif()

	add_custom_target(
		${ARG_PROJECT_NAME}-make
		COMMAND ${CMAKE_BUILD_TOOL}
			1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_make.log
			2> ${BUILD_DIR}/${ARG_PROJECT_NAME}_make.err
		DEPENDS ${CONFIGURE_TARGETS}
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Building ${ARG_PROJECT_NAME}"
		)

	add_custom_target(
		${ARG_PROJECT_NAME}-install
		COMMAND ${CMAKE_BUILD_TOOL} install
			1>> ${BUILD_DIR}/${ARG_PROJECT_NAME}_make.log
			2>> ${BUILD_DIR}/${ARG_PROJECT_NAME}_make.err
		DEPENDS ${ARG_PROJECT_NAME}-make
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Installing ${ARG_PROJECT_NAME}"
		)
	
	# RDB: Thu Jun 26 13:45:46 PDT 2014
	# Adding "debuild" target.
	#
	set( DEBUILD_PLATFORM "trusty"                         CACHE STRING "Name of the Debian/Ubuntu platform to build against." )
	set( DEBUILD_EMAIL    "Build Server <build@m2osw.com>" CACHE STRING "Email address of the package signer."                 )
	set( EMAIL_ADDY ${DEBUILD_EMAIL} )
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
			${ARG_PROJECT_NAME}-incdeps
			COMMAND ${INC_DEPS_SCRIPT} ${CMAKE_SOURCE_DIR} ${ARG_PROJECT_NAME} ${ARG_DEPENDS}
			WORKING_DIRECTORY ${SRC_DIR}
			COMMENT "Incrementing dependencies for debian package ${ARG_PROJECT_NAME}"
			)
	else()
		add_custom_target( ${ARG_PROJECT_NAME}-incdeps )
	endif()
	unset( NOINC_DEBVERS )
	if( ARG_NOINC_DEBVERS )
		set( NOINC_DEBVERS "--noinc" )
	endif()
	add_custom_target(
		${ARG_PROJECT_NAME}-debuild
		COMMAND env DEBEMAIL="${EMAIL_ADDY}" ${MAKE_SOURCE_SCRIPT} ${NOINC_DEBVERS} ${DEBUILD_PLATFORM}
			1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_debuild.log
		WORKING_DIRECTORY ${SRC_DIR}
		DEPENDS ${ARG_PROJECT_NAME}-incdeps
		COMMENT "Building debian source package for ${ARG_PROJECT_NAME}"
		)
	add_custom_target(
		${ARG_PROJECT_NAME}-dput
		COMMAND ${MAKE_DPUT_SCRIPT}
			1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_dput.log
		DEPENDS ${DPUT_DEPS}
		WORKING_DIRECTORY ${SRC_DIR}
		COMMENT "Dputting debian package ${ARG_PROJECT_NAME} to launchpad."
		)
	add_custom_target(
		${ARG_PROJECT_NAME}-pbuilder
		COMMAND ${PBUILDER_SCRIPT} ${DEBUILD_PLATFORM} 
			1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_pbuilder.log
		DEPENDS ${PBUILDER_DEPS}
		WORKING_DIRECTORY ${SRC_DIR}
		COMMENT "Building debian package ${ARG_PROJECT_NAME} with pbuilder-dist."
		)

	add_custom_target(
		${ARG_PROJECT_NAME}-clean
		COMMAND rm -rf ${RM_DIR}
		)

	add_custom_target(
		${ARG_PROJECT_NAME}
		DEPENDS ${ARG_PROJECT_NAME}-install
		)

	set_property( GLOBAL APPEND PROPERTY BUILD_TARGETS    ${ARG_PROJECT_NAME}          )
	set_property( GLOBAL APPEND PROPERTY CLEAN_TARGETS    ${ARG_PROJECT_NAME}-clean    )
	set_property( GLOBAL APPEND PROPERTY PACKAGE_TARGETS  ${ARG_PROJECT_NAME}-debuild  )
	set_property( GLOBAL APPEND PROPERTY DPUT_TARGETS     ${ARG_PROJECT_NAME}-dput     )
	set_property( GLOBAL APPEND PROPERTY PBUILDER_TARGETS ${ARG_PROJECT_NAME}-pbuilder )
endfunction()

# vim: ts=4 sw=4 noexpandtab
