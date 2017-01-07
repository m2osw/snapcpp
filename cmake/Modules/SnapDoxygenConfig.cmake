# File:         FindSnapDoxygen.cmake
# Object:       Find the Doxygen module and create a function which provides targets.
#
# Copyright:    Copyright (c) 2011-2017 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
find_package( Doxygen )

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

    if( DOXYGEN_FOUND )
        if( NOT DOXYGEN_DOT_FOUND )
            message( WARNING "The dot executable was not found. Did you install Graphviz? No graphic output shall be generated in documentation." )
        endif()

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.doxy @ONLY)
        set( DOCUMENTATION_OUTPUT ${TARGET_NAME}-doc-${VERSION} )

        add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}.tar.gz ${DOCUMENTATION_OUTPUT}
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.doxy
                1> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}-doxy.log
                2> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}-doxy.err
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

        string( TOLOWER ${TARGET_NAME} LOWER_TARGET_NAME )
        install( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${DOCUMENTATION_OUTPUT}/
                DESTINATION share/doc/${LOWER_TARGET_NAME}/html/
        )
    else()
        message( WARNING "You do not seem to have doxygen installed on this system, no documentation will be generated." )
    endif()
endfunction()

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args(
    SnapDoxygen
    DEFAULT_MSG
    DOXYGEN_FOUND
)

# vim: ts=4 sw=4 et
