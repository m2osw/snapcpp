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
    ${AS2JS_INCLUDE_DIR}
    ${CASSANDRACPPDRIVER_INCLUDE_DIR}
    ${PROJECT_BINARY_DIR}
)

configure_file(
		version.h.in
		${PROJECT_BINARY_DIR}/casswrapper/version.h
		)

set( HEADER_FILES
    batch.h
    exception.h
    query.h
    query_model.h
    schema.h
    schema_value.h
    session.h
    qstring_stream.h
    ${PROJECT_BINARY_DIR}/casswrapper/version.h
)

add_library( ${PROJECT_NAME} SHARED
    ${HEADER_FILES}
    version.h.in
    batch.cpp
    casswrapper_impl.h
    casswrapper_impl.cpp
    exception_impl.h
    exception_impl.cpp
    not_used.h
    query.cpp
    query_model.cpp
    schema.cpp
    schema_value.cpp
    session.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${AS2JS_LIBRARIES}
    ${CASSANDRACPPDRIVER_LIBRARIES}
    ${CASSVALUE_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    Qt5::Core
    Qt5::Xml
    Qt5::Sql
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${LIBCASSWRAPPER_VERSION_MAJOR}.${LIBCASSWRAPPER_VERSION_MINOR}
    SOVERSION ${LIBCASSWRAPPER_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)

install(
	FILES ${HEADER_FILES}
	DESTINATION include/casswrapper
)

# vim: ts=4 sw=4 et
