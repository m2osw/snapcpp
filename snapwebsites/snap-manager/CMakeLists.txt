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
#      Copyright (c) 2011-2014 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#     
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#     
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#


##
## The Snap! Manager
##

find_package( Qt4 4.8.1 QUIET REQUIRED 
    QtCore
    QtNetwork
    QtGui
    QtWebKit
)
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

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
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBTLD_INCLUDE_DIRS}
    ${QT_INCLUDES}
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
    ${QTCASSANDRA_LIBRARIES}
    ${QT_LIBRARIES}
    ${THRIFT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    dl
)

install( TARGETS snap-manager RUNTIME DESTINATION bin )
install( FILES snapmanager.desktop  DESTINATION share/applications )
install( FILES snap-medium-icon.png DESTINATION share/pixmaps/ RENAME snapmanager.png )

# vim: ts=4 sw=4 et nocindent
