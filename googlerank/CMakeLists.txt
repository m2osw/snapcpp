#
# File:         CMakeLists.txt
# Object:       Query Google for a page rank.
#
# Copyright:    Copyright (c) 2012-2013 Made to Order Software Corp.
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

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

##
## Include support modules
##
find_package( SnapCMakeModules )

##
## Find Snap! prerequisites
##
find_package( ControlledVars REQUIRED )
include_directories(
    "${CONTROLLEDVARS_INCLUDE_DIRS}"
)

##
## Find Qt5
##
find_package( Qt5Core    REQUIRED )
find_package( Qt5Network REQUIRED )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
)

##
## Google Rank Version
##
set(GOOGLERANK_VERSION_MAJOR 1)
set(GOOGLERANK_VERSION_MINOR 0)
set(GOOGLERANK_VERSION_PATCH 2)

##
## Google Rank library
##
project(libgooglerank)

add_library( googlepagerank SHARED
    googlepagerank.cpp
    googlepagerank_http.cpp
)

target_link_libraries(googlepagerank
    Qt5::Core
    Qt5::Network
)

set_target_properties(
    googlepagerank PROPERTIES 
    VERSION ${GOOGLERANK_VERSION_MAJOR}.${GOOGLERANK_VERSION_MINOR}
    SOVERSION ${GOOGLERANK_VERSION_MAJOR}
)

install(
    TARGETS googlepagerank
    DESTINATION lib
)

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
    Qt5::Core
    Qt5::Network
    googlepagerank
)

install(
    TARGETS ${PROJECT_NAME} 
    DESTINATION bin
)

##
## Include sub-folders
##
add_subdirectory(doc)       # API Documentation

##
## To pack the source
##
#set(CPACK_PACKAGE_NAME "googlerank")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "googlerank -- Tool to check the rank of your pages with the famous Google search engine.")
#set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
#set(CPACK_PACKAGE_CONTACT "alexis@m2osw.com")
#set(CPACK_RESOURCE_FILE_LICENSE "${libgooglerank_SOURCE_DIR}/LICENSE.txt")
#set(CPACK_GENERATOR "TGZ" "DEB")
#set(CPACK_SOURCE_GENERATOR "TGZ")
#set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
#set(CPACK_PACKAGE_VERSION "${GOOGLERANK_VERSION_MAJOR}.${GOOGLERANK_VERSION_MINOR}.${GOOGLERANK_VERSION_PATCH}")
#set(CPACK_PACKAGE_VERSION_MAJOR "${GOOGLERANK_VERSION_MAJOR}")
#set(CPACK_PACKAGE_VERSION_MINOR "${GOOGLERANK_VERSION_MINOR}")
#set(CPACK_PACKAGE_VERSION_PATCH "${GOOGLERANK_VERSION_PATCH}")
#set(CPACK_SOURCE_PACKAGE_FILE_NAME "googlerank-${GOOGLERANK_VERSION_MAJOR}.${GOOGLERANK_VERSION_MINOR}.${GOOGLERANK_VERSION_PATCH}")
##set(CPACK_PACKAGE_EXECUTABLES "")
#set(CPACK_DEBIAN_PACKAGE_SECTION "web")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/googlerank")
#include(CPack)

# vim: ts=4 sw=4 et
