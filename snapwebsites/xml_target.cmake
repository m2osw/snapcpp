################################################################################
# Snap Websites Server -- run xmllint against the XML content
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

if( NOT DEFINED XML_TARGET_CMAKE )
set( XML_TARGET_CMAKE TRUE )


################################################################################
# Make sure all lint targets get built properly...
#
set( arg_count 3 )
unset( lint_file_list )
get_property( XML_FILE_LIST GLOBAL PROPERTY XML_FILE_LIST )
list( LENGTH XML_FILE_LIST range )
math( EXPR mod_test "${range} % ${arg_count}" )
if( NOT ${mod_test} EQUAL 0 )
    message( FATAL_ERROR "The list of files must have an even count. Each XML file must have an accompanying DTD file!" )
endif()
#
# Create a lint file for each pair
#
math( EXPR whole_range "${range} - 1" )
foreach( var_idx RANGE 0 ${whole_range} ${arg_count} )
    list( GET XML_FILE_LIST ${var_idx} xml_file )
    math( EXPR next_idx "${var_idx} + 1" )
    list( GET XML_FILE_LIST ${next_idx} dtd_file )
    math( EXPR next_idx "${next_idx} + 1" )
    list( GET XML_FILE_LIST ${next_idx} binary_dir )
    #
    get_filename_component( base_xml_file ${xml_file} NAME )
    set( lint_file "${binary_dir}/${base_xml_file}.lint" )
    #
    # Command runs the lint_script above...
    #
    add_custom_command(
        OUTPUT ${lint_file}
        COMMAND ${BASH} ${lint_script} ${xml_file} ${lint_file} ${dtd_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${xml_file} ${dtd_file}
        COMMENT "running xmllint on ${xml_file}"
    )
    list( APPEND lint_file_list ${lint_file} )
endforeach()
#
# Make each lint file.
#
add_custom_target(
    build_lint ALL
    DEPENDS ${lint_file_list}
)
#
# Handy target for wiping out all lint files and forcing a recheck!
#
add_custom_target(
    clean_lint
    COMMAND rm -rf ${lint_file_list}
    DEPENDS ${lint_file_list}
)


endif()

# vim: ts=4 sw=4 et nocindent
