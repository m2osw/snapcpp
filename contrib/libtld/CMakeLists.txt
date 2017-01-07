#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2011-2017 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/project/libtld
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

cmake_minimum_required(VERSION 2.8.4)

project(tld_library)

#
# Include support modules
# (must be included in a project even though it is not made
# specific to that project)
#
find_package( SnapCMakeModules )
find_package( SnapDoxygen      )
find_package( Boost            REQUIRED )
find_package( Qt5Core          REQUIRED )
find_package( Qt5Xml           REQUIRED )

SnapGetVersion( LIBTLD ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Xml
)


add_subdirectory( cmake   )
add_subdirectory( include )
add_subdirectory( src     )
add_subdirectory( doc     )
add_subdirectory( tests   )
add_subdirectory( package )

# vim: ts=4 sw=4 et

