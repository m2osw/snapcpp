#
# File:
#      plugins/robotstxt/CMakeLists.txt
#
# Description:
#      Generate a robots.txt file so robots know what they can and cannot
#      access on your website.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2012 Made to Order Software Corp.
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

project(plugin_robotstxt)

find_package( Qt4 4.8.1 QUIET REQUIRED QtCore )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${QT_INCLUDES}
)

# This was an attempt to pre-process the .xml file to remove comments
# but at this point it doesn't work right (i.e. the target cannot always
# be found / regenerated).
#add_custom_command(OUTPUT content.xml
#    # The output should be ${MAKE_CURRENT_BINARY_DIR} but unfortunately rcc
#    # cannot be tweak to read files from two different folders (TBD)
#    COMMAND echo \"<?xml version=\\"1.0\\"?>\" >${CMAKE_CURRENT_SOURCE_DIR}/content.xml
#    COMMAND xmlpatterns ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/plugins.xsl ${CMAKE_CURRENT_SOURCE_DIR}/robotstxt.xml >>${CMAKE_CURRENT_SOURCE_DIR}/content.xml
#    MAIN_DEPENDENCY robotstxt.xml
#)
#
#set_directory_properties(PROPERTIES
#    ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/content.xml
#)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    robotstxt.qrc
)

add_library(robotstxt SHARED
    robotstxt.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
)

target_link_libraries( robotstxt ${QT_LIBRARIES} )

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
    add_dependencies( robotstxt make_controlled_vars_headers )
endif()

install(
    TARGETS robotstxt LIBRARY DESTINATION lib/snapwebsites/plugins
)

# vim: ts=4 sw=4 et
