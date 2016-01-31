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
## Initialization
##
set(SNAPWATCHDOG_VERSION_MAJOR  0)
set(SNAPWATCHDOG_VERSION_MINOR  2)
set(SNAPWATCHDOG_VERSION_PATCH  1)

cmake_minimum_required(VERSION 2.8)


project( snapwatchdog_project )

## Include support modules
##
set( CMAKE_MODULE_PATH
    ${CMAKE_PREFIX_PATH}/share/cmake/Modules
    ${PROJECT_SOURCE_DIR}/cmake
    ${CMAKE_MODULE_PATH}
)

find_package( AdvGetOpt        REQUIRED )
#find_package( ControlledVars   REQUIRED )
find_package( LibTLD           REQUIRED )
find_package( log4cplus        REQUIRED )
find_package( Qt4 4.8.1 QUIET  REQUIRED QtCore QtNetwork QtXml QtXmlPatterns )
find_package( QtCassandra      REQUIRED )
find_package( QtSerialization  REQUIRED )
find_package( SnapCMakeModules REQUIRED )
#find_package( SnapDoxygen      REQUIRED )
find_package( SnapWebsites     REQUIRED )
find_package( Thrift           REQUIRED )
#find_package( ZLIB             REQUIRED )
#find_package( ZipIos++         REQUIRED 0.1.5.9 )
#find_program( ZIP zip REQUIRED )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message("Debug is in effect for snapwatchdog!")
    add_definitions( -DDEBUG -D_DEBUG -DCONTROLLED_VARS_DEBUG )
else()
    message("Debug is turned OFF")
    add_definitions( -DNDEBUG )
endif()
# A few extra warnings specifically for snapwatchdog libraries and tools
# You can also play with -Weffc++ although we are definitively not compliant
# (especially we do not define all the auto-initialized variables!)
# -Wconversion -- would be nice, a few things are still not cooperating with that one
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

include_directories(
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/lib
    ${SNAPWEBSITES_INCLUDE_DIRS}
    ##${PROJECT_SOURCE_DIR}
    ##${ADVGETOPT_INCLUDE_DIRS}
    ##${QTCASSANDRA_INCLUDE_DIRS}
    ##${QTSERIALIZATION_INCLUDE_DIRS}
    ##${CMAKE_BINARY_DIR}
    ##${CONTROLLEDVARS_INCLUDE_DIRS}
    ##${LIBTLD_INCLUDE_DIRS}
    ##${THRIFT_INCLUDE_DIR}
)

##
## Compiling
##
add_subdirectory(cmake)         # SnapWebsites CMake Modules
add_subdirectory(lib)           # Common code which is too specialized to create a separate library
add_subdirectory(plugins)       # Plug-ins (i.e. modular functionality)
add_subdirectory(src)           # Actual Snap! Websites server
#add_subdirectory(layouts)       # Layouts offered along Core
#add_subdirectory(tests)         # Tests to run against the server
#add_subdirectory(doc)           # API Documentation
add_subdirectory(conf)          # Configuration files and samples

##
## Analyse xml and javascript
##
#snap_build_xml_targets()
#snap_build_js_targets()

##
## Install run script
##
#install( PROGRAMS reset-db.sh run-server.sh DESTINATION ${CMAKE_INSTALL_PREFIX} )

##
## Code Analysis
##
## TODO:
## Look into a way to compile everything using the -Weffc++ flag because that
## gives us many interesting warnings;
##
#add_custom_target(snap_code_analysis
#    COMMAND mkdir -p ${PROJECT_BINARY_DIR}/analysis
#    # Count things and report potential problems with different classes
#    COMMAND echo "cccc -- output directory ${PROJECT_BINARY_DIR}/analysis/cccc -- and errors saved in ${PROJECT_BINARY_DIR}/analysis/cccc.output"
#    COMMAND cccc --lang=c++ --html_outfile=${PROJECT_BINARY_DIR}/analysis/cccc/index.html --outdir=${PROJECT_BINARY_DIR}/analysis/cccc lib/*.cpp lib/*.h lib/*.h.in src/*.cpp plugins/*/*.cpp plugins/*/*.h >${PROJECT_BINARY_DIR}/analysis/cccc.output 2>&1
#    # Boost inspect tool that reports different problems
#    COMMAND echo "inspect -- output ${PROJECT_BINARY_DIR}/analysis/inspect.html"
#    COMMAND inspect -tab -crlf -end -path_name -ascii -minmax -assert_macro -deprecated_macro -unnamed -link -copyright >"${PROJECT_BINARY_DIR}/analysis/inspect.html"
#    # Static analysis of the code
#    COMMAND echo "cppcheck -- output ${PROJECT_BINARY_DIR}/analysis/cppcheck.out"
#    COMMAND cppcheck lib plugins src tests >"${PROJECT_BINARY_DIR}/analysis/cppcheck.out" 2>&1
#    # Count # of TODO, XXX, TBD, FIXME, and \todo: (documented in doxygen)
#    COMMAND sh tests/todo.sh "${PROJECT_BINARY_DIR}/analysis"
#
#    # All of these are expected to work on source code so make sure we're
#    # in the source code top directory
#    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#)

#add_custom_target(snap_javascript_analysis
#    # Compiler need to be scripts that get called on each compile
#    # because otherwise errors will pile up! This will require us
#    # to move to 14.10 so we get the latest working version of
#    # javascript closure compiler.
#
#    # Lint the JavaScript code to find additional "errors"
#    COMMAND sh tests/js-lint.sh "${PROJECT_BINARY_DIR}/analysis"
#    # Compile the JavaScript code to verify most everything (strong typing, etc.)
#    COMMAND sh tests/js-compile.sh "${PROJECT_BINARY_DIR}/analysis"
#
#    # All of these are expected to work on source code so make sure we're
#    # in the source code top directory
#    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#)

# vim: ts=4 sw=4 et nocindent
