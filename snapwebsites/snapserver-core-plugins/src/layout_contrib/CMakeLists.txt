#
# File:
#      plugins/layout_contrib/CMakeLists.txt
#
# Description:
#      Various CSS, JS, Fonts, etc. to help build modern layouts.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2017  Made to Order Software Corp.
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

project(plugin_layout_contrib)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    layout_contrib.qrc
)


## Validate content
##
snap_validate_xml( content.xml content/content.dtd )

snap_validate_css( css/agency.css       )
snap_validate_css( css/bootstrap.css    )
snap_validate_css( css/bootstrap-social.css )
snap_validate_css( css/bootstrap-theme.css  )
snap_validate_css( css/font-awesome.css )
snap_validate_css( css/metis-menu.css    )

snap_validate_js( js/agency.js    )
#snap_validate_js( js/bootstrap.js ) -- apparently the author does not believe in having ';' at the end of lines
snap_validate_js( js/metis-menu.js )



add_library(layout_contrib SHARED
    layout_contrib.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    css/agency.css
    css/bootstrap.css
    css/bootstrap-social.css
    css/bootstrap-theme.css
    css/font-awesome.css
    css/metis-menu.css
    js/agency.js
    js/bootstrap.js
    js/metis-menu.js
)

target_link_libraries( layout_contrib Qt5::Core )

install(
    TARGETS
        layout_contrib
    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION
        include/snapwebsites/plugins/layout_contrib
    FILES_MATCHING PATTERN
        "*.h"
)


# vim: ts=4 sw=4 et wrap
