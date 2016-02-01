#
# File:
#      plugins/date_widgets/CMakeLists.txt
#
# Description:
#      Date widget extensions to the Snap! editor.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013-2016 Made to Order Software Corp.
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

project(plugin_date_widgets)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    SYSTEM
        ${QT_INCLUDES}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBTLD_INCLUDE_DIRS}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
)


QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    date_widgets.qrc
)

snap_validate_xml( content.xml                      content/content.dtd )

snap_validate_xml( date_widgets/date-form.xsl       layout/simplified-xslt20.dtd )

snap_validate_js( date_widgets/date-widgets.js )


add_library(date_widgets SHARED
    date_widgets.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    date_widgets/date-widgets.js
    date_widgets/date-widgets.css
    date_widgets/date-widgets-logo-64x64.png
    date_widgets/date-form.xsl
)

target_link_libraries( date_widgets ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( date_widgets make_controlled_vars_headers )
endif()

install(
    TARGETS date_widgets LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES ${HEADER_FILES}
    DESTINATION include/snapwebsites/plugins/date_widgets
)


install(
    FILES
        date_widgets/date-widgets.js
    DESTINATION
        share/javascript/snapwebsites/date_widgets
)

# vim: ts=4 sw=4 et
