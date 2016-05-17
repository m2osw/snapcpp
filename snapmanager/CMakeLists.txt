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
#      Copyright (c) 2011-2016 Made to Order Software Corp.
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
cmake_minimum_required(VERSION 2.8)

##
## The Snap! Manager
##
project( snapmanager )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

##
## Version
##
set(SNAPMANAGER_VERSION_MAJOR 1)
set(SNAPMANAGER_VERSION_MINOR 1)
set(SNAPMANAGER_VERSION_PATCH 0)
add_definitions( -DSNAPMANAGER_VERSION=\"${SNAPMANAGER_VERSION_MAJOR}.${SNAPMANAGER_VERSION_MINOR}.${SNAPMANAGER_VERSION_PATCH}\" )

##
## CMake module path
##
if( NOT CMAKE_PREFIX_PATH )
    set( CMAKE_PREFIX_PATH "/usr" )
endif()

set( CMAKE_MODULE_PATH
    ${CMAKE_PREFIX_PATH}/share/cmake/Modules
    ${CMAKE_MODULE_PATH}
)

##
## Find required packages
##
find_package( AdvGetOpt        REQUIRED )
find_package( LibTLD           REQUIRED )
find_package( log4cplus        REQUIRED )
find_package( Qt5Core          REQUIRED )
find_package( Qt5Network       REQUIRED )
find_package( Qt5Widgets       REQUIRED )
find_package( Qt5WebKit        REQUIRED )
find_package( Qt5WebKitWidgets REQUIRED )
find_package( Qt5Xml           REQUIRED )
find_package( QtCassandra      REQUIRED )
find_package( QtSerialization  REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapWebsites     REQUIRED )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5WebKit_INCLUDE_DIRS}
    ${Qt5WebKitWidgets_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
)

set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Widgets
    Qt5::WebKit
    Qt5::WebKitWidgets
    Qt5::Xml
)

# We remove the -Wswitch-default because qt has some problematic headers in that regard
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

qt5_wrap_ui(SNAP_MANAGER_UIS_H
    snap-manager-mainwindow.ui
    snap-manager-aboutbox.ui
    snap-manager-createcontextbox.ui
    snap-manager-helpbox.ui
    snap-manager-initialize-websitebox.ui
    snap-manager-decode-utf8-dialog.ui
    snap-manager-connectform.ui
    snap-manager-domainform.ui
)
qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    snap-manager-resources.qrc
)


include_directories(
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBTLD_INCLUDE_DIRS}
    ${LOG4CPLUS_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${SNAPWEBSITES_INCLUDE_DIRS}
)

add_executable( ${PROJECT_NAME}
    DomainModel.cpp
    RowModel.cpp
    TableModel.cpp
    WebsiteModel.cpp
    snap-manager.cpp
    snap-manager-about.cpp
    snap-manager-createcontext.cpp
    snap-manager-help.cpp
    snap-manager-initialize-website.cpp
    snap-manager-decode-utf8.cpp
    get_child.h
    ${SNAP_MANAGER_UIS_H}
    ${SNAP_MANAGER_RESOURCES_RCC}
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries( ${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${QT_LIBRARIES}
    ${QTCASSANDRA_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${LIBTLD_LIBRARIES}
    ${SNAPWEBSITES_LIBRARIES}
    dl
)

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin )
install( FILES snapmanager.desktop  DESTINATION share/applications )
install( FILES snap-medium-icon.png DESTINATION share/pixmaps/ RENAME snapmanager.png )

## TODO: add code coverage, doxygen and unittests

# vim: ts=4 sw=4 et nocindent
