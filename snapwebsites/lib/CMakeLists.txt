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

project(snapwebsites)

set( CMAKE_AUTOMOC TRUE )

# Put the version in the header file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/snapwebsites.h.in ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites.h )

find_package( log4cplus REQUIRED )
find_package( Qt4 4.8.1 REQUIRED QtCore QtXml QtXmlPatterns )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

option( SNAP_NO_FORK OFF "Set to suppress snapserver's inate forking [for debugging purposes only--not for release!]" )
if( ${SNAP_NO_FORK} )
    add_definitions( -DSNAP_NO_FORK )
endif()

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
    compression.cpp                             # Compress/decompress data
    dbutils.h                                   # Utilities to help convert coded table and row names
    dbutils.cpp                                 # and column data.
    dom_util.cpp                                # DOM utilities
    http_cookie.cpp                             # HTTP cookie, server side
    http_strings.cpp                            # parse HTTP strings (Accept strings in particular)
    log.cpp                                     # log4cplus support
    mkgmtime.c                                  # mktime() with a UTC time
    plugins.cpp                                 # plugins loader and list
    process.cpp                                 # run a process optionally with pipes
    qdomnodemodel.cpp                           # Qt extension to support XSLT
    qdomreceiver.cpp                            # Qt extension to output XSLT as XML
    qdomxpath.cpp                               # Qt extension to query a XML DOM with X-Path
    qhtmlserializer.cpp                         # Qt extension to output HTML as a string
    qstring_stream.h                            # Stream QString objects property to ostream objects.
    quoted_printable.cpp                        # to encode / decode data using the quoted printable format
    qxmlmessagehandler.cpp                      # Qt extension for us to capture the QXmlQuery messages
	row_model.cpp                               #  libQtCassandra row model for use with a QTableView object.
	snap_child.cpp                              # requests processing
    snap_exception.cpp                          # exception stack backtrack
    snap_expr.cpp                               # snap C-like expression compiler and execution environment
    snap_image.cpp                              # image handling
    snap_magic.cpp                              # support for MIME type
    snap_parser.cpp                             # lexer/parser for all our script like entries
    snap_pipe.cpp                               # RAII popen()/pclose() class
    snap_thread.cpp                             # a C++ thread implementation
    snap_uri.cpp                                # URI handling
    snap_utf8.cpp                               # UTF-8 functions
    snap_version.cpp                            # extract name, version, browser info from a filename
    snapwebsites.h.in                           # Header for server class (pre-parsed)
    ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites.h  # binary output of header file (generated)
    snapwebsites.cpp                            # server
    tcp_client_server.cpp                       # low level TCP/IP that works
	table_model.cpp                             # libQtCassandra table model for use with a QListView object.
	udp_client_server.cpp                       # low level UDP/IP that works
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
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites
    FILES_MATCHING PATTERN "*.h"
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites.h
    DESTINATION include/snapwebsites
)

# At this time we suppose that all the plugins are compiled
# within our environment, however, we probably want to consider
# installing our headers at some point.

# vim: ts=4 sw=4 et nocindent
