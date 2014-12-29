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


cmake_minimum_required(VERSION 2.8)

project( snapwebsites_main )


################################################################################
# TODO: doesn't work...
#
#include( ProcessorCount )
#ProcessorCount( NUM_PROCS )
#
#math( EXPR NUM_JOBS "${NUM_PROCS}*2" )
#set( ENV{MAKEFLAGS} -j${NUM_JOBS} )    # Doesn't work. Nice idea, but no dice
#message( "ENV{MAKEFLAGS}=$ENV{MAKEFLAGS}" )


################################################################################
# Load up necessary modules
#
set( CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/snapCMakeModules/Modules
    ${CMAKE_BINARY_DIR}/dist/share/cmake-2.8/Modules
)
find_package( SnapBuild        REQUIRED )

################################################################################
# Third party libraries
#
#add_subdirectory( thrift )


################################################################################
# Snap libraries
#
ConfigureMakeProject( PROJECT_NAME thrift NOINC_DEBVERS )

ConfigureMakeProject( PROJECT_NAME snapCMakeModules )

ConfigureMakeProject( PROJECT_NAME zipios )

ConfigureMakeProject( PROJECT_NAME iplock
    CONFIG_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

ConfigureMakeProject( PROJECT_NAME controlled_vars
    CONFIG_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME libtld
    CONFIG_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME advgetopt
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapCMakeModules controlled_vars
)

ConfigureMakeProject( PROJECT_NAME as2js
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapCMakeModules controlled_vars advgetopt
)

ConfigureMakeProject( PROJECT_NAME googlerank
    CONFIG_ARGS
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapCMakeModules controlled_vars
)

ConfigureMakeProject( PROJECT_NAME libQtCassandra
    CONFIG_ARGS
        -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS thrift snapCMakeModules controlled_vars
)

ConfigureMakeProject( PROJECT_NAME libQtSerialization
    CONFIG_ARGS
        -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS thrift snapCMakeModules
)

ConfigureMakeProject( PROJECT_NAME snapwebsites
    CONFIG_ARGS
        -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTSERIALIZATION_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtSerialization
        -DLIBTLD_LIBRARY:PATH=${CMAKE_BINARY_DIR}/dist/lib/libtld.so
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS thrift snapCMakeModules zipios advgetopt libQtCassandra libQtSerialization libtld controlled_vars as2js
)

ConfigureMakeProject( PROJECT_NAME snapwatchdog
    CONFIG_ARGS
        -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapwebsites
)

ConfigureMakeProject( PROJECT_NAME snapmanager
    CONFIG_ARGS
        -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLIBTLD_LIBRARY:PATH=${CMAKE_BINARY_DIR}/dist/lib/libtld.so
        -DQTCASSANDRA_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtCassandra
        -DQTCASSANDRA_LIBRARY:PATH=${CMAKE_BINARY_DIR}/dist/lib/libQtCassandra.so
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS snapCMakeModules libQtCassandra libtld snapwebsites controlled_vars
)

ConfigureMakeProject( PROJECT_NAME cassview
    CONFIG_ARGS
        -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DCONTROLLEDVARS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTSERIALIZATION_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtSerialization
        -DLIBTLD_LIBRARY:PATH=${CMAKE_BINARY_DIR}/dist/lib/libtld.so
        -DSNAPWEBSITES_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DSNAPWEBSITES_LIBRARY:PATH=${CMAKE_BINARY_DIR}/dist/lib/libsnapwebsites.so
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DEPENDS thrift snapCMakeModules snapwebsites controlled_vars
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
