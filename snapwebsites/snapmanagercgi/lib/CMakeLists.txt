#
# File:
#      lib/CMakeLists.txt
#
# Description:
#      Library used by the Snap! Manager CGI/Daemon server and plugins.
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

project(snapmanager)

set( CMAKE_AUTOMOC TRUE )

# Put the version in the header file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/manager.h.in ${CMAKE_CURRENT_BINARY_DIR}/manager.h )

#find_package( log4cplus REQUIRED )
find_package( Qt5Core        REQUIRED )
find_package( Qt5Xml         REQUIRED )
find_package( Qt5XmlPatterns REQUIRED )

include_directories(
    SYSTEM
        #${LOG4CPLUS_INCLUDE_DIR}
        ${QT_INCLUDES}
)

include_directories(
    ${SNAPWEBSITES_INCLUDE_DIRS}
)

add_library( ${PROJECT_NAME} SHARED
    manager.cpp                                 # Implements the core of the watchdog server
    server_status.cpp                           # Handle status data
    status.cpp                                  # Hanele one specific status
)

target_link_libraries( ${PROJECT_NAME}
    #${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    #${LIBTLD_LIBRARIES}
    Qt5::Core
    Qt5::Xml
    Qt5::XmlPatterns
    #${QTCASSANDRA_LIBRARIES}
    #${QTSERIALIZATION_LIBRARIES}
    #${MAGIC_LIBRARY}
    #${LIBPROCPS_LIBRARIES}
    #pthread
    #dl
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${SNAPMANAGERCGI_VERSION_MAJOR}.${SNAPMANAGERCGI_VERSION_MINOR}
    SOVERSION ${SNAPMANAGERCGI_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapmanager
    FILES_MATCHING PATTERN "*.h"
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/manager.h
    DESTINATION include/snapmanager
)

# vim: ts=4 sw=4 et nocindent
