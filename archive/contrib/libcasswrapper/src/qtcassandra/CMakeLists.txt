#
# Text:
#      src/CMakeLists.txt
#
# Description:
#      Defines the source files.
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

project( casswrapper )

include_directories(
    ${casswrapper_SOURCE_DIR}
)


project( qtcassandra )

set( HEADER_FILES
    QCassandraDriver.h
    QCassandraResult.h
)

add_library( ${PROJECT_NAME} SHARED
    ${HEADER_FILES}
    QCassandraDriver.cpp
    QCassandraResult.cpp
    cassandra.json
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    casswrapper
    Qt5::Core
    Qt5::Xml
    Qt5::Sql
)

execute_process( COMMAND uname -m OUTPUT_VARIABLE MACHINE_TYPE )
string(STRIP ${MACHINE_TYPE} STRIPPED_MT ) # remove trailing \n
if( "${STRIPPED_MT}" STREQUAL "x86_64" )
    set( LIBPREFIX "${STRIPPED_MT}-linux-gnu/" )
endif()
install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/${LIBPREFIX}qt5/plugins/sqldrivers
)

#install(
#    FILES ${HEADER_FILES}
#    DESTINATION include/casswrapper
#)

# vim: ts=4 sw=4 et
