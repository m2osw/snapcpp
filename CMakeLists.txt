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

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/snapCMakeModules )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapBuild        REQUIRED )

ConfigureMakeProject(
    USE_CONFIGURE_SCRIPT
    PROJECT_NAME  thrift
    VERSION       0.9.0
    DISTFILE_PATH ${CMAKE_SOURCE_DIR}/3rdParty/thrift-0.9.0.tar.gz
    CONFIG_ARGS
        --without-csharp
        --without-java
        --without-erlang
        --without-python
        --without-perl
        --without-php
        --without-php_extension
        --without-ruby
        --without-haskell
        --without-go
)

ConfigureMakeProject( PROJECT_NAME cassview        DEPENDS libQtCassandra )
ConfigureMakeProject( PROJECT_NAME controlled_vars )
ConfigureMakeProject( PROJECT_NAME libtld          )
ConfigureMakeProject( PROJECT_NAME libQtCassandra
        CONFIG_ARGS
            -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/thrift
        DEPENDS thrift controlled_vars
        )
ConfigureMakeProject( PROJECT_NAME libQtSerialization
        CONFIG_ARGS -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/thrift
        DEPENDS thrift
        )
ConfigureMakeProject( PROJECT_NAME snapwebsites
        CONFIG_ARGS
            -DTHRIFT_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/thrift
            -DQTSERIALIZATION_INCLUDE_DIR:PATH=${CMAKE_BINARY_DIR}/dist/include/QtSerialization
            -DLIBTLD_LIBRARY:PATH=${CMAKE_BINARY_DIR}/dist/lib/libtld.so
        DEPENDS libQtCassandra libQtSerialization libtld
        )
ConfigureMakeProject( PROJECT_NAME googlerank
        DEPENDS controlled_vars
        )
ConfigureMakeProject( PROJECT_NAME iplock     )

# vim: ts=4 sw=4 et
