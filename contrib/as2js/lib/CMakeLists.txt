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
## AS to JavaScript library
##
project(libas2js)

include_directories(
    ${as2js_compiler_BINARY_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/include
)

add_library( as2js SHARED
    compiler.cpp
    compiler_attributes.cpp
    compiler_class.cpp
    compiler_compile.cpp
    compiler_directive.cpp
    compiler_expression.cpp
    compiler_function.cpp
    compiler_package.cpp
    compiler_program.cpp
    compiler_statement.cpp
    compiler_variable.cpp
    db.cpp
    json.cpp
    lexer.cpp
    message.cpp
    node.cpp
    node_attribute.cpp
    node_compare.cpp
    node_convert.cpp
    node_display.cpp
    node_flag.cpp
    node_lock.cpp
    node_operator.cpp
    node_param.cpp
    node_tree.cpp
    node_type.cpp
    node_value.cpp
    optimizer.cpp
    optimizer_matches.cpp
    optimizer_optimize.cpp
    optimizer_tables.cpp
    options.cpp
    os_raii.cpp
    parser.cpp
    parser_class.cpp
    parser_directive.cpp
    parser_expression.cpp
    parser_function.cpp
    parser_numeric_type.cpp
    parser_package.cpp
    parser_pragma.cpp
    parser_program.cpp
    parser_statement.cpp
    parser_variable.cpp
    position.cpp
    rc.cpp
    stream.cpp
    string.cpp
    version.cpp
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
