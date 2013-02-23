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


##
## The Snap! Server
##
SnapIncludeQt4( QtCore QtNetwork QtXml QtXmlPatterns )

project(snapserver)

include_directories(
    #"${CMAKE_CURRENT_SOURCE_DIR}/../../lib"
    #"${CMAKE_CURRENT_BINARY_DIR}/../../lib"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)


add_executable(snapserver
    snapserver.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(snapserver
    libsnapwebsites
    QtCassandra
    QtSerialization
    ${QT_LIBRARIES}
    thrift
    tld
    dl
)

install(
    TARGETS snapserver RUNTIME DESTINATION bin
)


##
## Backend process
##
project(snapbackend)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)


add_executable(snapbackend
    snapbackend.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(snapbackend
    libsnapwebsites
    QtCassandra
    QtSerialization
    ${QT_LIBRARIES}
    thrift
    tld
    dl
)

install(
    TARGETS snapbackend RUNTIME DESTINATION bin
)


##
## The Snap! Passthrough CGI script
##
project(snapcgi)

include_directories(
    #"${CMAKE_CURRENT_SOURCE_DIR}/../lib"
    #"${CMAKE_CURRENT_BINARY_DIR}/../lib"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)

add_executable(snap.cgi
    snap.cpp
    ../lib/tcp_client_server.cpp
)

target_link_libraries(snap.cgi
    ${QT_LIBRARIES}
)

install(
    TARGETS snap.cgi RUNTIME DESTINATION bin
)

##
## Snap DB process
##
project(snapdb)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)


add_executable(snapdb
    snapdb.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(snapdb
    #libsnapwebsites
    QtCassandra
    #QtSerialization
    ${QT_LIBRARIES}
    thrift
    tld
)

install(
    TARGETS snapdb RUNTIME DESTINATION bin
)


##
## Snap layout process
##
project(snaplayout)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)


add_executable(snaplayout
    snaplayout.cpp
)

target_link_libraries(snaplayout
    #libsnapwebsites
    QtCassandra
    #QtSerialization
    ${QT_LIBRARIES}
    thrift
    tld
)

install(
    TARGETS snaplayout RUNTIME DESTINATION bin
)


# vim: ts=4 sw=4 et
