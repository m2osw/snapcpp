#
# File:
#      lib/CMakeLists.txt
#
# Description:
#      Library used by the Snap! Websites server and plug-ins. All the functions
#      and classes that can be used in common are defined here
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

project(libsnapwebsites)

# Put the version in the header file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/snapwebsites.h.in ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites.h )

SnapIncludeQt4( QtCore QtXml )

include_directories( ${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR}/../../libtld/include )

QT4_ADD_RESOURCES(SNAP_LIBRARY_RESOURCES_RCC
    snap.qrc
)

add_library( ${PROJECT_NAME} SHARED
    dom_util.cpp            # DOM utilities
    http_cookie.cpp         # HTTP cookie, server side
    log.cpp                 # log4cplus support
    plugins.cpp             # plug-ins loader and list
    qdomnodemodel.cpp       # Qt extension to support XSLT
    qdomreceiver.cpp        # Qt extension to output XSLT as XML
    qhtmlserializer.cpp     # Qt extension to output HTML as a string
    snap_parser.cpp         # lexer/parser for all our script like entries
    snap_uri.cpp            # URI handling
    snap_child.cpp          # requests processing
    snapwebsites.cpp        # server
    tcp_client_server.cpp   # low level TCP/IP that works
)

target_link_libraries( ${PROJECT_NAME}
    QtSerialization
    log4cplus
    ${QT_LIBRARIES}
)

install(
    TARGETS libsnapwebsites LIBRARY DESTINATION lib/snapwebsites
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
    PATTERN .svn EXCLUDE
)

# At this time we suppose that all the plug-ins are compiled
# within our environment, however, we probably want to consider
# installing our headers at some point.

# vim: ts=4 sw=4 et
