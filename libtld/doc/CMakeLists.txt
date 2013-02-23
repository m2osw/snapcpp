#
# File:
#      CMakeLists.txt
#
# Description:
#      Create the documentation of the libtld library.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2013 Made to Order Software Corp.
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


##
## Documentation
##
include( doxygen )

AddDoxygenTarget( libtld
    ${LIBTLD_VERSION_MAJOR}
    ${LIBTLD_VERSION_MINOR}
    ${LIBTLD_VERSION_PATCH}
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libtld-doc-${LIBTLD_VERSION_MAJOR}.${LIBTLD_VERSION_MINOR}/.
    DESTINATION share/doc/libtld/html
    COMPONENT doc
)
install(
    FILES ${tld_library_SOURCE_DIR}/LICENSE.txt
    RENAME copyright
    DESTINATION share/doc/libtld
    COMPONENT runtime
)
install(
    FILES debian/changelog.Debian.gz
    DESTINATION share/doc/libtld
    COMPONENT runtime
)

##
## A Simple Working Example
##
project(example)
include_directories(
    "${PROJECT_BINARY_DIR}/../include"
    "${PROJECT_SOURCE_DIR}/../include"
)
add_executable(example
    example.c
)
target_link_libraries(example
    tld
)

# vim: ts=4 sw=4 et
