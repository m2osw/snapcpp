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
    DISTFILE_PATH ${CMAKE_SOURCE_DIR}/3rdParty/thrift-${THRIFT_VERSION}.tar.gz
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

#add_subdirectory(  controlled_vars    )
#add_subdirectory(  libtld             )
#add_subdirectory(  libQtCassandra     )
#add_subdirectory(  libQtSerialization )
#add_subdirectory(  snapwebsites       )
#add_subdirectory( backup             )
#add_subdirectory(  googlerank         )
#add_subdirectory(  iplock             )
#add_subdirectory(  cassview           )

ConfigureMakeProject( PROJECT_NAME controlled_vars )
ConfigureMakeProject( PROJECT_NAME libtld          )

##
## Packaging
##
set(CPACK_PACKAGE_NAME "snapwebsites_main")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "snapwebsites environement -- a fast and complete CMS written in C++")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "contact@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${snapwebsites_main_SOURCE_DIR}/snapwebsites/LICENSE.txt")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;/research/;a.txt;/cassandra/;BUILD;Build;build")
set(CPACK_PACKAGE_VERSION "${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}.${SNAPWEBSITES_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${SNAPWEBSITES_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${SNAPWEBSITES_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${SNAPWEBSITES_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "snapwebsites_main-${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR}.${SNAPWEBSITES_VERSION_PATCH}")
#set(CPACK_PACKAGE_EXECUTABLES "snapwebsites.so")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/snap-websites")
# I'm not too sure how we can add this one... components?
# See http://www.vtk.org/Wiki/CMake:Component_Install_With_CPack
#set(CPACK_DEBIAN_PACKAGE_DEV_DEPENDENCIES "cmake libqt4-dev")
include(CPack)

# vim: ts=4 sw=4 et
