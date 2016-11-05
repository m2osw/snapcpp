#
# File:
#      src/CMakeLists.txt
#
# Description:
#      The Snap! Websites Server. The server is the main() of the Snap!
#      environment. Most everything is however defined in the
#      libsnapwebsites.so library (see lib/*).
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

##
project(cassview)


##include_directories(
##    SYSTEM
##        ${LOG4CPLUS_INCLUDE_DIR}
##)

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
)

qt5_wrap_ui(UI_FILES
	AboutDialog.ui
	InputDialog.ui
	MainWindow.ui
	SettingsDialog.ui
)

qt5_add_resources(RES_FILES
	resources.qrc
)

add_executable(${PROJECT_NAME}
    resources.qrc
    AboutDialog.cpp
    CassandraModel.cpp
    KeyspaceModel.cpp
	DisplayException.h
	DisplayException.cpp
	InputDialog.cpp
	MainWindow.cpp
	main.cpp
    RowModel.cpp
    SettingsDialog.cpp
    TableModel.cpp
	${UI_FILES}
	${RES_FILES}
)

# while developing you may need to set LD_LIBRARY_PATH
target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CASSWRAPPER_LIBRARIES}
    ${LOG4CPLUS_LIBRARY}
    ${LIBTLD_LIBRARIES}
    ${QT_LIBRARIES}
    ${QTSERIALIZATION_LIBRARIES}
    ${SNAPWEBSITES_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin
)

install(
    FILES snap-medium-icon.png
    DESTINATION share/pixmaps
    RENAME cassview.png
)

# vim: ts=4 sw=4 et
