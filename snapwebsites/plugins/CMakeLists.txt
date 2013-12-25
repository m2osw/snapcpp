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
#      Copyright (c) 2011-2013 Made to Order Software Corp.
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
if( NOT EXISTS ${lint_script} )
	file( WRITE  ${lint_script} "#!${BASH}\n"                                                            )
#	file( APPEND ${lint_script} "dtdopts=--valid\n"                                                      )
	file( APPEND ${lint_script} "[ -n \"$3\" ] && [ -e \"$3\" ] && dtdopts=\"--dtdvalid $3\"\n" )
	file( APPEND ${lint_script} "${XMLLINT} $dtdopts $1 --output $2 && exit 0 || (rm $2; exit 1)\n"             )
endif()
#
macro( snap_validate_dtd PROJECT_NAME XML_FILE DTD_FILE )
	get_filename_component( full_xmlfile ${XML_FILE} ABSOLUTE )
	set( lint_file "${CMAKE_CURRENT_BINARY_DIR}/${XML_FILE}.lint"   )
	set( dtd_file  "${snapwebsites_plugins_SOURCE_DIR}/${DTD_FILE}" )
	#
	# Command runs the lint_script above...
	#
	add_custom_command(
		OUTPUT ${lint_file}
		COMMAND ${BASH} ${lint_script} ${full_xmlfile} ${lint_file} ${dtd_file}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${full_xmlfile}
		COMMENT "running xmllint on ${full_xmlfile}"
	)

	# Make sure each file is processed...
	#
#	add_custom_target(
#		build_lint_${PROJECT_NAME}
#		DEPENDS ${touch_file_list}
#	)
	set_property( GLOBAL APPEND PROPERTY xml_lint_files   ${lint_file}         )
	#set_property( GLOBAL APPEND PROPERTY xml_lint_targets build_lint_${PROJECT_NAME} )
endmacro()


################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(char_chart)
add_subdirectory(content)
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
#get_property( xml_lint_targets GLOBAL PROPERTY xml_lint_targets )
#add_custom_target(
#	build_lint ALL
#	DEPENDS ${xml_lint_targets}
#)
get_property( XML_FILE_LIST GLOBAL PROPERTY XML_FILE_LIST )
	set( lint_file "${CMAKE_CURRENT_BINARY_DIR}/${XML_FILE}.lint"   )
	set( dtd_file  "${snapwebsites_plugins_SOURCE_DIR}/${DTD_FILE}" )
	#
	# Command runs the lint_script above...
	#
	add_custom_command(
		OUTPUT ${lint_file}
		COMMAND ${BASH} ${lint_script} ${full_xmlfile} ${lint_file} ${dtd_file}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${full_xmlfile}
		COMMENT "running xmllint on ${full_xmlfile}"
	)
#
# Handy target for wiping out all lint files and forcing a recheck!
#
get_property( xml_lint_files GLOBAL PROPERTY xml_lint_files )
message( "xml_lint_files=${xml_lint_files}" )
add_custom_target(
	build_lint ALL
	DEPENDS ${xml_lint_files}
)
add_custom_target(
	clean_lint
	COMMAND rm -rf ${xml_lint_files}
	DEPENDS ${xml_lint_files}
)

# vim: ts=4 sw=4 nocindent
