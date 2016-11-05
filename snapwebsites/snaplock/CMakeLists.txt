#
# File:
#      snapwebsites/snaplock/CMakeLists.txt
#
# Description:
#      A daemon to synchronize processes between any number of computers
#      by blocking all of them but one.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2016 Made to Order Software Corp.
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
cmake_minimum_required(VERSION 3.5)

project( snaplock_project )

# Find includes in corresponding build directories
set( CMAKE_INCLUDE_CURRENT_DIR ON )

# Find required packaging
find_package( AdvGetOpt          REQUIRED )
find_package( log4cplus          REQUIRED )
find_package( LibTLD             REQUIRED )
find_package( Qt5Core            REQUIRED )
find_package( Qt5Xml             REQUIRED )
find_package( SnapCMakeModules   REQUIRED )
find_package( SnapWebsites       REQUIRED )
find_package( SnapDoxygen )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Xml
)
include_directories(
    ${SNAPWEBSITES_INCLUDE_DIRS}
)
SnapGetVersion( SNAPLOCK ${CMAKE_CURRENT_SOURCE_DIR} )

option( RUN_TESTS "Enable tests to run via CTest" OFF )
if( ${RUN_TESTS} )
    enable_testing()
endif()

add_subdirectory( conf  )
add_subdirectory( src   )
add_subdirectory( doc   )
add_subdirectory( tests )

##
## Analyse xml and javascript
##
#snap_build_xml_targets()

# vim: ts=4 sw=4 et
