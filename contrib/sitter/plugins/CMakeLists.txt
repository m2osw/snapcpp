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

project(sitter_plugins)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)


# We need a common place to install the plugins
set(PLUGIN_INSTALL_DIR lib/sitter/plugins)


# Keep the plugins in alphabetical order for ease of management
add_subdirectory(sitter_apt)
add_subdirectory(sitter_cpu)
add_subdirectory(sitter_disk)
add_subdirectory(sitter_firewall)
add_subdirectory(sitter_flags)
add_subdirectory(sitter_log)
add_subdirectory(sitter_memory)
add_subdirectory(sitter_network)
add_subdirectory(sitter_packages)
add_subdirectory(sitter_processes)
add_subdirectory(sitter_scripts)


# vim: ts=4 sw=4 nocindent
