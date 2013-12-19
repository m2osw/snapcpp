#
# File:
#      plugins/links/CMakeLists.txt
#
# Description:
#      Plugin used to manage all the types, categories, terms, tags, etc.
#      This heavily makes use of the links to connect everything together.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2012-2013 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

project(plugin_taxonomy)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore QtXml )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${QT_INCLUDES}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    taxonomy.qrc
)

set( XMLFILES content.xml )
snap_validate_dtd( ${PROJECT_NAME} ${XMLFILES} )

add_library(taxonomy SHARED
    taxonomy.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    ${XMLFILES}
)

target_link_libraries( taxonomy ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( taxonomy make_controlled_vars_headers )
endif()

install(
    TARGETS taxonomy LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)

# vim: ts=4 sw=4 et
