#
# Text:
#      CMakeLists.txt
#
# Description:
#      Compile and run libQtCassandra tests.
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
project(cluster)


add_executable(cluster
    cluster.cpp
)

target_link_libraries(cluster
    QtCassandra
)


################################################################################
project(cassandra_value)

add_executable(cassandra_value
    cassandra_value.cpp
)

target_link_libraries(cassandra_value
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
project(context_management)

add_executable(context_management
    context_management.cpp
)

target_link_libraries(context_management
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
project(read_write_data)

add_executable(read_write_data
    read_write_data.cpp
)

target_link_libraries(read_write_data
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
#project(read_uuid)
#
#add_executable(read_uuid
#    read_uuid.cpp
#)
#
#target_link_libraries(read_uuid
#    QtCassandra
#    ${QT_LIBRARIES}
#    ${THRIFT_LIBRARIES}
#)

################################################################################
project(million_rows)

add_executable(million_rows
    million_rows.cpp
)

target_link_libraries(million_rows
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
project(simple_counter)

add_executable(simple_counter
    simple_counter.cpp
)

target_link_libraries(simple_counter
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
project(composite_type)

add_executable(composite_type
    composite_type.cpp
)

target_link_libraries(composite_type
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
project(cassandra_lock)

add_executable(cassandra_lock
    cassandra_lock.cpp
)

target_link_libraries(cassandra_lock
    QtCassandra
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
)

################################################################################
project(run_cassandra_tests)

# Change the IP as required by your environment
set( DEFAULT_CASSANDRA_IP "$ENV{CASSANDRA_IP}" )
if( NOT DEFAULT_CASSANDRA_IP )
    set( DEFAULT_CASSANDRA_IP "localhost" )
endif()
set( CASSANDRA_IP "${DEFAULT_CASSANDRA_IP}" CACHE STRING "IP address of the cassandra server" )

add_custom_target(run_cassandra_tests
    COMMAND echo "========================================="
    COMMAND echo "cluster -h ${CASSANDRA_IP}"
    COMMAND cluster -h ${CASSANDRA_IP}
    COMMAND echo "========================================="
    COMMAND echo "cassandra_value -h ${CASSANDRA_IP}"
    COMMAND cassandra_value -h ${CASSANDRA_IP}
    COMMAND echo "========================================="
    COMMAND echo "context_management -h ${CASSANDRA_IP}"
    COMMAND context_management -h ${CASSANDRA_IP}
    COMMAND echo "========================================="
    COMMAND echo "read_write_data -h ${CASSANDRA_IP}"
    COMMAND read_write_data -h ${CASSANDRA_IP}
    # This test is actually not working, this is a Cassandra
    # limitation; (i.e. flooding a server is not supported)
    # http://www.datastax.com/dev/blog/bulk-loading
    #COMMAND echo "========================================="
    #COMMAND echo "million_rows -h ${CASSANDRA_IP}"
    #COMMAND million_rows -h ${CASSANDRA_IP}
    COMMAND echo "========================================="
    COMMAND echo "simple_counter -h ${CASSANDRA_IP}"
    COMMAND simple_counter -h ${CASSANDRA_IP}
    COMMAND echo "========================================="
    COMMAND echo "composite_type -h ${CASSANDRA_IP}"
    COMMAND composite_type -h ${CASSANDRA_IP}
    COMMAND echo "========================================="
    COMMAND echo "cassandra_lock -h ${CASSANDRA_IP}"
    COMMAND cassandra_lock -h ${CASSANDRA_IP} -i 4 -n 5
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
        cluster
        cassandra_value
        context_management
        read_write_data
        million_rows
        simple_counter
        composite_type
        cassandra_lock
)

# vim: ts=4 sw=4 et
