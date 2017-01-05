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

##
## AS to JavaScript tools
##
# The project name should be "as2js", unfortunately the library target
# has to be named that way to work...
project(as2js-compiler)

include_directories(
    ${as2js_compiler_BINARY_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/include
    ${as2js_SOURCE_DIR}
    ${ADVGETOPT_INCLUDE_DIRS}
)
add_executable(${PROJECT_NAME}
    as2js.cpp
    license.cpp
)
target_link_libraries(${PROJECT_NAME}
    as2js
    ${ADVGETOPT_LIBRARIES}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME as2js
    VERSION ${AS2JS_VERSION_MAJOR}.${AS2JS_VERSION_MINOR}
    SOVERSION ${AS2JS_VERSION_MAJOR}
)
install(
    TARGETS ${PROJECT_NAME}
    DESTINATION bin
    COMPONENT runtime
)


# vim: ts=4 sw=4 et
