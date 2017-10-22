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
#      Copyright (c) 2011-2017 Made to Order Software Corp.
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

project( snapwebsites_main )


################################################################################
# Load up necessary modules
#
set( CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/Modules
    ${CMAKE_BINARY_DIR}/dist/share/cmake-3.5/Modules
    ${CMAKE_BINARY_DIR}/dist/share/cmake
)
find_package( SnapBuild REQUIRED )


################################################################################
# Update all submodules
#find_package( Git REQUIRED )
#execute_process( COMMAND ${GIT_EXECUTABLE} submodule update --remote --merge )


################################################################################
#
ConfigureMakeProject( PROJECT_NAME cmake COMPONENT contrib )
#
# Include snapwebsites projects next
#
add_subdirectory( contrib      )

ConfigureMakeProject( PROJECT_NAME snapwebsites COMPONENT main )

BuildRepro    ( "main" )
CreateTargets ( "top"  )

# vim: ts=4 sw=4 et nocindent
