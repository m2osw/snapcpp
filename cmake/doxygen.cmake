################################################################################
# Add a target to generate API documentation with Doxygen
#
# Usage: AddDoxygenTarget( TARGET_NAME VERSION_MAJOR VERSION_MINOR VERSION_PATCH )
# where: TARGET_NAME is the name of the project (e.g. libQtCassandra)
#        VERSION_*   shall be used as the base of the tarball generated from the html folder.
#
# AddDoxygenTarget() assumes that the doxyfile lives under CMAKE_CURRENT_SOURCE_DIR, with the name ${TARGET_NAME}.doxy.in.
# The ".in" file must have the INPUT and OUTPUT_DIRECTORY values set appropriately. It is recommended to use @project_SOURCE_DIR@
# for INPUT, where "project" is the actual name of your master project. You can either leave OUTPUT_DIRECTORY empty, or set it to
# @CMAKE_CURRENT_BINARY_DIR@.
#
# Also, for version, use @FULL_VERSION@, which contains the major, minor and patch.
#
function( AddDoxygenTarget TARGET_NAME VERSION_MAJOR VERSION_MINOR VERSION_PATCH )
    project( ${TARGET_NAME}_Documentation )

    set( VERSION "${VERSION_MAJOR}.${VERSION_MINOR}" )
    set( FULL_VERSION "${VERSION}.${VERSION_PATCH}" )

    find_package( Doxygen )

    if( DOXYGEN_FOUND )
        if( NOT DOXYGEN_DOT_FOUND )
            message( WARNING "The dot executable was not found. Did you install Graphviz? No graphic output shall be generated in documentation." )
        endif()

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.doxy @ONLY)
        set( DOCUMENTATION_OUTPUT ${TARGET_NAME}-doc-${VERSION} )

        add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}.tar.gz ${DOCUMENTATION_OUTPUT}
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.doxy
            COMMAND echo Compacting as ${DOCUMENTATION_OUTPUT}.tar.gz
            COMMAND rm -rf ${DOCUMENTATION_OUTPUT}
            COMMAND mv html ${DOCUMENTATION_OUTPUT}
            COMMAND tar czf ${DOCUMENTATION_OUTPUT}.tar.gz ${DOCUMENTATION_OUTPUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

        add_custom_target( ${TARGET_NAME}_Documentation ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}.tar.gz
            COMMENT "Generating API documentation with Doxygen" VERBATIM
        )
    else()
        message( WARNING "You do not seem to have doxygen installed on this system, no documentation will be generated." )
    endif()
endfunction()

# vim: ts=4 sw=4 et
