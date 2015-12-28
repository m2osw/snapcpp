# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
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

set(ADVGETOPT_VERSION_MAJOR 1)
set(ADVGETOPT_VERSION_MINOR 0)
set(ADVGETOPT_VERSION_PATCH 16)

cmake_minimum_required(VERSION 2.8.4)

project(advgetopt_library)

#
# Include support modules
# (must be included in a project even though it is not made
# specific to that project)
#
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules/Modules )
    message( "Found snapCMakeModules in ${CMAKE_MODULE_PATH}" )
endif()
#
find_package( SnapCMakeModules REQUIRED )
find_package( ControlledVars   REQUIRED )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message("Debug is in effect for advgetopt!")
    add_definitions( -DDEBUG -D_DEBUG -DCONTROLLED_VARS_DEBUG )
else()
    message("Debug is turned OFF")
    add_definitions( -DNDEBUG )
endif()

add_subdirectory( src )
add_subdirectory( doc )

# vim: ts=4 sw=4 et nocindent
