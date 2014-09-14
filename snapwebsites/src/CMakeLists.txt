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
#      Copyright (c) 2011-2014 Made to Order Software Corp.
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

#################################################################################
## Dependencies
##
find_package( Qt4 4.8.1 QUIET REQUIRED QtCore QtNetwork QtXml QtXmlPatterns )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )


#################################################################################
## The Snap! Server
##
project(snapserver)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QT_INCLUDES}
)

add_executable(${PROJECT_NAME}
    snapserver.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Backend process
##
project(snapbackend)

add_executable(${PROJECT_NAME}
    snapbackend.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## The Snap! Passthrough CGI script
##
project(snapcgi)

include_directories(
    ${snapwebsites_SOURCE_DIR}
    )

add_executable(${PROJECT_NAME}
    snap.cpp
    ${snapwebsites_SOURCE_DIR}/tcp_client_server.cpp
)

set_property( TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME snap.cgi )

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${ADVGETOPT_LIBRARIES}
    ${QT_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION lib/cgi-bin
)


#################################################################################
## Snap Sites table handling
##
project(snapsiteinfo)

add_executable(${PROJECT_NAME}
    snapsiteinfo.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${ADVGETOPT_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Snap DB process
##
project(snapdb)

add_executable(${PROJECT_NAME}
    snapdb.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${ADVGETOPT_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Snap send signals
##
project(snapsignal)

add_executable(${PROJECT_NAME}
    snapsignal.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Snap layout process
##
project(snaplayout)

include_directories( ${ZIPIOSCC_INCLUDE_DIRS} )

add_executable(${PROJECT_NAME}
    snaplayout.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
	${ADVGETOPT_LIBRARIES}
	${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
	${ZIPIOSCC_LIBRARIES}
    dl
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## XPath compiler
##
project(cxpath)

add_executable(${PROJECT_NAME}
    cxpath.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Snap Image Information Tool
##
project(snapimg)

add_executable(${PROJECT_NAME}
    snapimg.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Snap Expression Computation and Compilation
##
project(snapexpr)

add_executable(${PROJECT_NAME}
    snapexpr.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Snap Initalization Server
##
## This server reads in a configuration file and keeps specified services running.
## When signaled, it will terminate those services cleanly.
##
project(snapinit)

add_executable(${PROJECT_NAME}
    snapinit.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


# vim: ts=4 sw=4 et
