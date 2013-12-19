#
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
#

project( snapwebsites_plugins )

# We need a common place to install the plugins
set( PLUGIN_INSTALL_DIR lib/snapwebsites/plugins )

find_program( BASH    bash    /bin     )
find_program( XMLLINT xmllint /usr/bin )

function( snap_validate_dtd )
	set( lint_script ${snapwebsites_plugins_BINARY_DIR}/dolint.sh )
	if( NOT EXISTS ${lint_script} )
		file( WRITE  ${lint_script} "#!${BASH}\n"                 )
		file( APPEND ${lint_script} "rm -f $2\n"                  )
		file( APPEND ${lint_script} "${XMLLINT} $1 && touch $2\n" )
	endif()
	foreach( xml_file ${ARGN} )
		get_filename_component( full_xmlfile ${xml_file} ABSOLUTE )
		set( touch_file "${CMAKE_CURRENT_BINARY_DIR}/${xml_file}.lint" )
		message( STATUS "running xmllint on ${full_xmlfile}" )
		execute_process(
			COMMAND ${BASH} ${lint_script} ${full_xmlfile} ${touch_file}
			OUTPUT_QUIET
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)
		if( NOT EXISTS ${touch_file} )
			message( FATAL_ERROR "${full_xmlfile} fails to pass xmllint!" )
		endif()
	endforeach()
endfunction()


# Keep the plugins in alphabetical order for ease of management
add_subdirectory(char_chart)
add_subdirectory(content)
add_subdirectory(favicon)
add_subdirectory(filter)
add_subdirectory(form)
add_subdirectory(header)
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

