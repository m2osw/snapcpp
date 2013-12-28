#
# File:
#      lib/CMakeLists.txt
#
# Description:
#      Library used by the Snap! Websites server and plugins. All the functions
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

find_package( log4cplus REQUIRED )
find_package( Qt4 4.8.1 REQUIRED QtCore QtXml QtXmlPatterns )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

find_file( MAGIC magic.h )
if( ${MAGIC} STREQUAL "MAGIC-NOTFOUND" )
    message( FATAL_ERROR "Please install libmagic!" )
endif()
get_filename_component( MAGIC_INCLUDE_PATH ${MAGIC} PATH )

find_library( MAGIC_LIBRARY magic )
if( ${MAGIC_LIBRARY} STREQUAL "MAGIC_LIBRARY-NOTFOUND" )
    message( FATAL_ERROR "Please install libmagic!" )
endif()

include_directories(
    ${MAGIC_INCLUDE_PATH}
    ${LOG4CPLUS_INCLUDE_DIR}
    ${QT_INCLUDES}
)

QT4_ADD_RESOURCES(SNAP_LIBRARY_RESOURCES_RCC
    snap.qrc
)

add_library( ${PROJECT_NAME} SHARED
    compression.cpp         # Compress/decompress data
    dom_util.cpp            # DOM utilities
    http_cookie.cpp         # HTTP cookie, server side
    http_strings.cpp        # parse HTTP strings (Accept strings in particular)
    log.cpp                 # log4cplus support
    mkgmtime.c              # mktime() with a UTC time
    plugins.cpp             # plugins loader and list
    process.cpp             # run a process optionally with pipes
    qdomnodemodel.cpp       # Qt extension to support XSLT
    qdomreceiver.cpp        # Qt extension to output XSLT as XML
    qdomxpath.cpp           # Qt extension to query a XML DOM with X-Path
    qhtmlserializer.cpp     # Qt extension to output HTML as a string
    quoted_printable.cpp    # to encode / decode data using the quoted printable format
    snap_child.cpp          # requests processing
    snap_image.cpp          # image handling
    snap_magic.cpp          # support for MIME type
    snap_parser.cpp         # lexer/parser for all our script like entries
    snap_thread.cpp         # a C++ thread implementation
    snap_uri.cpp            # URI handling
    snap_utf8.cpp           # UTF-8 functions
    snapwebsites.cpp        # server
    tcp_client_server.cpp   # low level TCP/IP that works
    udp_client_server.cpp   # low level UDP/IP that works
)

target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${MAGIC_LIBRARY}
    pthread
    dl
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}
    SOVERSION ${SNAPWEBSITES_VERSION_MAJOR}
)

install(
    TARGETS libsnapwebsites LIBRARY DESTINATION lib
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites
    FILES_MATCHING PATTERN "*.h"
    PATTERN .svn EXCLUDE
)

# At this time we suppose that all the plugins are compiled
# within our environment, however, we probably want to consider
# installing our headers at some point.

# vim: ts=4 sw=4 et
