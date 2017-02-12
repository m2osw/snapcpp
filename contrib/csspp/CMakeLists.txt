#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      csspp -- a CSS Preprocessor
#      Copyright (c) 2015-2017 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

##
## Initialization
##
cmake_minimum_required(VERSION 2.8.4)

project( csspp_project )

enable_language( CXX )
enable_testing()

## Include support modules
##
#if( NOT CMAKE_MODULE_PATH )
#    set( CMAKE_MODULE_PATH ${CMAKE_PREFIX_PATH}/share/cmake )
#endif()

set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )

find_package( AdvGetOpt        REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen )

SnapGetVersion( CSSPP ${CMAKE_CURRENT_SOURCE_DIR} )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message("Debug is in effect for csspp!")
    add_definitions( -DDEBUG -D_DEBUG )
else()
    message("Debug is turned OFF")
    add_definitions( -DNDEBUG )
endif()
# A few extra warnings specifically for snapwebsites libraries and tools
# You can also play with -Weffc++ although we are definitively not compliant
# (especially we do not define all the auto-initialized variables!)
# -Wconversion -- would be nice, a few things are still not cooperating with that one
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept" )

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}/include
)

##
## Compiling
##
add_subdirectory(include)       # public headers
add_subdirectory(lib)           # csspp library
add_subdirectory(scripts)       # command line tools
add_subdirectory(src)           # command line tools
add_subdirectory(tests)         # tests
add_subdirectory(doc)           # library API documentation

# Local Variables:
# indent-tabs-mode: nil
# tab-width: 4
# End:

# vim: ts=4 sw=4 et nocindent
