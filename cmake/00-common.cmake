# File:         00-common.cmake
# Object:       Common definitions for all M2OSW Snap! C++ projects
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
if( NOT COMMON_INCLUDED )
set( COMMON_INCLUDED TRUE )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization   -Winit-self -Wlogical-op -Wmissing-include-dirs    -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel    -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra" )

set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -g -O0" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3" )

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( SNAP_LINUX TRUE )
else()
    message( WARNING "You may have problems trying to compile this code on non-*nix platforms." )
endif()

set( ${PROJECT_NAME}_COVERAGE FALSE CACHE STRING "Turn on coverage" )

if( ${${PROJECT_NAME}_COVERAGE} )
	message("*** COVERAGE TURNED ON ***")
	find_program( COV gcov )
	if( ${COV} STREQUAL "COV-NOTFOUND" )
		message( FATAL_ERROR "Coverage requested, but gcov not installed!" )
	endif()
	#
	set( COV_C_FLAGS             "-fprofile-arcs -ftest-coverage" )
	set( COV_CXX_FLAGS           "-fprofile-arcs -ftest-coverage" )
	set( COV_SHARED_LINKER_FLAGS "-fprofile-arcs -ftest-coverage" )
	set( COV_EXE_LINKER_FLAGS    "-fprofile-arcs -ftest-coverage" )
	#
	set( CMAKE_C_FLAGS             "${CMAKE_C_FLAGS} ${COV_C_FLAGS}"                         )
	set( CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} ${COV_CXX_FLAGS}"                     )
	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${COV_SHARED_LINKER_FLAGS}" )
	set( CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} ${COV_EXE_LINKER_FLAGS}"       )

	set( COVERAGE_LIBS gcov )
endif()

endif( NOT COMMON_INCLUDED )

# vim: ts=4 sw=4 expandtab

