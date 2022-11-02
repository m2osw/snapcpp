# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
#
# http://snapwebsites.org/
# contact@m2osw.com
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.10.2)

project(snapwebsites_contribs)


################################################################################
# Snap Contrib libraries and tools
#
# To determine the dependencies of a project, we look at its debian/control
# file. This is done automatically whenever we build the snapcpp project
# and you get a final deps.make using the makefile syntax such as:
#
#    advgetopt: cppthread libutf8
#
# The script further checks the dependencies of the dependencies, so the
# list can be pretty long. We try to optimize the final results and then
# create and SVG using dot. This is also used inside our snapbuilder
# application.
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

ConfigureMakeProject(
    PROJECT_NAME
        commonmarkcpp
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        communicatord
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
        edhttp
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
        fastjournal
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        fluid-settings
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        ftmesh
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
        ipmgr
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
        libexcept
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        libmimemail
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

ConfigureMakeProject(
    PROJECT_NAME
        murmur3
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        prinbee
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        serverplugins
    COMPONENT
        contrib
)

ConfigureMakeProject(
    PROJECT_NAME
        sitter
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
        snaprfs
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
