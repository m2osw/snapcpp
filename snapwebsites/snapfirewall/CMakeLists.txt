#
# File:
#      snapwebsites/snapfirewall/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. The server is the main() of the Snap!
#      environment. Most everything is however defined in the
#      libsnapwebsites.so library (see lib/*).
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2016 Made to Order Software Corp.
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

#################################################################################
## The Snap! Passthrough CGI script
##
project(snapfirewall_project)

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    add_definitions( "-DMO_DEBUG" )
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set( SNAP_TABLE_DESCRIPTION_PATH lib/snapwebsites/tables )

find_package( As2Js            REQUIRED )
find_package( AdvGetOpt        REQUIRED )
find_package( CassValue        REQUIRED )
find_package( LibExcept        REQUIRED )
find_package( LibTLD           REQUIRED )
find_package( log4cplus        REQUIRED )
find_package( Qt5Core          REQUIRED )
find_package( Qt5Sql           REQUIRED )
find_package( Qt5Xml           REQUIRED )
find_package( QtCassandra      REQUIRED )
find_package( QtSerialization  REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen               )
find_package( SnapManager      REQUIRED )
find_package( SnapWebsites     REQUIRED )

SnapGetVersion( SNAPFIREWALL ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories(
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${SNAPWEBSITES_INCLUDE_DIRS}
    ${SNAPMANAGERCGI_INCLUDE_DIR}
)
include_directories(
    SYSTEM
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Sql_INCLUDE_DIRS}
        ${Qt5Xml_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Sql
    Qt5::Xml
)

add_subdirectory( conf            )
add_subdirectory( src             )
add_subdirectory( manager-plugins )
add_subdirectory( doc             )

snap_build_xml_targets()

# vim: ts=4 sw=4 et
