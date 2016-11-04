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

project(snapwebsites)

# Put the version in the header file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/snapwebsites/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites/version.h )


option( SNAP_NO_FORK OFF "Set to suppress snapserver's inate forking [for debugging purposes only--not for release!]" )
if( ${SNAP_NO_FORK} )
    message( WARNING "SNAP_NO_FORK is turned on! This is not a production-ready build!" )
    add_definitions( -DSNAP_NO_FORK )
endif()


include_directories(
    SYSTEM
        ${MAGIC_INCLUDE_DIRS}
        ${LIBPROCPS_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
)

include_directories(
    ${LIBTLD_INCLUDE_DIRS}
    ${LOG4CPLUS_INCLUDE_DIRS}
)

qt5_add_resources(SNAP_LIBRARY_RESOURCES_RCC
    snap.qrc
)
set( QTCASSANDRA_HEADER_FILES
    QtCassandra/QCassandra.h
    QtCassandra/QCassandraCell.h
    QtCassandra/QCassandraConsistencyLevel.h
    QtCassandra/QCassandraContext.h
    QtCassandra/QCassandraLock.h
    QtCassandra/QCassandraOrder.h
    QtCassandra/QCassandraOrderResult.h
    QtCassandra/QCassandraPredicate.h
    QtCassandra/QCassandraProxy.h
    QtCassandra/QCassandraRow.h
    QtCassandra/QCassandraTable.h
    QtCassandra/QCassandraValue.h
    QtCassandra/QCassandraVersion.h
)

set( QTCASSANDRA_SOURCE_FILES
    QtCassandra/QCassandra.cpp
    QtCassandra/QCassandraCell.cpp
    QtCassandra/QCassandraContext.cpp
    QtCassandra/QCassandraLock.cpp
    QtCassandra/QCassandraOrder.cpp
    QtCassandra/QCassandraOrderResult.cpp
    QtCassandra/QCassandraPredicate.cpp
    QtCassandra/QCassandraProxy.cpp
    QtCassandra/QCassandraRow.cpp
    QtCassandra/QCassandraTable.cpp
    QtCassandra/QCassandraValue.cpp
)

set( HEADERS
    ${QTCASSANDRA_HEADER_FILES}
    snapwebsites/addr.h
    snapwebsites/cache_control.h
    snapwebsites/chownnm.h
    snapwebsites/compression.h
    snapwebsites/dbutils.h
    snapwebsites/file_content.h
    snapwebsites/floats.h
    snapwebsites/fuzzy_string_compare.h
    snapwebsites/http_client_server.h
    snapwebsites/http_cookie.h
    snapwebsites/http_strings.h
    snapwebsites/inet_helpers.h
    snapwebsites/join_strings.h
    snapwebsites/loadavg.h
    snapwebsites/lockfile.h
    snapwebsites/log.h
    snapwebsites/minmax.h
    snapwebsites/mkdir_p.h
    snapwebsites/mkgmtime.h
    snapwebsites/mounts.h
    snapwebsites/not_reached.h
    snapwebsites/not_used.h
    snapwebsites/plugins.h
    snapwebsites/poison.h
    snapwebsites/process.h
    snapwebsites/qcaseinsensitivestring.h
    snapwebsites/qdomhelpers.h
    snapwebsites/qdomnodemodel.h
    snapwebsites/qdomreceiver.h
    snapwebsites/qdomxpath.h
    snapwebsites/qhtmlserializer.h
    snapwebsites/qlockfile.h
    snapwebsites/qstring_stream.h
    snapwebsites/quoted_printable.h
    snapwebsites/qxmlmessagehandler.h
    snapwebsites/reverse_cstring.h
    snapwebsites/snap_backend.h
    snapwebsites/snap_cassandra.h
    snapwebsites/snap_child.h
    snapwebsites/snap_communicator.h
    snapwebsites/snap_config.h
    snapwebsites/snap_exception.h
    snapwebsites/snap_expr.h
    snapwebsites/snap_image.h
    snapwebsites/snap_initialize_website.h
    snapwebsites/snap_listen_thread.h
    snapwebsites/snap_lock.h
    snapwebsites/snap_magic.h
    snapwebsites/snap_parser.h
    snapwebsites/snap_pipe.h
    snapwebsites/snap_signals.h
    snapwebsites/snap_string_list.h
    snapwebsites/snap_tables.h
    snapwebsites/snap_thread.h
    snapwebsites/snap_uri.h
    snapwebsites/snap_utf8.h
    snapwebsites/snap_version.h
    snapwebsites/snapwebsites.h
    snapwebsites/string_pathinfo.h
    snapwebsites/string_replace.h
    snapwebsites/tcp_client_server.h
    snapwebsites/tokenize_string.h
    snapwebsites/udp_client_server.h
    snapwebsites/version.h.in
    snapwebsites/xslt.h
)

add_library( ${PROJECT_NAME} SHARED
    ${HEADERS}
    ${QTCASSANDRA_SOURCE_FILES}
    addr.cpp                                    # handling of host IP addresses (IPv4 and IPv6)
    cache_control.cpp                           # parse / handle Cache-Control settings
    chownnm.cpp                                 # chown using user and group names instead of uid/gid
    compression.cpp                             # compress/decompress data
    dbutils.cpp                                 # utilities to help convert coded table and row names and column data. (see snap_tables.cpp too!)
    file_content.cpp                            # load / save whole files in one go
    floats.cpp                                  # Floats helper functions
    fuzzy_string_compare.cpp                    # Compare strings in a fuzzy manner
    http_client_server.cpp                      # implementation of the HTTP protocol
    http_cookie.cpp                             # HTTP cookie, server side
    http_strings.cpp                            # parse HTTP strings (Accept strings in particular)
    inet_helpers.cpp                            # helper functions in link with the network
    loadavg.cpp                                 # handle the loadavg file
    log.cpp                                     # log4cplus support
    mkdir_p.cpp                                 # make directories so path is valid
    mkgmtime.c                                  # mktime() with a UTC time
    mounts.cpp                                  # read /proc/mounts or some other mount point file
    plugins.cpp                                 # plugins loader and list
    process.cpp                                 # run a process optionally with pipes
    qdomhelpers.cpp                             # Qt extensions to help with the DOM
    qdomnodemodel.cpp                           # Qt extension to support XSLT
    qdomreceiver.cpp                            # Qt extension to output XSLT as XML
    qdomxpath.cpp                               # Qt extension to query a XML DOM with X-Path
    qhtmlserializer.cpp                         # Qt extension to output HTML as a string
    quoted_printable.cpp                        # to encode / decode data using the quoted printable format
    qxmlmessagehandler.cpp                      # Qt extension for us to capture the QXmlQuery messages
    snap_backend.cpp                            # type of snap_child for backends
    snap_cassandra.cpp                          # encapuslate initiating a connection to the database.
    snap_communicator.cpp                       # handle any number of sockets with a single process.
    snap_config.cpp                             # class to read in configuration
    snap_child.cpp                              # requests processing
    snap_child_cache_control.cpp                # manage cache controls in the snap_child class
    snap_exception.cpp                          # exception stack backtrack
    snap_expr.cpp                               # snap C-like expression compiler and execution environment
    snap_image.cpp                              # image handling
    snap_initialize_website.cpp                 # initialize website by sending an Apache-like query to the server
    snap_lock.cpp                               # support for inter-process / inter-computer locks using network and memory only
    snap_magic.cpp                              # support for MIME type
    snap_parser.cpp                             # lexer/parser for all our script like entries
    snap_pipe.cpp                               # RAII popen()/pclose() class
    snap_listen_thread.cpp                      # thread which listens on UDP and allows snapsignal to direct snapserver
    snap_tables.cpp                             # table schema/definitions
    snap_thread.cpp                             # a C++ thread implementation
    snap_uri.cpp                                # URI handling
    snap_utf8.cpp                               # UTF-8 functions
    snap_version.cpp                            # extract name, version, browser info from a filename
    snapwebsites.cpp                            # server
    tcp_client_server.cpp                       # low level TCP/IP that works
    udp_client_server.cpp                       # low level UDP/IP that works
    xslt.cpp                                    # code to do XSLT transformations
)

target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LOG4CPLUS_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${AS2JS_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${MAGIC_LIBRARIES}
    ${LIBPROCPS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${UUID}
    pthread
    dl
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${LIBSNAPWEBSITES_VERSION_MAJOR}.${LIBSNAPWEBSITES_VERSION_MINOR}
    SOVERSION ${LIBSNAPWEBSITES_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)
install(
    FILES "core-tables.xml"
    DESTINATION ${SNAP_TABLE_DESCRIPTION_PATH}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "snapwebsites/*.h"
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "QtCassandra/*.h"
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites/version.h
    DESTINATION include/snapwebsites
)

# vim: ts=4 sw=4 et nocindent
