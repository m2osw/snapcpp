#
# File:         CMakeLists.txt
# Object:       Query Google for a page rank.
#
# Copyright:    Copyright (c) 2012 Made to Order Software Corp.
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

cmake_minimum_required(VERSION 2.8)

project( googlerank )

##
## Include support modules
##
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH cmake
        PATHS
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/..
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/cmake )
    message( "Found cmake in ${CMAKE_MODULE_PATH}" )
endif()
##
include( 00-common )

##
## Find Snap! prerequisites
##
if( NOT controlled_vars_SOURCE_DIR )
    find_package( ControlledVars REQUIRED )
    include_directories(
        "${CONTROLLEDVARS_INCLUDE_DIRS}"
    )
else()
    include_directories(
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}"
    )
endif()

##
## Find Qt4
##
include( qt )
SnapIncludeQt4( QtCore QtNetwork )
include_directories( ${QT_INCLUDES} )

##
## Google Rank Version
##
set(GOOGLERANK_VERSION_MAJOR 1)
set(GOOGLERANK_VERSION_MINOR 0)
set(GOOGLERANK_VERSION_PATCH 0)

##
## Google Rank library
##
project(libgooglerank)

QT4_WRAP_CPP(
    GOOGLEPAGERANK_MOC_OUTFILES
    googlepagerank.h
    googlepagerank_http.h
)

add_library(googlepagerank SHARED
    googlepagerank.cpp
    googlepagerank_http.cpp
    ${GOOGLEPAGERANK_MOC_OUTFILES}
)

target_link_libraries(googlepagerank
    ${QT_LIBRARIES}
)

if( controlled_vars_SOURCE_DIR )
    add_dependencies( googlepagerank controlled_vars_headers )
endif()

install(
    FILES googlepagerank.h
    DESTINATION include
)

##
## Google Rank Tool
##
project(pagerank)

add_executable(${PROJECT_NAME}
    pagerank.cpp
)

include_directories(
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME}
    ${QT_LIBRARIES}
    googlepagerank
)

if( controlled_vars_SOURCE_DIR )
    add_dependencies( ${PROJECT_NAME} controlled_vars_headers )
endif()

##
## Include sub-folders
##
add_subdirectory(doc)       # API Documentation

##
## To pack the source
##
set(CPACK_PACKAGE_NAME "googlerank")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "googlerank -- Tool to check the rank of your pages with the famous Google search engine.")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "alexis@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${libgooglerank_SOURCE_DIR}/LICENSE.txt")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
set(CPACK_PACKAGE_VERSION "${GOOGLERANK_VERSION_MAJOR}.${GOOGLERANK_VERSION_MINOR}.${GOOGLERANK_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${GOOGLERANK_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${GOOGLERANK_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${GOOGLERANK_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "googlerank-${GOOGLERANK_VERSION_MAJOR}.${GOOGLERANK_VERSION_MINOR}.${GOOGLERANK_VERSION_PATCH}")
#set(CPACK_PACKAGE_EXECUTABLES "")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/googlerank")
include(CPack)

# vim: ts=4 sw=4 et
