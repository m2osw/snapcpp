#
# File:
#      lib/CMakeLists.txt
#
# Description:
#      The csspp library.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2015 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#     
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#     
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

project(csspp)

add_library( ${PROJECT_NAME} SHARED
    csspp.cpp                                   # Some basics about the library
    error.cpp                                   # Print out error messages
    lexer.cpp                                   # Transform input in tokens
    node.cpp                                    # Holds the data read
    position.cpp                                # Holds the current position
    unicode_range.cpp                           # Handle a Unicode Range value
)

#target_link_libraries( ${PROJECT_NAME}
#    ${LIBTLD_LIBRARIES}
#)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${CSSPP_VERSION_MAJOR}.${CSSPP_VERSION_MINOR}
    SOVERSION ${CSSPP_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib
)

# Local Variables:
# indent-tabs-mode: nil
# tab-width: 4
# End:

# vim: ts=4 sw=4 et nocindent
