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

# Plugin names must use underscores
project(sitter_log)

add_library(${PROJECT_NAME} SHARED
    log.cpp
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        ${PLUGIN_INSTALL_DIR}
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}

    DESTINATION
        include/sitter/plugins/log

    FILES_MATCHING PATTERN
        "*.h"
)

# vim: ts=4 sw=4 et
