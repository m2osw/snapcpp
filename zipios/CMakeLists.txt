#
# File:
#      CMakeLists.txt
#
# Description:
#      Build zipios++ library.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2013 Made to Order Software Corp.
#
#      http://snapwebsites.org/
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

cmake_minimum_required(VERSION 2.8)

project( zipios_project )

set( VERSION 0.1.5.10 )
set( SOVERSION 0 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=shadow -Wno-error=unused-parameter -Wno-error=unused-result")

find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen REQUIRED )

configure_file(
    ${CMAKE_SOURCE_DIR}/zipios++/zipios-config.h.in
    ${CMAKE_BINARY_DIR}/zipios++/zipios-config.h
    )

AddDoxygenTarget( zipios 0 1 5 )

include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} )

add_subdirectory( src )

install( DIRECTORY zipios++
    DESTINATION include
    PATTERN "*.in" EXCLUDE
    PATTERN "*.w32" EXCLUDE
    PATTERN "*.am" EXCLUDE
)

install( FILES ${CMAKE_BINARY_DIR}/zipios++/zipios-config.h
    DESTINATION include/zipios++
)

install( DIRECTORY cmake/ DESTINATION share/cmake-2.8/Modules )

# vim: ts=4 sw=4 et nocindent
