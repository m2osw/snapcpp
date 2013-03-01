#
# File:
#      plugins/filter/CMakeLists.txt
#
# Description:
#      Plug-in used to filter XML data before sending it to a client. This
#      ensures that the data does not contain any XSS code.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2013 Made to Order Software Corp.
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

project(plugin_filter)

SnapIncludeQt4( QtCore QtXml )
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    filter.qrc
)

add_library(filter SHARED
    filter.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}

    content.xml
)

target_link_libraries( filter ${QT_LIBRARIES} )

install(
    TARGETS filter LIBRARY DESTINATION lib/snapwebsites/plugins
)

# vim: ts=4 sw=4 et
