#
# File:
#      plugins/info/CMakeLists.txt
#
# Description:
#      Basic Information about the website. This module lets administrators
#      edit global information about their website. It is separate from the
#      content plugin because it requires form, permissions, etc. which the
#      content plugin cannot directly access.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2014 Made to Order Software Corp.
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

project(plugin_info)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    SYSTEM
        ${QT_INCLUDES}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    info.qrc
)

snap_validate_xml( content.xml content.dtd )
snap_validate_xml( settings-form.xml form/snap-form.dtd )

add_library(info SHARED
    info.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    settings-form.xml
)

target_link_libraries( info ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( info make_controlled_vars_headers )
endif()

install(
    TARGETS info LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/info
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
