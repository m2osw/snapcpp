#
# File:
#      snapmanagercgi/CMakeLists.txt
#
# Description:
#      The Snap! Manager CGI to manage a Snap! cluster from your
#      browser.
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

##
## The Snap! Manager CGI
##
project( snapmanagercgi_project )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

##
## Version
##
set(SNAPMANAGERCGI_VERSION_MAJOR 1)
set(SNAPMANAGERCGI_VERSION_MINOR 0)
set(SNAPMANAGERCGI_VERSION_PATCH 0)
add_definitions( -DSNAPMANAGERCGI_VERSION=\"${SNAPMANAGERCGI_VERSION_MAJOR}.${SNAPMANAGERCGI_VERSION_MINOR}.${SNAPMANAGERCGI_VERSION_PATCH}\" )

##
## CMake module path
##
if( NOT CMAKE_PREFIX_PATH )
    set( CMAKE_PREFIX_PATH "/usr" )
endif()

set( CMAKE_MODULE_PATH
    ${CMAKE_PREFIX_PATH}/share/cmake/Modules
    ${CMAKE_MODULE_PATH}
)

##
## Find required packages
##
find_package( AdvGetOpt         REQUIRED )
find_package( As2Js 0.1.0       REQUIRED )
find_package( LibTLD            REQUIRED )
find_package( log4cplus         REQUIRED )
find_package( ControlledVars    REQUIRED )
find_package( Qt5Core           REQUIRED )
find_package( Qt5Network        REQUIRED )
find_package( Qt5Xml            REQUIRED )
find_package( QtCassandra       REQUIRED )
find_package( QtSerialization   REQUIRED )
find_package( SnapCMakeModules  REQUIRED )
find_package( SnapWebsites      REQUIRED )


set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Xml
)


add_subdirectory( src )


# vim: ts=4 sw=4 et nocindent
