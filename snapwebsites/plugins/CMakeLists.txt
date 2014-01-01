################################################################################
# File:
#      plugins/CMakeLists.txt
#
# Description:
#      Set of extensions used by the Snap! Websites server.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2014 Made to Order Software Corp.
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
################################################################################

project( snapwebsites_plugins )

################################################################################
# We need a common place to install the plugins
#
set( PLUGIN_INSTALL_DIR lib/snapwebsites/plugins )


################################################################################
# Handle linting the xml files...
#
find_program( BASH    bash    /bin     )
find_program( XMLLINT xmllint /usr/bin )
#
set( lint_script ${snapwebsites_plugins_BINARY_DIR}/dolint.sh )
file( WRITE  ${lint_script} "#!${BASH}\n"                                                            )
file( APPEND ${lint_script} "${XMLLINT} --dtdvalid $3 --output $2 $1 && exit 0 || (rm $2; exit 1)\n"             )
#
function( snap_validate_xml XML_FILE DTD_FILE )
	get_filename_component( FULL_XML_PATH ${XML_FILE} ABSOLUTE )
	set_property( GLOBAL APPEND PROPERTY XML_FILE_LIST "${FULL_XML_PATH}" "${DTD_FILE}" "${CMAKE_CURRENT_BINARY_DIR}" )
endfunction()


################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(char_chart)
add_subdirectory(content)
add_subdirectory(editor)
add_subdirectory(favicon)
add_subdirectory(filter)
add_subdirectory(form)
add_subdirectory(header)
add_subdirectory(info)
add_subdirectory(javascript)
add_subdirectory(layout)
add_subdirectory(links)
add_subdirectory(mailinglist)
add_subdirectory(messages)
add_subdirectory(path)
add_subdirectory(permissions)
add_subdirectory(robotstxt)
add_subdirectory(search)
add_subdirectory(sessions)
add_subdirectory(sendmail)
add_subdirectory(shorturl)
add_subdirectory(sitemapxml)
add_subdirectory(taxonomy)
add_subdirectory(users)


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
	list( GET XML_FILE_LIST ${next_idx} base_dtd_file )
	math( EXPR next_idx "${next_idx} + 1" )
	list( GET XML_FILE_LIST ${next_idx} binary_dir )
	#
	set( dtd_file  "${snapwebsites_plugins_SOURCE_DIR}/${base_dtd_file}" )
	get_filename_component( base_xml_file ${xml_file} NAME )
	set( lint_file "${binary_dir}/${base_xml_file}.lint" )
	#
	# Command runs the lint_script above...
	#
	add_custom_command(
		OUTPUT ${lint_file}
		COMMAND ${BASH} ${lint_script} ${xml_file} ${lint_file} ${dtd_file}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${xml_file}
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

# vim: ts=4 sw=4 nocindent
