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
    COMPONENT main
)

ConfigureMakeProject( PROJECT_NAME cassview         COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapbackend      COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapbackup       COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapbase         COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapbounce       COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapcgi          COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapcommunicator COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapdb           COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapdbproxy      COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapfirewall
    CONFIG_ARGS
        -DSNAPMANAGERCGI_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    COMPONENT main
)
ConfigureMakeProject( PROJECT_NAME snaplayout       COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snaplock         COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snaplog          COMPONENT main )
ConfigureMakeProject( PROJECT_NAME snapmanager
    CONFIG_ARGS
        -DADVGETOPT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DQTCASSANDRA_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtCassandra
    COMPONENT main
)

ConfigureMakeProject( PROJECT_NAME snapmanagercgi
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    COMPONENT main
)

ConfigureMakeProject( PROJECT_NAME snapserver
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    COMPONENT main
)

ConfigureMakeProject( PROJECT_NAME snapserver-core-plugins
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    COMPONENT main
)

ConfigureMakeProject( PROJECT_NAME snapwatchdog
    CONFIG_ARGS
        -DLIBTLD_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
        -DLOG4CPLUS_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include
    COMPONENT main
)

BuildRepro   ( "main" )
CreateTargets( "main" )

# vim: ts=4 sw=4 et nocindent
