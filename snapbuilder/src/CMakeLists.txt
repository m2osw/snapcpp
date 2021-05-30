# Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/snap-builder
# contact@m2osw.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

##
## snapbuilder tool
##
project(snapbuilder)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)


qt5_wrap_ui(WINDOW_UI snap_builder-MainWindow.ui)


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ADVGETOPT_INCLUDE_DIRS}
    ${LIBEXCEPT_INCLUDE_DIRS}
    ${EVENTDISPATCHER_INCLUDE_DIRS}
    ${EVENTDISPATCHER_QT_INCLUDE_DIRS}
    ${SNAPLOGGER_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
)


add_executable(${PROJECT_NAME}
    main.cpp

    ${WINDOW_UI}
    project.cpp
    snap_builder.cpp
    version.cpp
)

message("EVENTDISPATCHER_LIBRARIES=${EVENTDISPATCHER_LIBRARIES}")
target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${EVENTDISPATCHER_LIBRARIES}
    ${EVENTDISPATCHER_QT_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    Qt5::Core
    Qt5::Widgets
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)

# vim: ts=4 sw=4 et
