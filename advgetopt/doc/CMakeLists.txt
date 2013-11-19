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
project(documentation)

find_package( SnapDoxygen )

set( PROJECT_ROOT_PATH ${CMAKE_SOURCE_DIR} )

AddDoxygenTarget( advgetopt
    ${LIBTLD_VERSION_MAJOR}
    ${LIBTLD_VERSION_MINOR}
    ${LIBTLD_VERSION_PATCH}
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/advgetopt-doc-${LIBTLD_VERSION_MAJOR}.${LIBTLD_VERSION_MINOR}/.
    DESTINATION share/doc/advgetopt/html
    COMPONENT documentation
)
install(
    FILES ${tld_library_SOURCE_DIR}/LICENSE.txt
    DESTINATION share/doc/advgetopt
    COMPONENT runtime
)

# vim: ts=4 sw=4 et
