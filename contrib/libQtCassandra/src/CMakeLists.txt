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

include_directories(
    ${AS2JS_INCLUDE_DIR}
    ${CASSANDRACPPDRIVER_INCLUDE_DIR}
)

configure_file(
		QtCassandra/QCassandraVersion.h.in
		${PROJECT_BINARY_DIR}/QtCassandra/QCassandraVersion.h
		)

set( HEADER_FILES
    QtCassandra/QCassandra.h
    QtCassandra/QCassandraCell.h
    QtCassandra/QCassandraConsistencyLevel.h
    QtCassandra/QCassandraContext.h
    QtCassandra/QCassandraOrder.h
    QtCassandra/QCassandraOrderResult.h
    QtCassandra/QCassandraPredicate.h
    QtCassandra/QCassandraProxy.h
    QtCassandra/QCassandraQuery.h
    QtCassandra/QCassandraRow.h
    QtCassandra/QCassandraSchema.h
    QtCassandra/QCassandraSchemaValue.h
    QtCassandra/QCassandraSession.h
    QtCassandra/QCassandraTable.h
    QtCassandra/QCassandraValue.h
    QtCassandra/QCassandraVersion.h.in
    QtCassandra/QStringStream.h
    QtCassandra/QueryModel.h
    ${PROJECT_BINARY_DIR}/QtCassandra/QCassandraVersion.h
)

add_library(${PROJECT_NAME} SHARED
    ${HEADER_FILES}
    NotUsed.h
    CassTools.h
    CassTools.cpp
    QCassandra.cpp
    QCassandraCell.cpp
    QCassandraContext.cpp
    QCassandraOrder.cpp
    QCassandraOrderResult.cpp
    QCassandraPredicate.cpp
    QCassandraProxy.cpp
    QCassandraQuery.cpp
    QCassandraRow.cpp
    QCassandraSchema.cpp
    QCassandraSchemaValue.cpp
    QCassandraSession.cpp
    QCassandraTable.cpp
    QCassandraValue.cpp
    QueryModel.cpp
)

target_link_libraries(QtCassandra
    ${AS2JS_LIBRARIES}
    ${CASSANDRACPPDRIVER_LIBRARIES}
    Qt5::Core
    Qt5::Xml
)

set_target_properties( QtCassandra PROPERTIES
    VERSION ${LIBQTCASSANDRA_VERSION_MAJOR}.${LIBQTCASSANDRA_VERSION_MINOR}
    SOVERSION ${LIBQTCASSANDRA_VERSION_MAJOR}
)

install(
    TARGETS QtCassandra LIBRARY DESTINATION lib
)

install(
	FILES ${HEADER_FILES}
	DESTINATION include/QtCassandra
)

install(
	FILES ${PROJECT_BINARY_DIR}/QtCassandra/QCassandraVersion.h
	DESTINATION include/QtCassandra/
)

# vim: ts=4 sw=4 et
