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

project( cassvalue )

configure_file(
		cassvalue/version.h.in
		${PROJECT_BINARY_DIR}/cassvalue/version.h
		)

set( HEADER_FILES
    cassvalue/encoder.h
    cassvalue/value.h
    ${PROJECT_BINARY_DIR}/cassvalue/version.h
)

add_library(${PROJECT_NAME} SHARED
    ${HEADER_FILES}
    cassvalue/version.h.in
    value.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${CASSVALUE_VERSION_MAJOR}.${CASSVALUE_VERSION_MINOR}
    SOVERSION ${CASSVALUE_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)

install(
	FILES ${HEADER_FILES}
	DESTINATION include/cassvalue
)

# vim: ts=4 sw=4 et
