#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      csspp -- a CSS Preprocessor
#      Copyright (c) 2015-2017 Made to Order Software Corp.
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

##
## Documentation
##
AddDoxygenTarget(
    csspp
    ${CSSPP_VERSION_MAJOR}
    ${CSSPP_VERSION_MINOR}
    ${CSSPP_VERSION_PATCH}
)

# At this point this may not work on all platform, so to make it easier
# I specialize it to Linux only; that just means one doxygen \include
# will fail.
if( DOXYGEN_FOUND )
    if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
        add_dependencies(csspp_Documentation
            sample.css
        )
    
        set( CSSPP_TOOL csspp-tool )
    
        add_custom_target(sample.css
            DEPENDS ${CSSPP_TOOL}
            COMMAND ${CSSPP_TOOL}
                -I ${CMAKE_SOURCE_DIR}/scripts ${CMAKE_BINARY_DIR}/scripts
                --debug --style expanded --output sample.css
                ${CMAKE_CURRENT_SOURCE_DIR}/sample.scss
        )
    endif()
endif()

## Website front page is done manually here for now...
## (At least until Snap! Websites works as expected for us.)

execute_process(
    COMMAND "date" "+%Y.%m.%d"
    OUTPUT_VARIABLE NOW
)

# Win32 date would use something like this
# execute_process(COMMAND "cmd" " /C date /T" OUTPUT_VARIABLE ${RESULT})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/front-page.html.in
    ${CMAKE_CURRENT_BINARY_DIR}/front-page.html
)

# Local Variables:
# indent-tabs-mode: nil
# tab-width: 4
# End:

# vim: ts=4 sw=4 et nocindent
