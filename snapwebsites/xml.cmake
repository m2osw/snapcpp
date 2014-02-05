################################################################################
# Snap Websites Server -- prepare XML content for checking against DTD
# Copyright (C) 2013-2014  Made to Order Software Corp.
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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
################################################################################

if( NOT DEFINED XML_CMAKE )
set( XML_CMAKE TRUE )


################################################################################
# Handle linting the xml files...
#
find_program( BASH    bash    /bin     )
find_program( XMLLINT xmllint /usr/bin )
#
set( lint_script ${snapwebsites_project_BINARY_DIR}/dolint.sh )
file( WRITE  ${lint_script} "#!${BASH}\n"                                                            )
file( APPEND ${lint_script} "${XMLLINT} --dtdvalid $3 --output $2 $1 && exit 0 || (rm $2; exit 1)\n" )
#
function( snap_validate_xml XML_FILE DTD_FILE )
    get_filename_component( FULL_XML_PATH ${XML_FILE} ABSOLUTE )
    if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_FILE}" )
        set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_FILE}" )
    else()
        set( DTD_PATH "${snapwebsites_project_plugins_SOURCE_DIR}/${DTD_FILE}" )
    endif()
    set_property( GLOBAL APPEND PROPERTY XML_FILE_LIST "${FULL_XML_PATH}" "${DTD_PATH}" "${CMAKE_CURRENT_BINARY_DIR}" )
endfunction()


endif()

# vim: ts=4 sw=4 et nocindent
