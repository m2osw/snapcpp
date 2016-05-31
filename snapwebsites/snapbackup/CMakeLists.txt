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
#set(SNAPBACKUP_VERSION_MAJOR  0)
#set(SNAPBACKUP_VERSION_MINOR  1)
#set(SNAPBACKUP_VERSION_PATCH  1)

cmake_minimum_required(VERSION 2.8)

# Reasons for warnings that are turned off:
# * We use variadic macros for the signals, very useful to avoid otherwise
#   quite complicated statements or many macro definitions (one per # of params).
# * We use | in our parser grammar and we get many parenthesis warnings for those.
# Final flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")
# Development flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

# Other interesting warnings but that appear all over the place in other libraries
#   -Wsign-conversion  -- unsigned int a; int b; a = b; or b = a;
#   -Wmissing-declarations  -- we often create local functions without first declaring them

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
find_package( ControlledVars     REQUIRED )
find_package( Qt5Core            REQUIRED )
find_package( Qt5Network         REQUIRED )
find_package( Qt5Sql             REQUIRED )
find_package( QtCassandra        REQUIRED )
find_package( SnapCMakeModules   REQUIRED )

SnapGetVersion( SNAPBACKUP ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories( SYSTEM
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
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
)

add_subdirectory(src)

# vim: ts=4 sw=4 et nocindent
