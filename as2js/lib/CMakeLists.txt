#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2005-2013 Made to Order Software Corp.
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
## AS to JavaScript library
##
project(libas2js)
include_directories(
    ${CONTROLLEDVARS_INCLUDE_DIR}
    ${as2js_compiler_BINARY_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/include
)

add_library( as2js SHARED
    class.cpp
    db.cpp
    directive.cpp
    expression.cpp
    function.cpp
    json.cpp
    lexer.cpp
    message.cpp
    node.cpp
    optimizer.cpp
    options.cpp
    package.cpp
    position.cpp
    pragma.cpp
    program.cpp
    statement.cpp
    stream.cpp
    string.cpp
    parser.cpp
    variable.cpp
    rc.cpp
    version.cpp

    #compile.cpp
    #compiler.cpp
)
set_target_properties( as2js PROPERTIES
        VERSION ${AS2JS_VERSION_MAJOR}.${AS2JS_VERSION_MINOR}
        SOVERSION ${AS2JS_VERSION_MAJOR}
)
install(
    TARGETS as2js
    LIBRARY DESTINATION lib
    COMPONENT runtime
)


# vim: ts=4 sw=4 et
