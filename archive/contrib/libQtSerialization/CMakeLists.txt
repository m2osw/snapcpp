#
# Text:
#      CMakeLists.txt
#
# Description:
#      Top level project definitions which includes the C++ files and the
#      header files as required to compile the libQtSerialization and tests
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
cmake_minimum_required(VERSION 3.10.2)

project(libQtSerialization)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

find_package( SnapCMakeModules REQUIRED )
find_package( SnapDev          REQUIRED )
find_package( LibExcept        REQUIRED )
find_package( Qt5Core          REQUIRED )
find_package( SnapDoxygen               )

SnapGetVersion( LIBQTSERIALIZATION ${CMAKE_CURRENT_SOURCE_DIR} )

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${SNAPDEV_INCLUDE_DIR}
    ${Qt5Core_INCLUDE_DIRS}
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/QtSerialization/QSerialization.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/QtSerialization/QSerialization.h
)

add_subdirectory(cmake)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)


# vim: ts=4 sw=4 et
