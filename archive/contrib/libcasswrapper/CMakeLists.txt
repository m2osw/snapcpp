#
# Text:
#      CMakeLists.txt
#
# Description:
#      Top level project definitions which includes the C++ files and the
#      header files as required to compile the libcasswrapper and tests
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
#      https://snapwebsites.org/
#      contact@m2osw.com
#
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 3.10.2)
project(libcasswrapper)

set( CMAKE_INCLUDE_CURRENT_DIR ON   )
set( CMAKE_AUTOMOC             TRUE )

## Include support modules
##
find_package( As2Js              REQUIRED )
find_package( CassandraCppDriver REQUIRED )
find_package( CassValue          REQUIRED )
find_package( LibExcept          REQUIRED )
find_package( Qt5Core            REQUIRED )
find_package( Qt5Sql             REQUIRED )
find_package( Qt5Xml             REQUIRED )
find_package( SnapCMakeModules   REQUIRED )
find_package( SnapDoxygen )

SnapGetVersion( LIBCASSWRAPPER ${CMAKE_SOURCE_DIR} )

## This will cause us to look in the 3rdParty area for thrift and other 3rdparty libs
##
link_directories( ${CMAKE_BINARY_DIR}/dist/lib/ )

include_directories(
    SYSTEM
        ${CASSWRAPPER_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Sql_INCLUDE_DIRS}
        ${Qt5Xml_INCLUDE_DIRS}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_BINARY_DIR}
)

option( RUN_TESTS "Enable CTest support and turn on the 'test' make target." OFF )
if( ${RUN_TESTS} )
    enable_testing()
endif()

add_subdirectory(cmake)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)

# vim: ts=4 sw=4 et
