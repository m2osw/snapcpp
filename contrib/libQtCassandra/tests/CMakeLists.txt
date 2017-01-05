#
# Text:
#      tests/CMakeLists.txt
#
# Description:
#      Compile and run libQtCassandra tests.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2017 Made to Order Software Corp.
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

################################################################################
include_directories(
    ${AS2JS_INCLUDE_DIR}
    ${CASSANDRACPPDRIVER_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    ${QtCassandra_SOURCE_DIR}
    ${QtCassandra_BINARY_DIR}
)

################################################################################
# Change the IP as required by your environment
set( DEFAULT_CASSANDRA_IP "$ENV{CASSANDRA_IP}" )
if( NOT DEFAULT_CASSANDRA_IP )
    set( DEFAULT_CASSANDRA_IP "localhost" )
endif()
set( CASSANDRA_IP "${DEFAULT_CASSANDRA_IP}" CACHE STRING "IP address of the cassandra server" )

###############################################################################
project(cluster)


add_executable(cluster
    cluster.cpp
)

target_link_libraries(cluster
    QtCassandra
)

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

################################################################################
project(cassandra_value)

add_executable(cassandra_value
    cassandra_value.cpp
)

target_link_libraries(cassandra_value
    QtCassandra
    Qt5::Core
    Qt5::Xml
)

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

################################################################################
project(context_management)

add_executable(context_management
    context_management.cpp
)

target_link_libraries(context_management
    QtCassandra
    Qt5::Core
    Qt5::Xml
)

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

################################################################################
project(read_write_data)

add_executable(read_write_data
    read_write_data.cpp
)

target_link_libraries(read_write_data
    QtCassandra
    Qt5::Core
    Qt5::Xml
)

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

################################################################################
#project(read_uuid)
#
#add_executable(read_uuid
#    read_uuid.cpp
#)
#
#target_link_libraries(read_uuid
#    QtCassandra
#    Qt5::Core
#    Qt5::Xml
#)

################################################################################
project(million_rows)

add_executable(million_rows
    million_rows.cpp
)

target_link_libraries(million_rows
    QtCassandra
    Qt5::Core
    Qt5::Xml
)

option( RUN_MILLION_ROW_TEST "In addition to the normal tests, include the (very slow) 'million row' test." OFF )
if( ${RUN_MILLION_ROW_TEST} )
    add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )
endif()

################################################################################
# This test has been deprecated because the "counter" column for "column1" is
# no longer supported.
project(simple_counter)

add_executable(simple_counter
    simple_counter.cpp
)

target_link_libraries(simple_counter
    QtCassandra
    Qt5::Core
    Qt5::Xml
)

#add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

################################################################################
#project(composite_type)
#
#add_executable(composite_type
#    composite_type.cpp
#)
#
#target_link_libraries(composite_type
#    QtCassandra
#    Qt5::Core
#    Qt5::Xml
#)
#
#add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

# vim: ts=4 sw=4 et
