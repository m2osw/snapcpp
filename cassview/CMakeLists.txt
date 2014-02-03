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
set(CASSVIEW_VERSION_PATCH 3)

cmake_minimum_required(VERSION 2.8)
project(cassview-project)

set( CMAKE_AUTOMOC TRUE )

add_definitions( -std=c++0x )

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

# Create a configure script
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/QtCassandra/QCassandra.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/QtCassandra/QCassandra.h)

add_subdirectory(src)
#add_subdirectory(tests)
#add_subdirectory(doc)

#set(CPACK_PACKAGE_NAME "cassview")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "cassview -- a Qt based-application for browsing a Cassandra database.")
#set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
#set(CPACK_PACKAGE_CONTACT "doug@m2osw.com")
#set(CPACK_RESOURCE_FILE_LICENSE "${${PROJECT_NAME}_SOURCE_DIR}/LICENSE.txt")
#set(CPACK_GENERATOR "TGZ" "DEB")
#set(CPACK_SOURCE_GENERATOR "TGZ")
#set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
#set(CPACK_PACKAGE_VERSION "${CASSVIEW_VERSION_MAJOR}.${CASSVIEW_VERSION_MINOR}.${CASSVIEW_VERSION_PATCH}")
#set(CPACK_PACKAGE_VERSION_MAJOR "${CASSVIEW_VERSION_MAJOR}")
#set(CPACK_PACKAGE_VERSION_MINOR "${CASSVIEW_VERSION_MINOR}")
#set(CPACK_PACKAGE_VERSION_PATCH "${CASSVIEW_VERSION_PATCH}")
#set(CPACK_SOURCE_PACKAGE_FILE_NAME "cassview-${CASSVIEW_VERSION_MAJOR}.${CASSVIEW_VERSION_MINOR}.${CASSVIEW_VERSION_PATCH}")
#set(CPACK_PACKAGE_EXECUTABLES "cassview")
#set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/cassview")
#set(CPACK_DEBIAN_PACKAGE_ENHANCES "libqtcore4") # need to add "cassandra/thrift" once available
## I'm not too sure how we can add this one... components?
## See http://www.vtk.org/Wiki/CMake:Component_Install_With_CPack
##set(CPACK_DEBIAN_PACKAGE_DEV_DEPENDENCIES "cmake libqtcore4-dev thrift-dev")
#include(CPack)

# vim: ts=4 sw=4 et
