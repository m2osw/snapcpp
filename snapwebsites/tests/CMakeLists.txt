#
# File:
#      tests/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. This CMake compiles tests that can be used
#      against the server. All C++ tests should be converted to a catch_*
#      test instead of its own brew. Now that we have a test bed ready for
#      such the conversion should be easy enough. We will still have separate
#      tests that are external processes we need to run against the main test.
#      (i.e. a test for client/server network connections.)
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2015 Made to Order Software Corp.
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
    SYSTEM
        ${QT_INCLUDES}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_custom_target( test_prereqs DEPENDS make_controlled_vars_headers )
else()
    add_custom_target( test_prereqs )
endif()

##
## The unified unit tests in one large test
##
project(test_snap)


add_executable(test_snap
    # Main
    catch_tests.cpp

    # Test suites
    catch_snap_communicator.cpp
)
add_dependencies( test_snap test_prereqs )
target_link_libraries(test_snap
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


##
## A client used to test our Snap! Communicator
##
project(test_snap_communicator_client)


add_executable(test_snap_communicator_client
    # Main
    test_snap_communicator_client.cpp
)
add_dependencies( test_snap_communicator_client test_prereqs )
target_link_libraries(test_snap_communicator_client
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


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


add_executable(test_entity_filter
    test_entity_filter.cpp
)
add_dependencies( test_entity_filter test_prereqs )
target_link_libraries(test_entity_filter
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


add_executable(test_ip_strings
    test_ip_strings.cpp
)
add_dependencies( test_ip_strings test_prereqs )
target_link_libraries(test_ip_strings
    snapwebsites
    ${QTCASSANDRA_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    dl
)


# This one is retired because we now use snap_communicator
##add_executable(test_snap_udp_message
##    test_snap_udp_message.cpp
##)
##add_dependencies( test_snap_udp_message test_prereqs )
##target_link_libraries(test_snap_udp_message
##    snapwebsites
##    ${QTCASSANDRA_LIBRARIES}
##    ${LIBTLD_LIBRARIES}
##    ${QT_LIBRARIES}
##    ${THRIFT_LIBRARIES}
##    dl
##)


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



##
## Check catching and displaying of snap exceptions
## (because we render the stack)
##
add_executable( test_snap_exception
	test_snap_exception.cpp
)
add_dependencies( test_snap_exception test_prereqs )
target_link_libraries(test_snap_exception
	snapwebsites
	${QTCASSANDRA_LIBRARIES}
	${LIBTLD_LIBRARIES}
	${QT_LIBRARIES}
	${THRIFT_LIBRARIES}
	dl
)


##
## Test a client/server via snap_communicator to see whether shutdown()
## has the same effect in the parent as it has in the client
##
add_executable( test_shutdown_server
	test_shutdown_server.cpp
)
add_dependencies( test_shutdown_server test_prereqs )
target_link_libraries(test_shutdown_server
	snapwebsites
	${QTCASSANDRA_LIBRARIES}
	${LIBTLD_LIBRARIES}
	${QT_LIBRARIES}
	${THRIFT_LIBRARIES}
	dl
)

add_executable( test_shutdown_client
	test_shutdown_client.cpp
)
add_dependencies( test_shutdown_client test_prereqs )
target_link_libraries(test_shutdown_client
	snapwebsites
	${QTCASSANDRA_LIBRARIES}
	${LIBTLD_LIBRARIES}
	${QT_LIBRARIES}
	${THRIFT_LIBRARIES}
	dl
)


##
## String Speed (testing a Qt feature)
##
add_executable( string_speed
	string_speed.cpp
)
target_link_libraries(string_speed
	#snapwebsites
	#${QTCASSANDRA_LIBRARIES}
	#${LIBTLD_LIBRARIES}
	${QT_LIBRARIES}
	#${THRIFT_LIBRARIES}
	#dl
)


##
## Check gethostbyname()
##
add_executable( gethostips
	gethostips.cpp
)
target_link_libraries(gethostips
	#snapwebsites
	#${QTCASSANDRA_LIBRARIES}
	#${LIBTLD_LIBRARIES}
	#${QT_LIBRARIES}
	#${THRIFT_LIBRARIES}
	#dl
)



# vim: ts=4 sw=4 et
