#
# File:
#      src/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. This CMake compiles tests that can be used
#      against the server.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2012 Made to Order Software Corp.
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

SnapIncludeQt4( QtCore QtNetwork QtXml QtXmlPatterns )
include_directories( ${QT_INCLUDES} )

##
## Send orders to the Snap Server via the snap.cgi tool
##
project(test_snap_cgi)

include_directories(
    #"${CMAKE_SOURCE_DIR}/lib"
    #"${CMAKE_BINARY_DIR}/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)


add_executable(test_snap_cgi
    test_snap_cgi.cpp
)

add_executable(test_snap_parser
    test_snap_parser.cpp
)

add_executable(test_unix_timestamp
    test_unix_timestamp.cpp
)

add_executable(test_snap_udp_message
    test_snap_udp_message.cpp
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(test_snap_cgi
    #libsnapwebsites
    #QtCassandra
    #QtCore
    #QtXml
    #QtNetwork
    #thrift
    #tld
)

target_link_libraries(test_snap_parser
    libsnapwebsites
    ${QT_LIBRARIES}
    QtCassandra
    thrift
    tld
    dl
)

target_link_libraries(test_snap_udp_message
    libsnapwebsites
    ${QT_LIBRARIES}
    QtCassandra
    thrift
    tld
    dl
)


# vim: ts=4 sw=4 et
