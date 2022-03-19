# - Try to find Sitter
#
# Once done this will define
#
# SITTER_FOUND        - System has Sitter
# SITTER_INCLUDE_DIRS - The Sitter include directories
# SITTER_LIBRARIES    - The libraries needed to use Sitter
# SITTER_DEFINITIONS  - Compiler switches required for using Sitter
#
# License:
#
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

find_path(
    SITTER_INCLUDE_DIR
        sitter/version.h

    PATHS
        $ENV{SITTER_INCLUDE_DIR}
)

find_library(
    SITTER_LIBRARY
        sitter

    PATHS
        $ENV{SITTER_LIBRARY}
)

mark_as_advanced(
    SITTER_INCLUDE_DIR
    SITTER_LIBRARY
)

set(SITTER_INCLUDE_DIRS ${SITTER_INCLUDE_DIR})
set(SITTER_LIBRARIES    ${SITTER_LIBRARY})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set SITTER_FOUND to
# TRUE if all listed variables are TRUE
find_package_handle_standard_args(
    Sitter
    DEFAULT_MSG
    SITTER_INCLUDE_DIR
    SITTER_LIBRARY
)

# vim: ts=4 sw=4 et
