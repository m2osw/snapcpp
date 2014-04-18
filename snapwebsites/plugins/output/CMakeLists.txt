#
# File:
#      plugins/output/CMakeLists.txt
#
# Description:
#      Generate the output of a page. This plugin takes what the content
#      plugin manages and transforms it in HTML or some other format.
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

project(plugin_output)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${QT_INCLUDES}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    output.qrc
)

snap_validate_xml( content.xml content.dtd )

# TODO: do these need to pass the linter too?
#snap_validate_js( jquery.js                    )
#snap_validate_js( jquery-extensions.js         )
#snap_validate_js( externs/jquery-extensions.js )
#
snap_validate_js( server-access.js             )
snap_validate_js( popup.js                     )
snap_validate_js( output.js                    )

add_library(output SHARED
    output.cpp
    output.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    output.css
    close-button.png
    jquery.js
    jquery-extensions.js
    output.js
    popup.js
    server-access.js
    layout/notheme-body-parser.xsl
    layout/notheme-content.xml
    layout/notheme-theme-parser.xsl
)

target_link_libraries( output ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( output make_controlled_vars_headers )
endif()

install(
    TARGETS output LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/output
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
