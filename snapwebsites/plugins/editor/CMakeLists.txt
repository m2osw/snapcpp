#
# File:
#      plugins/editor/CMakeLists.txt
#
# Description:
#      Implementation of a full WYSIWYG editor for our Snap! websites.
#      This is similar to existing editors except that it let you edit
#      your pages right there, inline, with the right fonts, right,
#      effects, etc.
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

project(plugin_editor)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${LIBTLD_INCLUDE_DIRS}
    ${QT_INCLUDES}
)


QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    editor.qrc
)

snap_validate_xml( content.xml content.dtd )
snap_validate_xml( new-draft.xml form/snap-form.dtd )


add_library(editor SHARED
    editor.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    new-draft.xml
)

target_link_libraries( editor ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( editor make_controlled_vars_headers )
endif()

install(
    TARGETS editor LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
FILE(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
    FILES ${HEADER_FILES}
    DESTINATION include/snapwebsites/plugins/editor
)

# vim: ts=4 sw=4 et
