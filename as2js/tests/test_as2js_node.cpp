/* test_as2js_node.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "test_as2js_node.h"
#include    "test_as2js_main.h"

#include    "as2js/node.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsNodeUnitTests );


namespace
{

struct node_type_info_t
{
    as2js::Node::node_t     f_type;
    char const *            f_name;
    uint64_t                f_flags;
};


uint64_t const              TEST_NODE_IS_NUMBER     = 0x0000000000000001;
uint64_t const              TEST_NODE_IS_NAN        = 0x0000000000000002;
uint64_t const              TEST_NODE_IS_INT64      = 0x0000000000000004;
uint64_t const              TEST_NODE_IS_FLOAT64    = 0x0000000000000008;
uint64_t const              TEST_NODE_IS_BOOLEAN    = 0x0000000000000010;
uint64_t const              TEST_NODE_IS_TRUE       = 0x0000000000000020;
uint64_t const              TEST_NODE_IS_FALSE      = 0x0000000000000040;
uint64_t const              TEST_NODE_IS_STRING     = 0x0000000000000080;
uint64_t const              TEST_NODE_IS_UNDEFINED  = 0x0000000000000100;
uint64_t const              TEST_NODE_IS_NULL       = 0x0000000000000200;
uint64_t const              TEST_NODE_IS_IDENTIFIER = 0x0000000000000400;
uint64_t const              TEST_NODE_ACCEPT_STRING = 0x0000000000000800;


// index from 0 to g_node_types_size - 1 to go through all the valid
// node types
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
node_type_info_t g_node_types[] =
{
    {
        .f_type = as2js::Node::node_t::NODE_EOF,
        .f_name = "EOF",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_UNKNOWN,
        .f_name = "UNKNOWN",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ADD,
        .f_name = "ADD",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_BITWISE_AND,
        .f_name = "BITWISE_AND",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_BITWISE_NOT,
        .f_name = "BITWISE_NOT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT,
        .f_name = "ASSIGNMENT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_BITWISE_OR,
        .f_name = "BITWISE_OR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_BITWISE_XOR,
        .f_name = "BITWISE_XOR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CLOSE_CURVLY_BRACKET,
        .f_name = "CLOSE_CURVLY_BRACKET",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        .f_name = "CLOSE_PARENTHESIS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET,
        .f_name = "CLOSE_SQUARE_BRACKET",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_COLON,
        .f_name = "COLON",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_COMMA,
        .f_name = "COMMA",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CONDITIONAL,
        .f_name = "CONDITIONAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_DIVIDE,
        .f_name = "DIVIDE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_GREATER,
        .f_name = "GREATER",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LESS,
        .f_name = "LESS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LOGICAL_NOT,
        .f_name = "LOGICAL_NOT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_MODULO,
        .f_name = "MODULO",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_MULTIPLY,
        .f_name = "MULTIPLY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_OPEN_CURVLY_BRACKET,
        .f_name = "OPEN_CURVLY_BRACKET",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        .f_name = "OPEN_PARENTHESIS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET,
        .f_name = "OPEN_SQUARE_BRACKET",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_MEMBER,
        .f_name = "MEMBER",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SEMICOLON,
        .f_name = "SEMICOLON",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SUBTRACT,
        .f_name = "SUBTRACT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ARRAY,
        .f_name = "ARRAY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ARRAY_LITERAL,
        .f_name = "ARRAY_LITERAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_AS,
        .f_name = "AS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_ADD,
        .f_name = "ASSIGNMENT_ADD",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND,
        .f_name = "ASSIGNMENT_BITWISE_AND",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,
        .f_name = "ASSIGNMENT_BITWISE_OR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR,
        .f_name = "ASSIGNMENT_BITWISE_XOR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE,
        .f_name = "ASSIGNMENT_DIVIDE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND,
        .f_name = "ASSIGNMENT_LOGICAL_AND",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR,
        .f_name = "ASSIGNMENT_LOGICAL_OR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR,
        .f_name = "ASSIGNMENT_LOGICAL_XOR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM,
        .f_name = "ASSIGNMENT_MAXIMUM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM,
        .f_name = "ASSIGNMENT_MINIMUM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_MODULO,
        .f_name = "ASSIGNMENT_MODULO",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY,
        .f_name = "ASSIGNMENT_MULTIPLY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_POWER,
        .f_name = "ASSIGNMENT_POWER",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT,
        .f_name = "ASSIGNMENT_ROTATE_LEFT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,
        .f_name = "ASSIGNMENT_ROTATE_RIGHT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT,
        .f_name = "ASSIGNMENT_SHIFT_LEFT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,
        .f_name = "ASSIGNMENT_SHIFT_RIGHT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED,
        .f_name = "ASSIGNMENT_SHIFT_RIGHT_UNSIGNED",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT,
        .f_name = "ASSIGNMENT_SUBTRACT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ATTRIBUTES,
        .f_name = "ATTRIBUTES",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_AUTO,
        .f_name = "AUTO",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_BREAK,
        .f_name = "BREAK",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_CALL,
        .f_name = "CALL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CASE,
        .f_name = "CASE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CATCH,
        .f_name = "CATCH",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CLASS,
        .f_name = "CLASS",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_CONST,
        .f_name = "CONST",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_CONTINUE,
        .f_name = "CONTINUE",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_DEBUGGER,
        .f_name = "DEBUGGER",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_DECREMENT,
        .f_name = "DECREMENT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_DEFAULT,
        .f_name = "DEFAULT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_DELETE,
        .f_name = "DELETE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_DIRECTIVE_LIST,
        .f_name = "DIRECTIVE_LIST",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_DO,
        .f_name = "DO",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ELSE,
        .f_name = "ELSE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_EMPTY,
        .f_name = "EMPTY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ENTRY,
        .f_name = "ENTRY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ENUM,
        .f_name = "ENUM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_EQUAL,
        .f_name = "EQUAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_EXCLUDE,
        .f_name = "EXCLUDE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_EXTENDS,
        .f_name = "EXTENDS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_FALSE,
        .f_name = "FALSE",
        .f_flags = TEST_NODE_IS_BOOLEAN | TEST_NODE_IS_FALSE
    },
    {
        .f_type = as2js::Node::node_t::NODE_FINALLY,
        .f_name = "FINALLY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_FLOAT64,
        .f_name = "FLOAT64",
        .f_flags = TEST_NODE_IS_NUMBER | TEST_NODE_IS_FLOAT64
    },
    {
        .f_type = as2js::Node::node_t::NODE_FOR,
        .f_name = "FOR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_FUNCTION,
        .f_name = "FUNCTION",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_GOTO,
        .f_name = "GOTO",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_GREATER_EQUAL,
        .f_name = "GREATER_EQUAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_IDENTIFIER,
        .f_name = "IDENTIFIER",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_IS_IDENTIFIER
    },
    {
        .f_type = as2js::Node::node_t::NODE_IF,
        .f_name = "IF",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_IMPLEMENTS,
        .f_name = "IMPLEMENTS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_IMPORT,
        .f_name = "IMPORT",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_IN,
        .f_name = "IN",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_INCLUDE,
        .f_name = "INCLUDE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_INCREMENT,
        .f_name = "INCREMENT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_INSTANCEOF,
        .f_name = "INSTANCEOF",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_INT64,
        .f_name = "INT64",
        .f_flags = TEST_NODE_IS_NUMBER | TEST_NODE_IS_INT64
    },
    {
        .f_type = as2js::Node::node_t::NODE_INTERFACE,
        .f_name = "INTERFACE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_IS,
        .f_name = "IS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LABEL,
        .f_name = "LABEL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LESS_EQUAL,
        .f_name = "LESS_EQUAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LIST,
        .f_name = "LIST",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LOGICAL_AND,
        .f_name = "LOGICAL_AND",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LOGICAL_OR,
        .f_name = "LOGICAL_OR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_LOGICAL_XOR,
        .f_name = "LOGICAL_XOR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_MATCH,
        .f_name = "MATCH",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_MAXIMUM,
        .f_name = "MAXIMUM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_MINIMUM,
        .f_name = "MINIMUM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_NAME,
        .f_name = "NAME",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_NAMESPACE,
        .f_name = "NAMESPACE",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_NEW,
        .f_name = "NEW",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_NOT_EQUAL,
        .f_name = "NOT_EQUAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_NULL,
        .f_name = "NULL",
        .f_flags = TEST_NODE_IS_NULL
    },
    {
        .f_type = as2js::Node::node_t::NODE_OBJECT_LITERAL,
        .f_name = "OBJECT_LITERAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_PACKAGE,
        .f_name = "PACKAGE",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_PARAM,
        .f_name = "PARAM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_PARAMETERS,
        .f_name = "PARAMETERS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_PARAM_MATCH,
        .f_name = "PARAM_MATCH",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_POST_DECREMENT,
        .f_name = "POST_DECREMENT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_POST_INCREMENT,
        .f_name = "POST_INCREMENT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_POWER,
        .f_name = "POWER",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_PRIVATE,
        .f_name = "PRIVATE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_PROGRAM,
        .f_name = "PROGRAM",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_PUBLIC,
        .f_name = "PUBLIC",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_RANGE,
        .f_name = "RANGE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_REGULAR_EXPRESSION,
        .f_name = "REGULAR_EXPRESSION",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_REST,
        .f_name = "REST",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_RETURN,
        .f_name = "RETURN",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ROOT,
        .f_name = "ROOT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ROTATE_LEFT,
        .f_name = "ROTATE_LEFT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_ROTATE_RIGHT,
        .f_name = "ROTATE_RIGHT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SCOPE,
        .f_name = "SCOPE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SET,
        .f_name = "SET",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SHIFT_LEFT,
        .f_name = "SHIFT_LEFT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SHIFT_RIGHT,
        .f_name = "SHIFT_RIGHT",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED,
        .f_name = "SHIFT_RIGHT_UNSIGNED",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_STRICTLY_EQUAL,
        .f_name = "STRICTLY_EQUAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL,
        .f_name = "STRICTLY_NOT_EQUAL",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_STRING,
        .f_name = "STRING",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_IS_STRING | TEST_NODE_ACCEPT_STRING
    },
    {
        .f_type = as2js::Node::node_t::NODE_SUPER,
        .f_name = "SUPER",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_SWITCH,
        .f_name = "SWITCH",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_THIS,
        .f_name = "THIS",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_THROW,
        .f_name = "THROW",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_TRUE,
        .f_name = "TRUE",
        .f_flags = TEST_NODE_IS_BOOLEAN | TEST_NODE_IS_TRUE
    },
    {
        .f_type = as2js::Node::node_t::NODE_TRY,
        .f_name = "TRY",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_TYPE,
        .f_name = "TYPE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_TYPEOF,
        .f_name = "TYPEOF",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_UNDEFINED,
        .f_name = "UNDEFINED",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_IS_UNDEFINED
    },
    {
        .f_type = as2js::Node::node_t::NODE_USE,
        .f_name = "USE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_VAR,
        .f_name = "VAR",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_VARIABLE,
        .f_name = "VARIABLE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_VAR_ATTRIBUTES,
        .f_name = "VAR_ATTRIBUTES",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_VIDENTIFIER,
        .f_name = "VIDENTIFIER",
        .f_flags = TEST_NODE_IS_NAN | TEST_NODE_IS_IDENTIFIER
    },
    {
        .f_type = as2js::Node::node_t::NODE_VOID,
        .f_name = "VOID",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_WHILE,
        .f_name = "WHILE",
        .f_flags = TEST_NODE_IS_NAN
    },
    {
        .f_type = as2js::Node::node_t::NODE_WITH,
        .f_name = "WITH",
        .f_flags = TEST_NODE_IS_NAN
    }
};
#pragma GCC diagnostic pop
size_t const g_node_types_size(sizeof(g_node_types) / sizeof(g_node_types[0]));

}



void As2JsNodeUnitTests::test_type()
{
    // test all the different types available
    std::bitset<static_cast<size_t>(as2js::Node::node_t::NODE_max)> valid_types;
    for(size_t i(0); i < g_node_types_size; ++i)
    {
        // define the type
        as2js::Node::node_t node_type(g_node_types[i].f_type);

        if(static_cast<size_t>(node_type) > static_cast<size_t>(as2js::Node::node_t::NODE_max))
        {
            if(node_type != as2js::Node::node_t::NODE_EOF)
            {
                std::cerr << "Somehow a node type (" << static_cast<int>(node_type)
                          << ") is larger than the maximum allowed ("
                          << static_cast<int>(as2js::Node::node_t::NODE_max) << ")" << std::endl;
            }
        }
        else
        {
            valid_types[static_cast<size_t>(node_type)] = true;
        }

        // get the next type of node
        as2js::Node::pointer_t node(new as2js::Node(node_type));

        // check the type
        CPPUNIT_ASSERT(node->get_type() == node_type);

        // get the name
        char const *name(node->get_type_name());
        CPPUNIT_ASSERT(strcmp(name, g_node_types[i].f_name) == 0);

        // test functions determining general types
//std::cerr << "type = " << static_cast<int>(node_type) << " / " << name << "\n";
        CPPUNIT_ASSERT(node->is_number() == false || node->is_number() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_number() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NUMBER) == 0));

        // This NaN test is not sufficient for strings
        CPPUNIT_ASSERT(node->is_nan() == false || node->is_nan() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_nan() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NAN) == 0));

        CPPUNIT_ASSERT(node->is_int64() == false || node->is_int64() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_int64() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_INT64) == 0));

        CPPUNIT_ASSERT(node->is_float64() == false || node->is_float64() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_float64() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_FLOAT64) == 0));

        CPPUNIT_ASSERT(node->is_boolean() == false || node->is_boolean() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_boolean() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_BOOLEAN) == 0));

        CPPUNIT_ASSERT(node->is_true() == false || node->is_true() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_true() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_TRUE) == 0));

        CPPUNIT_ASSERT(node->is_false() == false || node->is_false() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_false() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_FALSE) == 0));

        CPPUNIT_ASSERT(node->is_string() == false || node->is_string() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_string() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_STRING) == 0));

        CPPUNIT_ASSERT(node->is_undefined() == false || node->is_undefined() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_undefined() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_UNDEFINED) == 0));

        CPPUNIT_ASSERT(node->is_null() == false || node->is_null() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_null() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NULL) == 0));

        CPPUNIT_ASSERT(node->is_identifier() == false || node->is_identifier() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_identifier() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_IDENTIFIER) == 0));

        if((g_node_types[i].f_flags & TEST_NODE_IS_BOOLEAN) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_boolean(), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_boolean(rand() & 1), as2js::exception_internal_error);
        }
        else if((g_node_types[i].f_flags & TEST_NODE_IS_TRUE) != 0)
        {
            CPPUNIT_ASSERT(node->get_boolean());
        }
        else
        {
            CPPUNIT_ASSERT(!node->get_boolean());
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_INT64) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_int64(), as2js::exception_internal_error);
            as2js::Int64 random(rand());
            CPPUNIT_ASSERT_THROW(node->set_int64(random), as2js::exception_internal_error);
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_FLOAT64) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_float64(), as2js::exception_internal_error);
            as2js::Float64 random(rand());
            CPPUNIT_ASSERT_THROW(node->set_float64(random), as2js::exception_internal_error);
        }

        // here we have a special case as "many" different nodes accept
        // a string to represent one thing or another
        if((g_node_types[i].f_flags & TEST_NODE_ACCEPT_STRING) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_string(), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_string("test"), as2js::exception_internal_error);
        }
        else
        {
            node->set_string("random test");
            CPPUNIT_ASSERT(node->get_string() == "random test");
        }
    }

    // make sure that special numbers are correctly caught
    for(size_t i(0); i < static_cast<size_t>(as2js::Node::node_t::NODE_max); ++i)
    {
        if(!valid_types[i])
        {
            as2js::Node::node_t node_type(static_cast<as2js::Node::node_t>(i));
            CPPUNIT_ASSERT_THROW(new as2js::Node(node_type), as2js::exception_incompatible_node_type);
        }
    }

    // test with completely random numbers too (outside of the
    // standard range of node types.)
    for(size_t i(0); i < 100; ++i)
    {
        int32_t j((rand() << 16) ^ rand());
        if(j < -1 || j >= static_cast<ssize_t>(as2js::Node::node_t::NODE_max))
        {
            as2js::Node::node_t node_type(static_cast<as2js::Node::node_t>(j));
            CPPUNIT_ASSERT_THROW(new as2js::Node(node_type), as2js::exception_incompatible_node_type);
        }
    }
}


void As2JsNodeUnitTests::test_conversions()
{
    // first test simple conversions
    for(size_t i(0); i < g_node_types_size; ++i)
    {
        // original type
        as2js::Node::node_t original_type(g_node_types[i].f_type);

        // all nodes can be converted to UNKNOWN
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            node->to_unknown();
        }

        // CALL can be convert to AS
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            if(original_type == as2js::Node::node_t::NODE_CALL)
            {
                // in this case it works
                CPPUNIT_ASSERT(node->to_as());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_AS);
            }
            else
            {
                // in this case it fails
                CPPUNIT_ASSERT(!node->to_as());
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }

        // test what would happen if we were to call to_boolean()
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            as2js::Node::node_t new_type(node->to_boolean_type_only());
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(new_type == as2js::Node::node_t::NODE_TRUE);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_UNDEFINED:
            case as2js::Node::node_t::NODE_INT64: // by default integers are set to zero
            case as2js::Node::node_t::NODE_FLOAT64: // by default floating points are set to zero
            case as2js::Node::node_t::NODE_STRING: // by default strings are empty
                CPPUNIT_ASSERT(new_type == as2js::Node::node_t::NODE_FALSE);
                break;

            default:
                CPPUNIT_ASSERT(new_type == as2js::Node::node_t::NODE_UNDEFINED);
                break;

            }
        }

        // a few nodes can be converted to a boolean value
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_boolean());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_TRUE);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_UNDEFINED:
            case as2js::Node::node_t::NODE_INT64: // by default integers are set to zero
            case as2js::Node::node_t::NODE_FLOAT64: // by default floating points are set to zero
            case as2js::Node::node_t::NODE_STRING: // by default strings are empty
                CPPUNIT_ASSERT(node->to_boolean());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FALSE);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_boolean());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a couple types of nodes can be converted to a CALL
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_ASSIGNMENT:
            case as2js::Node::node_t::NODE_MEMBER:
                CPPUNIT_ASSERT(node->to_call());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_CALL);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_call());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to an INT64
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_INT64: // no change
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                break;

            case as2js::Node::node_t::NODE_FLOAT64:
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_UNDEFINED:
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 0);
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 1);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to a FLOAT64
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_INT64: // no change
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                break;

            case as2js::Node::node_t::NODE_FLOAT64:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == 0.0);
#pragma GCC diagnostic pop
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == 1.0);
#pragma GCC diagnostic pop
                break;

            case as2js::Node::node_t::NODE_UNDEFINED:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                CPPUNIT_ASSERT(node->get_float64().is_NaN());
                break;

            default:
                CPPUNIT_ASSERT(!node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to a Number
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_INT64: // no change!
            case as2js::Node::node_t::NODE_FLOAT64: // no change!
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 0);
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 1);
                break;

            case as2js::Node::node_t::NODE_STRING: // empty strings represent 0 here
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == 0.0);
#pragma GCC diagnostic pop
                break;

            case as2js::Node::node_t::NODE_UNDEFINED:
//std::cerr << " . type = " << static_cast<int>(original_type) << " / " << node->get_type_name() << "\n";
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                CPPUNIT_ASSERT(node->get_float64().is_NaN());
                break;

            default:
                CPPUNIT_ASSERT(!node->to_number());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to a STRING
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_STRING:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                CPPUNIT_ASSERT(node->get_string() == "");
                break;

            case as2js::Node::node_t::NODE_INT64:
            case as2js::Node::node_t::NODE_FLOAT64:
                // by default numbers are zero; we'll have another test
                // to verify the conversion
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "0");
                break;

            case as2js::Node::node_t::NODE_FALSE:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "false");
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "true");
                break;

            case as2js::Node::node_t::NODE_NULL:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "null");
                break;

            case as2js::Node::node_t::NODE_UNDEFINED:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "undefined");
                break;

            case as2js::Node::node_t::NODE_IDENTIFIER: // the string remains the same
            //case as2js::Node::node_t::NODE_VIDENTIFIER: // should the VIDENTIFIER be supported too?
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_string());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // IDENTIFIER can be convert to VIDENTIFIER
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            if(original_type == as2js::Node::node_t::NODE_IDENTIFIER)
            {
                // in this case it works
                node->to_videntifier();
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_VIDENTIFIER);
            }
            else
            {
                // this one fails dramatically
                CPPUNIT_ASSERT_THROW(node->to_videntifier(), as2js::exception_internal_error);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }

        // VARIABLE can be convert to VAR_ATTRIBUTES
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            if(original_type == as2js::Node::node_t::NODE_VARIABLE)
            {
                // in this case it works
                node->to_var_attributes();
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_VAR_ATTRIBUTES);
            }
            else
            {
                // in this case it fails
                CPPUNIT_ASSERT_THROW(node->to_var_attributes(), as2js::exception_internal_error);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }
    }

    for(int i(0); i < 100; ++i)
    {
        // Integer to other types
        {
            as2js::Int64 j((static_cast<int64_t>(rand()) << 48)
                         ^ (static_cast<int64_t>(rand()) << 32)
                         ^ (static_cast<int64_t>(rand()) << 16)
                         ^ (static_cast<int64_t>(rand()) <<  0));

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                as2js::Float64 invalid;
                CPPUNIT_ASSERT_THROW(node->set_float64(invalid), as2js::exception_internal_error);
                CPPUNIT_ASSERT(node->to_int64());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_int64().get() == j.get());
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_number());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == j.get());
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                as2js::Node::node_t bool_type(node->to_boolean_type_only());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(bool_type == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_boolean());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_float64());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == static_cast<as2js::Float64::float64_type>(j.get()));
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_string());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == as2js::String(std::to_string(j.get())));
            }
        }

        // Floating point to other values
        {
            // generate a random 64 bit number
            float s1(rand() & 1 ? -1 : 1);
            float n1(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                      ^ (static_cast<int64_t>(rand()) << 32)
                                      ^ (static_cast<int64_t>(rand()) << 16)
                                      ^ (static_cast<int64_t>(rand()) <<  0)));
            float d1(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                      ^ (static_cast<int64_t>(rand()) << 32)
                                      ^ (static_cast<int64_t>(rand()) << 16)
                                      ^ (static_cast<int64_t>(rand()) <<  0)));
            float r(n1 / d1 * s1);
            as2js::Float64 j(r);

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_int64().get() == static_cast<as2js::Int64::int64_type>(j.get()));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == j.get());
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                as2js::Node::node_t bool_type(node->to_boolean_type_only());
                // probably always true here; we had false in the loop prior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(bool_type == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_boolean());
                // probably always true here; we had false in the loop prior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_type() == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
#pragma GCC diagnostic pop

                // also test the set_boolean() with valid values
                node->set_boolean(true);
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_TRUE);
                node->set_boolean(false);
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FALSE);
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == j.get());
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == as2js::String(std::to_string(j.get())));
            }
        }
    }

    // verify special floating point values
    { // NaN
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_NaN();
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_string());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(node->get_string() == "NaN");
    }
    { // +Infinity
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_infinity();
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_string());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(node->get_string() == "Infinity");
    }
    { // -Infinity
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_infinity();
        j.set(-j.get());
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_string());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(node->get_string() == "-Infinity");
    }
}



// vim: ts=4 sw=4 et
