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

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set( CASSANDRA_HOST_IP "127.0.0.1" CACHE STRING "Host IP address for the cassandra server to connect to." )

##
## The unified unit tests in one large test
##

find_package( Catch )

if(CATCH_FOUND)

    # TODO: turn this into a static library, because it takes a lot
    # longer to compile each test.
    set( SNAP_TESTS
        ${CMAKE_PREFIX_PATH}/share/snapwebsites/src/snap_tests.h
        ${CMAKE_PREFIX_PATH}/share/snapwebsites/src/snap_tests.cpp
    )

    include_directories( ${CMAKE_PREFIX_PATH}/share/snapwebsites/src )

    project(test_snap_parser)
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_parser.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project( test_unix_timestamp )
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_unix_timestamp.cpp
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project( test_entity_filter )
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_entity_filter.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project( test_ip_strings )
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_ip_strings.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    # This one is retired because we now use snap_communicator
    ##add_executable(test_snap_udp_message
    ##    test_snap_udp_message.cpp
    ##)
    ##target_link_libraries(test_snap_udp_message
    ##    ${SNAPWEBSITES_LIBRARIES}
    ##    ${LIBTLD_LIBRARIES}
    ##    ${QT_LIBRARIES}
    ##    dl
    ##)

    project(test_quoted_printable)
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_quoted_printable.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project(test_snap_child_date_parser)
    add_executable(${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_child_date_parser.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


    project(test_snap_version)
    add_executable( ${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_version.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )



    ##
    ## Check catching and displaying of snap exceptions
    ## (because we render the stack)
    ##
    project(test_snap_exception)
    add_executable( ${PROJECT_NAME}
        ${SNAP_TESTS}
        test_snap_exception.cpp
    )
    target_link_libraries(${PROJECT_NAME}
        ${SNAPWEBSITES_LIBRARIES}
        ${LOG4CPLUS_LIBRARIES}
        ${LIBTLD_LIBRARIES}
        ${QT_LIBRARIES}
        dl
    )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )


else(CATCH_FOUND)

    # You may use 'apt-get install catch' under Ubuntu
    message("Catch based tests will not be created because you do not seem to have catch.hpp installed...")

endif(CATCH_FOUND)



# vim: ts=4 sw=4 et
