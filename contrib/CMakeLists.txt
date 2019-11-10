#
# Description:
#      Definitions to create the build environment with cmake
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2019 Made to Order Software Corp.
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

project( snapwebsites_contrib )


################################################################################
# Snap Contrib libraries
#
# Dependencies in make syntax:
#
# (This gives us the order in which we have to compile these projects
# on Launchpad.)
#
# Note 1: <null> makes it explicit that there are no dependencies.
# Note 2: these dependencies are defined in the debian/control files.
#
#   advgetopt: libexcept
#   as2js: advgetopt
#   cassandra-cpp-driver-snap: libuv1-snap
#   snapcmakemodules: <null>
#   csspp: advgetopt
#   iplock: advgetopt libaddr
#   libaddr: libexcept
#   libcassvalue: libexcept
#   libcasswrapper: cassandra-cpp-driver-snap as2js libcassvalue
#   libexcept: snapcmakemodules
#   libQtSerialization: snapcmakemodules
#   libtld: snapcmakemodules
#   libuv1-snap: <null>
#   log4cplus12: snapcmakemodules
#   snapwebsites: (all the others, always upload last)
#   zipios: <null>
#

ConfigureMakeProject(
    PROJECT_NAME
        advgetopt
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        as2js
    COMPONENT
        contrib
)

# TODO: make the -std=c++14 a variable in our SnapBuild use it here
ConfigureMakeProject(
    PROJECT_NAME
        cassandra-cpp-driver-snap
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++14
        -DCASS_BUILD_STATIC=ON
        -DCASS_BUILD_DOCS=ON
        -D_LIBUV_FOUND=TRUE
        -D_LIBUV_LIBDIR=${CMAKE_BINARY_DIR}/dist
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        cppthread
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        csspp
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        eventdispatcher
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        iplock
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libaddr
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libcassvalue
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libcasswrapper
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libexcept
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libQtSerialization
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libtld
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libutf8
    COMPONENT
        contrib
)

# TODO: make the -std=c++14 a variable in our SnapBuild use it here
ConfigureMakeProject(
    PROJECT_NAME
        libuv1-snap
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++14
        -DLIBUV_BUILD_TESTS=OFF
    COMPONENT
        contrib
)

# TODO: make the -std=c++14 a variable in our SnapBuild use it here
ConfigureMakeProject(
    PROJECT_NAME
        log4cplus12
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++14
        -DLOG4CPLUS_BUILD_TESTING=OFF
        -DLOG4CPLUS_SINGLE_THREADED=OFF
        -DLOG4CPLUS_QT4=OFF
        -DLOG4CPLUS_QT5=OFF
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        snapcatch2
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        snapdev
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        snaplogger
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        zipios
    CONFIG_ARGS
        -DBUILD_ZIPIOS_TESTS:BOOL=ON
    COMPONENT
        contrib
)

################################################################################
BuildRepro   ( contrib )
CreateTargets( contrib )

# vim: ts=4 sw=4 et nocindent
