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
#      Copyright (c) 2012-2013 Made to Order Software Corp.
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
cmake_minimum_required(VERSION 2.8)

set(LIBQTSERIALIZATION_VERSION_MAJOR 0)
set(LIBQTSERIALIZATION_VERSION_MINOR 1)
set(LIBQTSERIALIZATION_VERSION_PATCH 0)

project(libQtSerialization)

## Include support modules
##
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules/Modules )
    message( "Found snapCMakeModules in ${CMAKE_MODULE_PATH}" )
endif()

find_package( SnapCMakeModules )
find_package( SnapDoxygen      )
find_package( Thrift           )
find_package( Qt4 4.8.1 REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${THRIFT_INCLUDE_DIRS}
    ${QT_INCLUDES}
)

# Create a configure script
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/QtSerialization/QSerialization.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/QtSerialization/QSerialization.h)

install( FILES LICENSE.txt DESTINATION share/doc/libqtserialization )

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)


set(CPACK_PACKAGE_NAME "libQtSerialization")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libQtSerialization -- a Qt extension to read and write data in a forward and backward compatible manner")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "alexis@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${libQtSerialization_SOURCE_DIR}/LICENSE.txt")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
set(CPACK_PACKAGE_VERSION "${LIBQTSERIALIZATION_VERSION_MAJOR}.${LIBQTSERIALIZATION_VERSION_MINOR}.${LIBQTSERIALIZATION_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${LIBQTSERIALIZATION_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${LIBQTSERIALIZATION_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${LIBQTSERIALIZATION_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "libQtSerialization-${LIBQTSERIALIZATION_VERSION_MAJOR}.${LIBQTSERIALIZATION_VERSION_MINOR}.${LIBQTSERIALIZATION_VERSION_PATCH}")
set(CPACK_PACKAGE_EXECUTABLES "libQtSerialization.so")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/libqtserialization")
set(CPACK_DEBIAN_PACKAGE_ENHANCES "libqtcore4")
# I'm not too sure how we can add this one... components?
# See http://www.vtk.org/Wiki/CMake:Component_Install_With_CPack
#set(CPACK_DEBIAN_PACKAGE_DEV_DEPENDENCIES "cmake libqtcore4-dev")
include(CPack)

# vim: ts=4 sw=4 et
