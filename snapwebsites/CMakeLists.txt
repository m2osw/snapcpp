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
#      Copyright (c) 2011-2012 Made to Order Software Corp.
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


##
## Initialization
##
set(SNAPWEBSITES_VERSION_MAJOR 0)
set(SNAPWEBSITES_VERSION_MINOR 1)
set(SNAPWEBSITES_VERSION_PATCH 0)

cmake_minimum_required(VERSION 2.8)

# Reasons for warnings that are turned off:
# * We use variadic macros for the signals, very useful to avoid otherwise
#   quite complicated statements or many macro definitions (one per # of params).
# * We use | in our parser grammar and we get many parenthesis warnings for those.
# Final flags:
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")
# Development flags:
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++0x -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

# Other interesting warnings but that appear all over the place in other libraries
#   -Wsign-conversion  -- unsigned int a; int b; a = b; or b = a;
#   -Wmissing-declarations  -- we often create local functions without first declaring them

project( snapwebsites )

## Include support modules
##
if( NOT CMAKE_MODULE_PATH )
    set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
endif()
include( doxygen )
include( thrift  )
include( qt      )

include_directories( 
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/lib"
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_BINARY_DIR}/lib"
    "${libQtCassandra_SOURCE_DIR}/include"
    "${libQtCassandra_BINARY_DIR}/include"
    "${libQtSerialization_SOURCE_DIR}/include"
    "${libQtSerialization_BINARY_DIR}/include"
    "${libtld_SOURCE_DIR}"
    "${libtld_BINARY_DIR}"
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_BINARY_DIR}/dist/include"
    "${CMAKE_BINARY_DIR}/dist/include/thrift"
)

##
## Compiling
##
add_subdirectory(lib)           # Common code which is too specialized to create a separate library
add_subdirectory(plugins)       # Plug-ins (i.e. modular functionality)
add_subdirectory(src)           # Actual Snap! Websites server
add_subdirectory(snap-manager)  # Snap Manager to work on Cassandra data from a constrained UI
add_subdirectory(tests)         # Tests to run against the server
add_subdirectory(doc)           # API Documentation


##
## Code Analysis
##
add_custom_target(snap_code_analysis
    # Count things and report potential problems with different classes
    COMMAND echo "cccc -- output ${PROJECT_BINARY_DIR}/cccc"
    COMMAND cccc --lang=c++ --outdir=${PROJECT_BINARY_DIR}/cccc lib/*.cpp lib/*.h lib/*.h.in src/*.cpp plugins/*/*.cpp plugins/*/*.h snap-manager/*.cpp snap-manager/*.h
    # Boost inspect tool that reports different problems
    COMMAND echo "inspect -- output ${PROJECT_BINARY_DIR}/inspect.html"
    COMMAND inspect -crlf -path_name -ascii -minmax -unnamed >"${PROJECT_BINARY_DIR}/inspect.html"

    # All of these are expected to work on source code so make sure we're
    # in the source code top directory
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


##
## Packaging
##
set(CPACK_PACKAGE_NAME "snapwebsites")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "snapwebsites -- a fast CMS written in C++")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "contact@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${snapwebsites_SOURCE_DIR}/LICENSE.txt")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
set(CPACK_PACKAGE_VERSION "${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}.${SNAPWEBSITES_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${SNAPWEBSITES_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${SNAPWEBSITES_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${SNAPWEBSITES_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "snapwebsites-${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}.${SNAPWEBSITES_VERSION_PATCH}")
set(CPACK_PACKAGE_EXECUTABLES "snapwebsites.so")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/snap-websites")
# I'm not too sure how we can add this one... components?
# See http://www.vtk.org/Wiki/CMake:Component_Install_With_CPack
#set(CPACK_DEBIAN_PACKAGE_DEV_DEPENDENCIES "cmake libqt4-dev")
include(CPack)

# vim: ts=4 sw=4 et
