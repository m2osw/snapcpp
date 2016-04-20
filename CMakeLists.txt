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
#      Copyright (c) 2011-2015 Made to Order Software Corp.
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


cmake_minimum_required(VERSION 2.8)

project( snapwebsites_main )


################################################################################
# Load up necessary modules
#
set( CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/snapCMakeModules/Modules
    ${CMAKE_BINARY_DIR}/dist/share/cmake-2.8/Modules
    ${CMAKE_BINARY_DIR}/dist/share/cmake
)
find_package( SnapBuild REQUIRED )


################################################################################
# Snap libraries
#
ConfigureMakeProject( PROJECT_NAME snapCMakeModules )

ConfigureMakeProject( PROJECT_NAME zipios2
    CONFIG_ARGS
        -DBUILD_ZIPIOS_TESTS:BOOL=OFF
)

ConfigureMakeProject( PROJECT_NAME log4cplus
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++11
        -DLOG4CPLUS_BUILD_TESTING=OFF
        -DLOG4CPLUS_SINGLE_THREADED=OFF
        -DLOG4CPLUS_QT4=ON
)

ConfigureMakeProject( PROJECT_NAME iplock
    DEPENDS snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME controlled_vars
    DEPENDS snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME libtld
    DEPENDS snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME advgetopt
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS snapCMakeModules controlled_vars
)

ConfigureMakeProject( PROJECT_NAME as2js
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS snapCMakeModules controlled_vars advgetopt
)

ConfigureMakeProject( PROJECT_NAME csspp
    DEPENDS snapCMakeModules advgetopt
)

ConfigureMakeProject( PROJECT_NAME googlerank
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS snapCMakeModules controlled_vars
)

ConfigureMakeProject( PROJECT_NAME libQtCassandra
        CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS snapCMakeModules controlled_vars as2js libtld
)

ConfigureMakeProject( PROJECT_NAME libQtSerialization
    DEPENDS snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME snapwebsites
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTSERIALIZATION_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtSerialization
        -DCSSPP_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS zipios2 libQtCassandra libQtSerialization libtld as2js log4cplus csspp
)

ConfigureMakeProject( PROJECT_NAME snapwatchdog
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS snapwebsites
)

ConfigureMakeProject( PROJECT_NAME snapmanager
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTCASSANDRA_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtCassandra
    DEPENDS snapwebsites
)

ConfigureMakeProject( PROJECT_NAME cassview
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTSERIALIZATION_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtSerialization
        -DSNAPWEBSITES_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    DEPENDS snapwebsites
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

# vim: ts=4 sw=4 et nocindent
