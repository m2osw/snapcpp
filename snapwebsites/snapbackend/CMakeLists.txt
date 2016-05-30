#
# File:
#      src/CMakeLists.txt
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
## Backend process
##
project(snapbackend)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wold-style-cast -Wnoexcept -DUNISTR_FROM_STRING_EXPLICIT=explicit -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

find_package( log4cplus       REQUIRED )
find_package( LibTLD          REQUIRED )
find_package( Qt5Core         REQUIRED )
find_package( Qt5Xml          REQUIRED )
find_package( QtCassandra     REQUIRED )
find_package( QtSerialization REQUIRED )
find_package( SnapWebsites    REQUIRED )

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

add_executable(${PROJECT_NAME}
    snapbackend.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${SNAPWEBSITES_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


# vim: ts=4 sw=4 et
