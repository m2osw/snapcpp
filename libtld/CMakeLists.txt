#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2011-2013 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/
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

set(LIBTLD_VERSION_MAJOR 1)
set(LIBTLD_VERSION_MINOR 4)
set(LIBTLD_VERSION_PATCH 3)

cmake_minimum_required(VERSION 2.8.4)

project(tld_library)

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

#
# Include support modules
# (must be included in a project even though it is not made
# specific to that project)
#
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules/Modules )
    message( "Found snapCMakeModules in ${CMAKE_MODULE_PATH}" )
endif()
#
find_package( SnapCMakeModules )
find_package( SnapDoxygen )
find_package( Qt4 4.8.1 REQUIRED QtCore QtXml )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )


add_subdirectory( include )
add_subdirectory( src )
add_subdirectory( doc )
add_subdirectory( tests )
add_subdirectory( package )

###
### Packaging
###
#set(CPACK_PACKAGE_NAME "libtld")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libtld -- a C library to extract TLDs in a URI")
#set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
#set(CPACK_PACKAGE_CONTACT "alexis@m2osw.com")
#set(CPACK_RESOURCE_FILE_LICENSE "${tld_library_SOURCE_DIR}/LICENSE.txt")
#set(CPACK_GENERATOR "TGZ" "DEB")
#set(CPACK_SOURCE_GENERATOR "TGZ")
#set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;.*\\\\.o$;.*\\\\.so$;BUILD;Build;build;.gitignore")
#set(CPACK_PACKAGE_VERSION "${LIBTLD_VERSION_MAJOR}.${LIBTLD_VERSION_MINOR}.${LIBTLD_VERSION_PATCH}")
#set(CPACK_PACKAGE_VERSION_MAJOR "${LIBTLD_VERSION_MAJOR}")
#set(CPACK_PACKAGE_VERSION_MINOR "${LIBTLD_VERSION_MINOR}")
#set(CPACK_PACKAGE_VERSION_PATCH "${LIBTLD_VERSION_PATCH}")
#set(CPACK_SOURCE_PACKAGE_FILE_NAME "libtld-${LIBTLD_VERSION_MAJOR}.${LIBTLD_VERSION_MINOR}.${LIBTLD_VERSION_PATCH}")
#set(CPACK_PACKAGE_EXECUTABLES "libtld.so")
#set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/tld-library-libtld")
## I'm not too sure how we can add this one...
##set(CPACK_DEBIAN_PACKAGE_DEV_DEPENDENCIES "cmake libqt4-dev")
#include(CPack)

# vim: ts=4 sw=4 et
