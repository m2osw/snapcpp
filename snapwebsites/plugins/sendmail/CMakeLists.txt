#
# File:
#      plugins/sendmail/CMakeLists.txt
#
# Description:
#      Send emails to specified addresses using the system sendmail command
#      line. This plugin makes use of a backend process to actually process
#      the emails on a mail server. The front end saves the email in your
#      Cassandra database.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2013 Made to Order Software Corp.
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

project(plugin_sendmail)

SnapIncludeQt4( QtCore )
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${QT_INCLUDES}
    "${PROJECT_BINARY_DIR}/../../../libtld/include"
)

QT4_ADD_RESOURCES(SNAP_MANAGER_RESOURCES_RCC
    sendmail.qrc
)

add_library(sendmail SHARED
    sendmail.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}

    content.xml
)

target_link_libraries( sendmail ${QT_LIBRARIES} )

install(
    TARGETS sendmail LIBRARY DESTINATION lib/snapwebsites/plugins
)

# vim: ts=4 sw=4 et
