#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment
#
# Copyright:    Copyright (c) 2012-2017 Made to Order Software Corp.
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

project(iplock_project)

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    add_definitions( "-DMO_DEBUG" )
endif()

find_package( AdvGetOpt        REQUIRED )
find_package( Boost            COMPONENTS filesystem system REQUIRED )
find_package( LibAddr          REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen )

SnapGetVersion( IPLOCK ${CMAKE_CURRENT_SOURCE_DIR} )

# Sub-directories
add_subdirectory(src)
add_subdirectory(doc)


# vim: ts=4 sw=4 et
