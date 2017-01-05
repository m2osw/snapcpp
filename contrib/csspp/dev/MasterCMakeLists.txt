#
# File:
#      CMakeLists.txt
#
# Description:
#      Master CMakeLists.txt to compile the whole set of libraries.
#      I may look into removing dependencies to make it easier to
#      compile the csspp project by itself.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2015-2017 Made to Order Software Corp.
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


cmake_minimum_required(VERSION 2.8.4)

project( snapwebsites_main )


################################################################################
# Load up necessary modules
#
set( CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/snapCMakeModules/Modules
    ${CMAKE_BINARY_DIR}/dist/share/cmake-2.8/Modules
)
find_package( SnapBuild        REQUIRED )


################################################################################
# Snap libraries
#
ConfigureMakeProject( PROJECT_NAME cmake )

ConfigureMakeProject( PROJECT_NAME advgetopt
    CONFIG_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS cmake
)

ConfigureMakeProject( PROJECT_NAME csspp
    CONFIG_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS cmake advgetopt
)

get_property( BUILD_TARGETS    GLOBAL PROPERTY BUILD_TARGETS    )
get_property( CLEAN_TARGETS    GLOBAL PROPERTY CLEAN_TARGETS    )
get_property( PACKAGE_TARGETS  GLOBAL PROPERTY PACKAGE_TARGETS  )
get_property( DPUT_TARGETS     GLOBAL PROPERTY DPUT_TARGETS     )
get_property( PBUILDER_TARGETS GLOBAL PROPERTY PBUILDER_TARGETS )

add_custom_target( build-all ALL
    DEPENDS ${BUILD_TARGETS}
)
add_custom_target( clean-all
    COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
    DEPENDS ${CLEAN_TARGETS}
)
add_custom_target( debuild
    DEPENDS ${PACKAGE_TARGETS}
)
add_custom_target( dput
    DEPENDS ${DPUT_TARGETS}
)
add_custom_target( pbuilder
    DEPENDS ${PBUILDER_TARGETS}
)

# vim: ts=4 sw=4 et nocindent syntax=cmake
