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
#      Copyright (c) 2011-2013 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
    libsnapwebsites
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

install(
    FILES 
        log.conf
        snapserver.conf
        snapcgi.conf
        snap-apache2.conf
    DESTINATION
        share/snapwebsites/conf
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
    libsnapwebsites
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
    ${libsnapwebsites_SOURCE_DIR}
    )

add_executable(${PROJECT_NAME}
    snap.cpp
    ${libsnapwebsites_SOURCE_DIR}/tcp_client_server.cpp
)

set_property( TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME snap.cgi )

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${QT_LIBRARIES}
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
    libsnapwebsites
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

add_executable(${PROJECT_NAME}
    snaplayout.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
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
    libsnapwebsites
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
