#
# File:
#      plugins/layout/CMakeLists.txt
#
# Description:
#      Generate a page content. This includes many different type of data such
#      as a regular page or a tag part of a taxonomy.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2012-2014 Made to Order Software Corp.
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

project(plugin_layout)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${QT_INCLUDES}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    layout.qrc
)

snap_validate_xml( layout/content.xml content.dtd )

add_library(layout SHARED
    layout.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    layout/functions.xsl
    layout/html-header.xsl
    layout/user-messages.xsl
    layout/default-body-parser.xsl
    layout/default-theme-parser.xsl
)

target_link_libraries( layout ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( layout make_controlled_vars_headers )
endif()

install(
    TARGETS layout LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
FILE(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES ${HEADER_FILES}
    DESTINATION include/snapwebsites/plugins/layout
)

# vim: ts=4 sw=4 et
