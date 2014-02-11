#
# Text:
#      CMakeLists.txt
#
# Description:
#      Top level project definitions which includes the C++ files and the
#      header files as required to compile the CassandraBrowse project
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

set(CASSVIEW_VERSION_MAJOR 0)
set(CASSVIEW_VERSION_MINOR 0)
set(CASSVIEW_VERSION_PATCH 7)

cmake_minimum_required(VERSION 2.8)

project(cassview-project)

set( CMAKE_AUTOMOC TRUE )

if( NOT MSVC )
    add_definitions( -std=c++0x )
endif()

add_definitions( -DCASSVIEW_VERSION=\"${CASSVIEW_VERSION_MAJOR}.${CASSVIEW_VERSION_MINOR}.${CASSVIEW_VERSION_PATCH}\" )

## Include support modules
##
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules/Modules )
    message( "Found cmake in ${CMAKE_MODULE_PATH}" )
endif()
find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen      REQUIRED )
find_package( QtCassandra      REQUIRED )
find_package( SnapWebsites     REQUIRED )

find_package( Qt4 4.8.1 REQUIRED QtCore QtNetwork QtGui )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories( 
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}"
    "${QTCASSANDRA_INCLUDE_DIRS}"
    "${CMAKE_BINARY_DIR}"
    "${QT_INCLUDES}"
    "${SNAPWEBSITES_INCLUDE_DIRS}"
)

add_subdirectory(src)
#add_subdirectory(tests)
#add_subdirectory(doc)

install( FILES cassview.desktop DESTINATION share/applications )

# vim: ts=4 sw=4 et
