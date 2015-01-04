#
# File:
#      plugins/hashtag/CMakeLists.txt
#
# Description:
#      Transform words preceeded by a hash into tags.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2015 Made to Order Software Corp.
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

project(plugin_hashtag)

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
    hashtag.qrc
)


snap_validate_xml( content.xml content/content.dtd )

snap_validate_xml( hashtag-settings-page.xml    editor/editor-form.dtd )
snap_validate_xml( hashtag-settings-parser.xsl  layout/simplified-xslt20.dtd )


add_library(hashtag SHARED
    hashtag.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    hashtag.css
    hashtag-settings-page.xml
    hashtag-settings-parser.xsl
)

target_link_libraries( hashtag ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( hashtag make_controlled_vars_headers )
endif()

install(
    TARGETS hashtag LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/hashtag
    FILES_MATCHING PATTERN "*.h"
)


# vim: ts=4 sw=4 et
