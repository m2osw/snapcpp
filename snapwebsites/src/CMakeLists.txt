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
## Encode a username and password for basic auth
##
project(snap_uuencode_basic_auth)

add_executable(${PROJECT_NAME}
    snap_uuencode_basic_auth.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${QT_LIBRARIES}
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
## Snap signal manager
##
project(snapcommunicator)

add_executable(${PROJECT_NAME}
    snapcommunicator.cpp
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
## Snap firewall manager
##
project(snapfirewall)

add_executable(${PROJECT_NAME}
    snapfirewall.cpp
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
## Snap send signals
##
project(snapsignal)

add_executable(${PROJECT_NAME}
    snapsignal.cpp
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




################################################################################
# Snap Bounced Email Handler
#
# This is kicked off by the mail server if the email bounces. It saves
# the offending email into the cassanda database for later processing.
#
project(snapbounce)

add_executable(${PROJECT_NAME}
    snapbounce.cpp
)

target_link_libraries(${PROJECT_NAME}
    snapwebsites
    ${UUID}
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
## Build CGI
##
project(snapbuild)

add_executable(${PROJECT_NAME}
    build.cpp
)

set_property( TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME build.cgi )

# We install the binary so that way the linking is finalized, but
# we do not install that in a pacakage for now (maybe later if we
# do a build system for snap, although I don't think we need such.)
install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


#################################################################################
## Build CGI
##
project(build-service)

add_executable(${PROJECT_NAME}
    build-service.cpp
)

# We install the binary so that way the linking is finalized, but
# we do not install that in a pacakage for now (maybe later if we
# do a build system for snap, although I don't think we need such.)
install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)


# vim: ts=4 sw=4 et
