#
# Text:
#      CMakeLists.txt
#
# Description:
#      Top level project definitions which includes the C++ files and the
#      header files as required to compile the CassandraBrowse project
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2015 Made to Order Software Corp.
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

cmake_minimum_required(VERSION 3.5)

project(cassview-project)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

## Include support modules
##
if( NOT CMAKE_PREFIX_PATH )
    set( CMAKE_PREFIX_PATH "/usr" )
endif()

set( CMAKE_MODULE_PATH
    ${CMAKE_PREFIX_PATH}/share/cmake/Modules
    ${CMAKE_MODULE_PATH}
)

find_package( SnapCMakeModules REQUIRED )
find_package( AdvGetOpt        REQUIRED )
find_package( CassWrapper      REQUIRED )
find_package( LibExcept        REQUIRED )
find_package( log4cplus        REQUIRED )
find_package( LibTLD           REQUIRED )
find_package( Qt5Core   	   REQUIRED )
find_package( Qt5Network 	   REQUIRED )
find_package( Qt5Xml		   REQUIRED )
find_package( Qt5Widgets 	   REQUIRED )
find_package( QtSerialization  REQUIRED )
find_package( SnapWebsites     REQUIRED )
#find_package( SnapDoxygen )

##
## Version
##
SnapGetVersion( CASSVIEW ${CMAKE_CURRENT_SOURCE_DIR} )
add_definitions( -DCASSVIEW_VERSION=\"${CASSVIEW_VERSION_MAJOR}.${CASSVIEW_VERSION_MINOR}.${CASSVIEW_VERSION_PATCH}\" )

set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Xml
    Qt5::Widgets
)

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

include_directories(
    ${CASSWRAPPER_INCLUDE_DIRS}
    ${SNAPWEBSITES_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

add_subdirectory(src)
#add_subdirectory(tests)
#add_subdirectory(doc)

install( FILES cassview.desktop DESTINATION share/applications )

# vim: ts=4 sw=4 et
