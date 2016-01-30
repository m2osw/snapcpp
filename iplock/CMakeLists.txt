#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment
#
# Copyright:    Copyright (c) 2012-2015 Made to Order Software Corp.
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

enable_language( CXX )

##
## IP Lock Version
##
set(IPLOCK_MAJOR 2)
set(IPLOCK_MINOR 1)
set(IPLOCK_PATCH 2)

project(iplock)

include(GNUInstallDirs)

# Put the version in the cpp file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/iplock.h.in ${CMAKE_CURRENT_BINARY_DIR}/iplock.h )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

##
## Include common Snap! cmake modules
##
#if( NOT CMAKE_MODULE_PATH )
#    set( CMAKE_MODULE_PATH ${CMAKE_PREFIX_PATH}/share/cmake )
#endif()

find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen      REQUIRED )


# The documentation
add_subdirectory(doc)


##
## IP Lock Tool
##
add_executable(${PROJECT_NAME}
    iplock.cpp
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION sbin
    #${CMAKE_INSTALL_SBINDIR}
)

install(
    FILES
        iplock.conf
    DESTINATION
        share/iplock/conf
)

# vim: ts=4 sw=4 et
