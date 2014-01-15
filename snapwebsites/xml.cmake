if( NOT DEFINED XML_CMAKE )
set( XML_CMAKE TRUE )


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
	if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_FILE}" )
		set( DTD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${DTD_FILE}" )
	else()
		set( DTD_PATH "${snapwebsites_plugins_SOURCE_DIR}/${DTD_FILE}" )
	endif()
	set_property( GLOBAL APPEND PROPERTY XML_FILE_LIST "${FULL_XML_PATH}" "${DTD_PATH}" "${CMAKE_CURRENT_BINARY_DIR}" )
endfunction()


endif()

# vim: ts=4 sw=4 nocindent
