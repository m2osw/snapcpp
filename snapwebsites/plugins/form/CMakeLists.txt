#
# File:
#      plugins/form/CMakeLists.txt
#
# Description:
#      Plugin used to handle the generate HTML forms from XML data.
#      This plugin runs an XSLT against the form XML data to generate
#      the output HTML which generally is then taken as the body output
#      in the current page.
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

project(plugin_form)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${QT_INCLUDES}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    form.qrc
)

snap_validate_xml( content.xml content.dtd )
snap_validate_js ( form.js )

# Unfortunate, but at this time this doesn't work too well...
# (i.e. xmllint [as in libxml2] does not support this complex XSD schema)
#snap_validate_xml( core-form.xsl schema-for-xslt20.xsd )

add_library(form SHARED
    form.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    core-form.xsl
    form.js
)

target_link_libraries( form ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( form make_controlled_vars_headers )
endif()

install(
    TARGETS form LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/form
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
