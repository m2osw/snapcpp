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
# Note: <null> makes it explicit that there are no dependencies.
#
#   advgetopt: cmake
#   as2js: advgetopt
#   cassandra-cpp-driver-snap: libuv-snap
#   cmake: <null> (snapcmakemodules)
#   csspp: advgetopt
#   iplock: advgetopt libaddr
#   libaddr: libexcept
#   libcassvalue: libexcept
#   libcasswrapper: cassandra-cpp-driver-snap as2js libcassvalue
#   libexcept: cmake
#   libQtSerialization: cmake
#   libtld: cmake
#   libuv1-snap: <null>
#   log4cplus: cmake
#   snapwebsites: (all the others, always upload last)
#   zipios: <null>
#
# IMPORTANT: The DEPENDS parameter is for information only at the moment,
#            it is not required by the cmake configuration. That way, though
#            we can document our dependencies here (it is also defined in the
#            debian/control file of each project.)
#

ConfigureMakeProject(
    PROJECT_NAME
        advgetopt
    COMPONENT
        contrib
    DEPENDS
        cmake
)

ConfigureMakeProject(
    PROJECT_NAME
        as2js
    COMPONENT
        contrib
    DEPENDS
        advgetopt
        cmake
)

# TODO: make the -std=c++14 a variable in our SnapBuild use it here
ConfigureMakeProject(
    PROJECT_NAME
        cassandra-cpp-driver-snap
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++14
    COMPONENT
        contrib
    DEPENDS
        cmake
)

ConfigureMakeProject(
    PROJECT_NAME
        csspp
    COMPONENT
        contrib
    DEPENDS
        advgetopt
        cmake
)

ConfigureMakeProject(
    PROJECT_NAME
        iplock
    COMPONENT
        contrib
    DEPENDS
        advgetopt
        cmake
        libaddr
        libexcept
)

ConfigureMakeProject(
    PROJECT_NAME
        libaddr
    COMPONENT
        contrib
    DEPENDS
        cmake
        libexcept
)

ConfigureMakeProject(
    PROJECT_NAME
        libcassvalue
    COMPONENT
        contrib
    DEPENDS
        cmake
        libexcept
)

ConfigureMakeProject(
    PROJECT_NAME
        libcasswrapper
    COMPONENT
        contrib
    DEPENDS
        advgetopt
        cassandra-cpp-driver-snap
        cmake
        as2js
        libcassvalue
        libexcept
)

ConfigureMakeProject(
    PROJECT_NAME
        libexcept
    COMPONENT
        contrib
    DEPENDS
        cmake
)

ConfigureMakeProject(
    PROJECT_NAME
        libQtSerialization
    COMPONENT
        contrib
    DEPENDS
        cmake
)

ConfigureMakeProject(
    PROJECT_NAME
        libtld
    COMPONENT
        contrib
    DEPENDS
        cmake
)

# TODO: make the -std=c++14 a variable in our SnapBuild use it here
ConfigureMakeProject(
    PROJECT_NAME
        libuv1-snap
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++14
    COMPONENT
        contrib
    DEPENDS
        cmake
)

# TODO: make the -std=c++14 a variable in our SnapBuild use it here
ConfigureMakeProject(
    PROJECT_NAME
        log4cplus
    CONFIG_ARGS
        -DCMAKE_CXX_FLAGS=-std=c++14
        -DLOG4CPLUS_BUILD_TESTING=OFF
        -DLOG4CPLUS_SINGLE_THREADED=OFF
        -DLOG4CPLUS_QT4=OFF
        -DLOG4CPLUS_QT5=OFF
    COMPONENT
        contrib
    DEPENDS
        cmake
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
