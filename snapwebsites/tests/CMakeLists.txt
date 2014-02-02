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

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore QtNetwork QtXml QtXmlPatterns )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QT_INCLUDES}
        )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_custom_target( test_prereqs DEPENDS make_controlled_vars_headers )
else()
    add_custom_target( test_prereqs )
endif()

##
## Send orders to the Snap Server via the snap.cgi tool
##
project(test_snap_cgi)


add_executable(test_snap_cgi
    test_snap_cgi.cpp
)
add_dependencies( test_snap_cgi test_prereqs )
# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(test_snap_cgi
    #snapwebsites
    #QtCassandra
    #QtCore
    #QtXml
    #QtNetwork
    #thrift
    #tld
)


add_executable(test_snap_parser
    test_snap_parser.cpp
)
add_dependencies( test_snap_parser test_prereqs )
target_link_libraries(test_snap_parser
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


add_executable(test_unix_timestamp
    test_unix_timestamp.cpp
)
add_dependencies( test_unix_timestamp test_prereqs )


add_executable(test_snap_udp_message
    test_snap_udp_message.cpp
)
add_dependencies( test_snap_udp_message test_prereqs )
target_link_libraries(test_snap_udp_message
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


add_executable(test_quoted_printable
    test_quoted_printable.cpp
)
add_dependencies( test_quoted_printable test_prereqs )
target_link_libraries(test_quoted_printable
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


add_executable(test_snap_child_date_parser
    test_snap_child_date_parser.cpp
)
add_dependencies( test_snap_child_date_parser test_prereqs )
target_link_libraries(test_snap_child_date_parser
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)



add_executable( test_snap_version
    test_snap_version.cpp
)
add_dependencies( test_snap_version test_prereqs )
target_link_libraries(test_snap_version
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


# vim: ts=4 sw=4 et
