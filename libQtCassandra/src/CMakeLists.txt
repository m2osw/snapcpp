#
# Text:
#      CMakeLists.txt
#
# Description:
#      Defines the source files.
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

project( QtCassandra )

include_directories( ${CASSANDRACPPDRIVER_INCLUDE_DIR} )

add_library(${PROJECT_NAME} SHARED
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandra.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraCell.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraColumnPredicate.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraContext.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraLock.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraRow.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraRowPredicate.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraTable.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraTools.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraValue.h
    ${libQtCassandra_includes_SOURCE_DIR}/QtCassandra/QCassandraVersion.h.in
    ${libQtCassandra_includes_BINARY_DIR}/QtCassandra/QCassandraVersion.h
    QCassandra.cpp
    QCassandraCell.cpp
    QCassandraContext.cpp
    QCassandraColumnPredicate.cpp
    QCassandraLock.cpp
    QCassandraRow.cpp
    QCassandraRowPredicate.cpp
    QCassandraTable.cpp
    QCassandraTools.cpp
    QCassandraValue.cpp
)

target_link_libraries(QtCassandra
    ${CASSANDRACPPDRIVER_LIBRARIES}
    ${QT_LIBRARIES}
)

set_target_properties( QtCassandra PROPERTIES
    VERSION ${LIBQTCASSANDRA_VERSION_MAJOR}.${LIBQTCASSANDRA_VERSION_MINOR}
    SOVERSION ${LIBQTCASSANDRA_VERSION_MAJOR}
)

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( ${PROJECT_NAME} controlled_vars.h )
endif()

install(
    TARGETS QtCassandra LIBRARY DESTINATION lib
)

# vim: ts=4 sw=4 et
