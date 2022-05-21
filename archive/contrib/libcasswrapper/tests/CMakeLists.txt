#
# Text:
#      tests/CMakeLists.txt
#
# Description:
#      Compile and run libcasswrapper tests.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
#
#      https://snapwebsites.org/
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
    ${casswrapper_SOURCE_DIR}
    ${casswrapper_BINARY_DIR}
)

################################################################################
project(libquerytest)

add_library( ${PROJECT_NAME} STATIC
    query_test.h
    query_test.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${AS2JS_LIBRARIES}
    ${CASSANDRACPPDRIVER_LIBRARIES}
    qtcassandra
    casswrapper
)

################################################################################
# Change the IP as required by your environment
set( DEFAULT_CASSANDRA_IP "$ENV{CASSANDRA_IP}" )
if( NOT DEFAULT_CASSANDRA_IP )
    set( DEFAULT_CASSANDRA_IP "localhost" )
endif()
set( CASSANDRA_IP "${DEFAULT_CASSANDRA_IP}" CACHE STRING "IP address of the cassandra server" )

################################################################################
project(query_tests)

set( SQLDRIVERS_DIR ${PROJECT_BINARY_DIR}/sqldrivers )
add_custom_command( OUTPUT ${SQLDRIVERS_DIR}/libqtcassandra.so
    COMMAND cmake -E make_directory ${SQLDRIVERS_DIR}
    COMMAND cmake -E copy ${qtcassandra_BINARY_DIR}/libqtcassandra.so ${SQLDRIVERS_DIR}
    DEPENDS qtcassandra
)

#create_test_sourcelist(
#    SRCLIST ${PROJECT_NAME}.cpp describeSchema.cpp
#    FUNCTION tests_main
#    EXTRA_INCLUDE tests_main.h
#)
add_executable( ${PROJECT_NAME}
    ${SQLDRIVERS_DIR}/libqtcassandra.so
    query_test.h
    query_test.cpp
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    libquerytest
)

add_test( NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} -h ${CASSANDRA_IP} )

# vim: ts=4 sw=4 et
