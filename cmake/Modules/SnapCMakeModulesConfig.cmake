# File:         FindSnapCMakeModules.cmake
# Object:       Common definitions for all M2OSW Snap! C++ projects
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

# TODO: Work on adding the following two additional warning captures
#    -Wold-style-cast -Wnoexcept
#
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option -Wunused-parameter -Wfloat-equal -DQT_DISABLE_DEPRECATED_BEFORE=0x050501 -DQT_DEPRECATED_WARNINGS" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra -Wunused-parameter -fwrapv" )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    option( SANITIZE_ADDRESS "Sanitize addresses for ${PROJECT_NAME}. Debug mode only." OFF )
    if( ${SANITIZE_ADDRESS} )
        set( SANITIZE_SWITCH "-fsanitize=address" )
    endif()
endif()

set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG -g -O0 ${SANITIZE_SWITCH}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3" )

set( CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -D_DEBUG -g -O0 ${SANITIZE_SWITCH}" )
set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O3" )

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( SNAP_LINUX TRUE )

    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
else()
    message( WARNING "You may have problems trying to compile this code on non-*nix platforms." )
endif()

# To generate coverage, use -D<project name>_COVERAGE=ON
#                       and -DCMAKE_BUILD_TYPE=Debug
option( ${PROJECT_NAME}_COVERAGE "Turn on coverage for ${PROJECT_NAME}." OFF )

if( ${${PROJECT_NAME}_COVERAGE} )
	message("*** COVERAGE TURNED ON ***")
	find_program( COV gcov )
	if( ${COV} STREQUAL "COV-NOTFOUND" )
		message( FATAL_ERROR "Coverage requested, but gcov not installed!" )
	endif()
	if( NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
		message( FATAL_ERROR "Coverage requested, but Debug is not turned on! (i.e. -DCMAKE_BUILD_TYPE=Debug)" )
	endif()
	#
	set( COV_C_FLAGS             "-fprofile-arcs -ftest-coverage -pg" )
	set( COV_CXX_FLAGS           "-fprofile-arcs -ftest-coverage -pg" )
	set( COV_SHARED_LINKER_FLAGS "-fprofile-arcs -ftest-coverage -pg" )
	set( COV_EXE_LINKER_FLAGS    "-fprofile-arcs -ftest-coverage -pg" )
	#
	set( CMAKE_C_FLAGS             "${CMAKE_C_FLAGS} ${COV_C_FLAGS}"                         )
	set( CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} ${COV_CXX_FLAGS}"                     )
	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${COV_SHARED_LINKER_FLAGS}" )
	set( CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} ${COV_EXE_LINKER_FLAGS}"       )
endif()

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args(
    SnapCMakeModules
    DEFAULT_MSG
    CMAKE_C_FLAGS
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
)


find_program( DPKG_PARSECHANGELOG dpkg-parsechangelog )

# Use this function to parse the version from the debian/changelog.
# It will create four variables in the parent scope, each with ${PACKAGE_NAME}_
# prepended.
#
# You must have dpkg-dev installed first!
#
function( SnapGetVersion PACKAGE_NAME WORKING_DIRECTORY )
    if( ${DPKG_PARSECHANGELOG} STREQUAL ${DPKG_PARSECHANGELOG}-NOTFOUND )
        message( FATAL_ERROR "dpkg-parsechangelog not found! Is dpkg-dev installed?" )
    endif()

    execute_process(
        COMMAND ${DPKG_PARSECHANGELOG} -S Version
        WORKING_DIRECTORY ${WORKING_DIRECTORY}
        OUTPUT_VARIABLE VERSION
        )

    string( REPLACE "\n" ""  ORIG_VERSION ${VERSION}      )
    string( REPLACE "."  " " VERSION      ${ORIG_VERSION} )

    separate_arguments( VERSION )

    list( LENGTH VERSION LEN )
    if( NOT ${LEN} EQUAL 4 )
        message( FATAL_ERROR "Package name '${PACKAGE_NAME}' does not have a valid version number: '${ORIG_VERSION}'. It must have 4 parts: major.minor.patch.build!" )
    endif()

    list( GET VERSION 0 major_version  )
    list( GET VERSION 1 minor_version  )
    list( GET VERSION 2 patch_version  )
    list( GET VERSION 3 _build_version )

    string( REPLACE "-"  " " temp_ver ${_build_version} )
    list( LENGTH temp_ver temp_ver_len )
    if( ${temp_ver_len} EQUAL 2 )
        #
        # Throw away the debian build number if it is present (e.g. 1.2.3.4-5, where we throw out the '-5').
        #
        list( GET temp_ver 0 build_version )
    else()
        set( build_version ${_build_version} )
    endif()

    set( ${PACKAGE_NAME}_VERSION_MAJOR ${major_version} PARENT_SCOPE )
    set( ${PACKAGE_NAME}_VERSION_MINOR ${minor_version} PARENT_SCOPE )
    set( ${PACKAGE_NAME}_VERSION_PATCH ${patch_version} PARENT_SCOPE )
    set( ${PACKAGE_NAME}_VERSION_BUILD ${build_version} PARENT_SCOPE )

endfunction()

# vim: ts=4 sw=4 expandtab wrap
