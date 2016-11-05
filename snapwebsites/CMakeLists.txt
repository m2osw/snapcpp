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


cmake_minimum_required(VERSION 3.5)

ConfigureMakeProject( PROJECT_NAME libsnapwebsites
    CONFIG_ARGS
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTSERIALIZATION_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtSerialization
        -DCSSPP_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

ConfigureMakeProject( PROJECT_NAME cassview
)

ConfigureMakeProject( PROJECT_NAME snapbackend
)

ConfigureMakeProject( PROJECT_NAME snapbackup
)

ConfigureMakeProject( PROJECT_NAME snapbase
)

ConfigureMakeProject( PROJECT_NAME snapbounce
)

ConfigureMakeProject( PROJECT_NAME snapcgi
)

ConfigureMakeProject( PROJECT_NAME snapcommunicator
)

ConfigureMakeProject( PROJECT_NAME snapdb
)

ConfigureMakeProject( PROJECT_NAME snapdbproxy
)

ConfigureMakeProject( PROJECT_NAME snapfirewall
    CONFIG_ARGS
        -DSNAPMANAGERCGI_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

#ConfigureMakeProject( PROJECT_NAME snapinit
#)

ConfigureMakeProject( PROJECT_NAME snaplayout
)

ConfigureMakeProject( PROJECT_NAME snaplock
)

ConfigureMakeProject( PROJECT_NAME snaplog
)

ConfigureMakeProject( PROJECT_NAME snapmanager
    CONFIG_ARGS
        -DADVGETOPT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

ConfigureMakeProject( PROJECT_NAME snapmanagercgi
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

ConfigureMakeProject( PROJECT_NAME snapserver
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

ConfigureMakeProject( PROJECT_NAME snapserver-core-plugins
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

ConfigureMakeProject( PROJECT_NAME snapwatchdog
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
)

get_property( BUILD_TARGETS    GLOBAL PROPERTY BUILD_TARGETS    )
get_property( CLEAN_TARGETS    GLOBAL PROPERTY CLEAN_TARGETS    )
get_property( PACKAGE_TARGETS  GLOBAL PROPERTY PACKAGE_TARGETS  )
get_property( DPUT_TARGETS     GLOBAL PROPERTY DPUT_TARGETS     )
get_property( PBUILDER_TARGETS GLOBAL PROPERTY PBUILDER_TARGETS )

add_custom_target( build-snapwebsites ALL
    DEPENDS ${BUILD_TARGETS}
)
add_custom_target( clean-snapwebsites
    COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
    DEPENDS ${CLEAN_TARGETS}
)
add_custom_target( debuild-snapwebsites
    DEPENDS ${PACKAGE_TARGETS}
)
add_custom_target( dput-snapwebsites
    DEPENDS ${DPUT_TARGETS}
)
add_custom_target( pbuilder-snapwebsites
    DEPENDS ${PBUILDER_TARGETS}
)

# vim: ts=4 sw=4 et nocindent
