#
# File:
#      snapwebsites/snapbackup/CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2015 Made to Order Software Corp.
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

cmake_minimum_required(VERSION 3.5)

project( snapbackup_project )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

## Include support modules
##
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv"                       )

find_package( AdvGetOpt          REQUIRED )
find_package( CassWrapper        REQUIRED )
find_package( Qt5Core            REQUIRED )
find_package( Qt5Network         REQUIRED )
find_package( Qt5Sql             REQUIRED )
find_package( SnapCMakeModules   REQUIRED )
find_package( SnapWebsites       REQUIRED )

SnapGetVersion( SNAPBACKUP ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories( SYSTEM
    ${CASSWRAPPER_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Sql_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Sql
)

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

add_subdirectory( conf            )
add_subdirectory( src             )
add_subdirectory( manager-plugins )

snap_build_xml_targets()

# vim: ts=4 sw=4 et nocindent
