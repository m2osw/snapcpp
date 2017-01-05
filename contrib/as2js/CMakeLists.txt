#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2005-2017 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/project/as2js
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# Build command for now:
#   (mkdir -p ../BUILD/as2js; cd ../BUILD/as2js; cmake -DCMAKE_INSTALL_PREFIX:PATH=/home/snapwebsites/BUILD/dist -DCMAKE_MODULE_PATH:PATH=/home/snapwebsites/BUILD/dist/share/cmake-3.5/Modules ../../as2js)
#
# Build with DEBUG turned on using this definition:
#   -DCMAKE_BUILD_TYPE=Debug
#
# To build with coverage add the following two definitions:
#   -Das2js_compiler_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#

set(AS2JS_VERSION_MAJOR 0)
set(AS2JS_VERSION_MINOR 1)
set(AS2JS_VERSION_PATCH 14)

cmake_minimum_required(VERSION 3.5)

project(as2js_compiler)

## Global C++ command line flags
#if( DEFINED ENV{LIBTLD_COVERAGE} )
#    message("*** COVERAGE TURNED ON ***")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option -fprofile-arcs -ftest-coverage")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Werror -Wall -Wextra -fprofile-arcs -ftest-coverage")
#    set(CMAKE_SHARED_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
#    set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")
#    set(COVERAGE_LIBS gcov)
#else()
#    # If you have problems compiling with the -W... flags, use the -O3 only
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wlogical-op -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=1 -Wundef -Wno-unused -Wunused-variable -Wno-variadic-macros -Wno-parentheses -Wno-unknown-pragmas -Wwrite-strings -Wswitch -fdiagnostics-show-option")
#    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Werror -Wall -Wextra")
#    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
#endif()

enable_language( CXX )
enable_language( C )

#
# Include support modules
# (must be included in a project even though it is not made
# specific to that project)
#
if( NOT CMAKE_MODULE_PATH AND CMAKE_PREFIX_PATH )
    set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_PREFIX_PATH}/share/cmake-3.5/Modules
        )
endif()
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )
#
find_package( AdvGetOpt        REQUIRED )
find_package( SnapCMakeModules REQUIRED )
find_package( SnapDoxygen )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message("Debug is in effect for as2js!")
    add_definitions( -DDEBUG -D_DEBUG )
else()
    message("Debug is turned OFF")
    add_definitions( -DNDEBUG )
endif()

add_subdirectory( cmake )
add_subdirectory( include )
add_subdirectory( lib )
add_subdirectory( src )
add_subdirectory( doc )
add_subdirectory( tests )
#add_subdirectory( package )

# vim: ts=4 sw=4 et
