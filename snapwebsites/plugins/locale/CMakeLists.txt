#
# File:
#      plugins/locale/CMakeLists.txt
#
# Description:
#      Offer support for various locale function used to format strings
#      depending on a locale selection. The locale also defines the
#      timezone that a user may select.
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

project(plugin_locale)

find_package( ICU REQUIRED )
find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    SYSTEM
        ${QT_INCLUDES}
        ${ICU_INCLUDE_DIRS}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    locale.qrc
)

snap_validate_xml( content.xml content.dtd )

snap_validate_xml( locale-form.xsl ../layout/simplified-xslt20.dtd )



# We use the name snap_locale.h for the header because there is a locale.h
# file in the OS under Linux

add_library(locale SHARED
    locale.cpp
    snap_locale.h
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    locale-form.xsl
    locale-timezone.js
    locale-timezone.css
    locale-timezone-page.xml
    locale-timezone-parser.xsl
)

target_link_libraries( locale
    ${QT_LIBRARIES}
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
)

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( locale make_controlled_vars_headers )
endif()

install(
    TARGETS locale LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/locale
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
