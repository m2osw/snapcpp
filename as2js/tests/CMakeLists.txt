#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2005-2014 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/project/as2js
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

##
## as2js tests
##

find_package( CppUnit )
find_package( Qt4 4.8.0 COMPONENTS QtCore QtGui ) # TODO: we really want to find qxcppunitd and qxrunnerd, should probably be part of CppUnit
if(QT_FOUND)
    include( ${QT_USE_FILE} )
endif(QT_FOUND)

# All the tests require cppunit
if(CPPUNIT_FOUND)

# Test library objects
# Using cppunit does not require a function
#function( as2jsLibraryTest )
#    set( options        NO_OPTIONS )
#    set( oneValueArgs   PROJECT_NAME )
#    set( multiValueArgs SOURCES )
#    cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
#
#    if( NOT ARG_PROJECT_NAME )
#        message( FATAL_ERROR "You must specify PROJECT_NAME to as2jsLibraryTest!" )
#    endif()
#    if( NOT ARG_SOURCES )
#        message( FATAL_ERROR "You must specify SOURCES to as2jsLibraryTest!" )
#    endif()
#
#    message( "Setup test \"" ${ARG_PROJECT_NAME} "\"" )
#    project( ${ARG_PROJECT_NAME} )
#    include_directories(
#        ${as2js_compiler_BINARY_DIR}/include
#        ${as2js_compiler_BINARY_DIR}/src
#        ${as2js_compiler_SOURCE_DIR}/include
#        ${as2js_compiler_SOURCE_DIR}/src
#    )
#    add_executable( ${ARG_PROJECT_NAME}
#        ${ARG_SOURCES}
#    )
#    target_link_libraries( ${ARG_PROJECT_NAME}
#        as2js
#    )
#endfunction()


# Library: class String
#as2jsLibraryTest(PROJECT_NAME test_as2js_string SOURCES test_as2js_string)

project(test_as2js)

# TODO: add proper Qt stuff
if(QT_FOUND)
    set(HAVE_QT4 1)
    add_definitions(-DHAVE_QT4)
endif(QT_FOUND)

include_directories(
    ${CONTROLLEDVARS_INCLUDE_DIR}
    ${ADVGETOPT_INCLUDE_DIR}
    ${as2js_compiler_BINARY_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/src
    ${CPPUNIT_INCLUDE_DIR}
    ${QT_INCLUDES}
)
add_executable(${PROJECT_NAME}
    test_as2js_main.cpp

    test_as2js_float64.cpp
    test_as2js_int64.cpp
    test_as2js_options.cpp
    test_as2js_position.cpp
    test_as2js_string.cpp
    test_as2js_version.cpp

    # Stick a copy of the tools license in the tests
    ../src/license.cpp
)
if(HAVE_QT4)
    set(CPPUNIT_QT4 qxcppunitd qxrunnerd)
endif(HAVE_QT4)
target_link_libraries( ${PROJECT_NAME}
    as2js
    ${ADVGETOPT_LIBRARIES}
    ${QT_LIBRARIES}
    ${CPPUNIT_LIBRARIES}
    ${CPPUNIT_QT4}
)



##
## Run all the tests
##
project(run_as2js_tests)
add_custom_target(run_as2js_tests
	COMMAND echo "*** test_as2js"
	COMMAND test_as2js
	DEPENDS
		test_as2js
)

else(CPPUNIT_FOUND)
message("No test will be created because you do not seem to have cppunit available")
endif(CPPUNIT_FOUND)

# vim: ts=4 sw=4 et
