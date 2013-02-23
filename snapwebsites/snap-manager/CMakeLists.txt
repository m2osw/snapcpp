#
# File:
#      snap-manager/CMakeLists.txt
#
# Description:
#      The Snap! Manager to edit data in the Cassandra cluster
#      using a simple Qt graphical interface.
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
## The Snap! Manager
##


SnapIncludeQt4(
    QtCore
    QtNetwork
    QtGui
    QtWebKit
)

project(snap-manager)

# We remove the -Wswitch-default because qt has some problematic headers in that regard
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

QT4_WRAP_UI(SNAP_MANAGER_UIS_H
    snap-manager-mainwindow.ui
    snap-manager-aboutbox.ui
    snap-manager-helpbox.ui
    snap-manager-decode-utf8-dialog.ui
    snap-manager-connectform.ui
    snap-manager-domainform.ui
)
QT4_WRAP_CPP(SNAP_MANAGER_MOC_CXX
    snap-manager.h
    snap-manager-about.h
    snap-manager-help.h
    snap-manager-decode-utf8.h
)
QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    snap-manager-resources.qrc
)


include_directories(
    #"${CMAKE_CURRENT_SOURCE_DIR}/../lib"
    #"${CMAKE_CURRENT_BINARY_DIR}/../lib"
    #"${CMAKE_CURRENT_BINARY_DIR}/../snap-manager"
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
    "${PROJECT_BINARY_DIR}/../../libtld/include"
)


add_executable(snap-manager
    snap-manager.cpp
    snap-manager-about.cpp
    snap-manager-help.cpp
    snap-manager-decode-utf8.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_UIS_H}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries( ${PROJECT_NAME}
    libsnapwebsites
    QtCassandra
    ${QT_LIBRARIES}
    thrift
    tld
    dl
)

install(
    TARGETS snap-manager RUNTIME DESTINATION bin
)

# vim: ts=4 sw=4 et
