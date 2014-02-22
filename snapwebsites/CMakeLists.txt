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
#      Copyright (c) 2011-2014 Made to Order Software Corp.
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
set(SNAPWEBSITES_VERSION_MAJOR 0)
set(SNAPWEBSITES_VERSION_MINOR 1)
set(SNAPWEBSITES_VERSION_PATCH 17)

cmake_minimum_required(VERSION 2.8)

# Reasons for warnings that are turned off:
# * We use variadic macros for the signals, very useful to avoid otherwise
#   quite complicated statements or many macro definitions (one per # of params).
# * We use | in our parser grammar and we get many parenthesis warnings for those.
# Final flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")
# Development flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

# Other interesting warnings but that appear all over the place in other libraries
#   -Wsign-conversion  -- unsigned int a; int b; a = b; or b = a;
#   -Wmissing-declarations  -- we often create local functions without first declaring them

project( snapwebsites_project )

## Include support modules
##
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules/Modules )
    message( "Found snapCMakeModules in ${CMAKE_MODULE_PATH}" )
endif()

find_package( AdvGetOpt        REQUIRED )
find_package( ControlledVars   REQUIRED )
find_package( LibTLD           REQUIRED )
find_package( QtCassandra      REQUIRED )
find_package( QtSerialization  REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen      REQUIRED )
find_package( Thrift           REQUIRED )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message("Debug is in effect for snapwebsites!")
    add_definitions( -DDEBUG -DCONTROLLED_VARS_DEBUG )
else()
    message("Debug is turned OFF")
endif()
# These are the 3 extra warnings that did not make it in the modules
# -Wold-style-cast -Wstrict-overflow=5 -Wnoexcept
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wstrict-overflow=5 -Wnoexcept -fwrapv" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

include_directories( 
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/lib
    ${ADVGETOPT_INCLUDE_DIRS}
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${LIBTLD_INCLUDE_DIRS}
    ${THRIFT_INCLUDE_DIR}
)

##
## Compiling
##
add_subdirectory(lib)           # Common code which is too specialized to create a separate library
add_subdirectory(plugins)       # Plug-ins (i.e. modular functionality)
add_subdirectory(src)           # Actual Snap! Websites server
add_subdirectory(snap-manager)  # Snap Manager to work on Cassandra data from a constrained UI
add_subdirectory(layouts)       # Layouts offered along Core
add_subdirectory(tests)         # Tests to run against the server
add_subdirectory(doc)           # API Documentation
add_subdirectory(conf)          # Configuration files and samples

##
## Analyse xml
##
snap_build_xml_targets()

##
## Code Analysis
##
add_custom_target(snap_code_analysis
    # Count things and report potential problems with different classes
    COMMAND echo "cccc -- output directory ${PROJECT_BINARY_DIR}/cccc -- and errors saved in ${PROJECT_BINARY_DIR}/cccc.output"
    COMMAND cccc --lang=c++ --html_outfile=${PROJECT_BINARY_DIR}/cccc/index.html --outdir=${PROJECT_BINARY_DIR}/cccc lib/*.cpp lib/*.h lib/*.h.in src/*.cpp plugins/*/*.cpp plugins/*/*.h snap-manager/*.cpp snap-manager/*.h >${PROJECT_BINARY_DIR}/cccc.output 2>&1
    # Boost inspect tool that reports different problems
    COMMAND echo "inspect -- output ${PROJECT_BINARY_DIR}/inspect.html"
    COMMAND inspect -tab -crlf -path_name -ascii -minmax -unnamed >"${PROJECT_BINARY_DIR}/inspect.html"
    # Static analysis of the code
    COMMAND echo "cppcheck -- output ${PROJECT_BINARY_DIR}/cppcheck.out"
    COMMAND cppcheck lib plugins snap-manager src tests >"${PROJECT_BINARY_DIR}/cppcheck.out" 2>&1

    # All of these are expected to work on source code so make sure we're
    # in the source code top directory
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# vim: ts=4 sw=4 et nocindent
