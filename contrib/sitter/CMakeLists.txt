# sitter -- a tool used to verify service availabilities
# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/sitter
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10.2)

project(sitter_project)

find_package(AdvGetOpt        REQUIRED)
find_package(CppThread        REQUIRED)
find_package(LibExcept        REQUIRED)
find_package(SnapCMakeModules REQUIRED)
find_package(SnapDev          REQUIRED)
find_package(SnapLogger       REQUIRED)
find_package(AtomicNames      REQUIRED)

SnapGetVersion(SITTER ${CMAKE_CURRENT_SOURCE_DIR})

enable_language(CXX)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

##
## Compiling
##
add_subdirectory(conf)              # Configuration files and samples
add_subdirectory(sitter)            # Common code which is too specialized to create a separate library
add_subdirectory(daemon)            # Deamon running sitting scripts & plugins
add_subdirectory(plugins)           # Plug-ins (i.e. modular functionality)
add_subdirectory(tools)             # Various tools
add_subdirectory(cmake)             # CMake Config
add_subdirectory(doc)               # Documentation

# vim: ts=4 sw=4 et nocindent
