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

cmake_minimum_required(VERSION 3.10.2)

project(snapbuilder_tool)

set(CMAKE_AUTOMOC TRUE)

find_package(AdvGetOpt          REQUIRED)
find_package(CppProcess         REQUIRED)
find_package(EventDispatcher    REQUIRED)
find_package(EventDispatcherQt  REQUIRED)
find_package(LibExcept          REQUIRED)
find_package(Qt5Core            REQUIRED)
find_package(Qt5Svg             REQUIRED)
find_package(Qt5Widgets         REQUIRED)
find_package(SnapCMakeModules   REQUIRED)
find_package(SnapDev            REQUIRED)
find_package(SnapLogger         REQUIRED)
find_package(X11                REQUIRED)

SnapGetVersion(SNAPBUILDER ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(src)

# vim: ts=4 sw=4 et
