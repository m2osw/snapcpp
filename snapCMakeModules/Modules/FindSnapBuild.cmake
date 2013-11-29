# File:         FindSnapBuild.cmake
# Object:       Provide functions to build projects.
#
# Copyright:    Copyright (c) 2011-2013 Made to Order Software Corp.
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

function( ConfigureMakeProject )
	set( options        USE_CONFIGURE_SCRIPT )
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
	set( DIST_DIR ${CMAKE_BINARY_DIR}/dist )
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

	add_custom_target( ${ARG_PROJECT_NAME}-depends DEPENDS ${ARG_DEPENDS} )

	if( ARG_USE_CONFIGURE_SCRIPT )
		set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${BUILD_DIR}/configure --prefix=${DIST_DIR} ${ARG_CONFIG_ARGS}
				1> ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.log
				2> ${BUILD_DIR}/${ARG_PROJECT_NAME}_configure.err
			DEPENDS ${ARG_PROJECT_NAME}-depends
			WORKING_DIRECTORY ${BUILD_DIR}
			COMMENT "Running ${ARG_PROJECT_NAME} configure script..."
			)
	else()
		set( CONFIGURE_TARGETS ${BUILD_DIR}/CMakeCache.txt  )
		add_custom_command(
			OUTPUT ${CONFIGURE_TARGETS}
			COMMAND ${CMAKE_COMMAND}
				-DCMAKE_INSTALL_PREFIX:PATH=${DIST_DIR}
				-DCMAKE_MODULE_PATH:PATH=${CMAKE_SOURCE_DIR}/snapCMakeModules/Modules
				${ARG_CONFIG_ARGS}
				${SRC_DIR}
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

	add_custom_target(
		${ARG_PROJECT_NAME}-clean
		COMMAND rm -rf ${RM_DIR}
		)

	add_custom_target(
		${ARG_PROJECT_NAME}
		DEPENDS ${ARG_PROJECT_NAME}-install
		)

	set_property( GLOBAL APPEND PROPERTY BUILD_TARGETS ${ARG_PROJECT_NAME}       )
	set_property( GLOBAL APPEND PROPERTY CLEAN_TARGETS ${ARG_PROJECT_NAME}-clean )
endfunction()

# vim: ts=4 sw=4 noexpandtab
