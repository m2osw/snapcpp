/* test_as2js_lexer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

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

#include    "test_as2js_lexer.h"
#include    "test_as2js_main.h"

#include    "as2js/lexer.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"

// See http://icu-project.org/apiref/icu4c/index.html
#include <unicode/uchar.h>
//#include <unicode/cuchar> // once available in Linux...

#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsLexerUnitTests );

namespace
{

struct option_t
{
    as2js::Options::option_t    f_option;
};

enum check_value_t
{
    CHECK_VALUE_IGNORE,
    CHECK_VALUE_INTEGER,
    CHECK_VALUE_FLOATING_POINT,
    CHECK_VALUE_STRING,
    CHECK_VALUE_BOOLEAN
};

struct result_t
{
    // this token, with that value if options match
    as2js::Node::node_t     f_token;
    check_value_t           f_check_value;
        int64_t                 f_integer;
        double                  f_floating_point;
        char const *            f_string; // utf8
        bool                    f_boolean;
    option_t const *        f_options;
};

struct token_t
{
    char const *            f_input;
    result_t const *        f_results;
};

option_t const g_option_extended_statements[] =
{
    as2js::Options::option_t::OPTION_EXTENDED_STATEMENTS,
    as2js::Options::option_t::OPTION_max
};

option_t const g_option_extended_escape_sequences[] =
{
    as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES,
    as2js::Options::option_t::OPTION_max
};

option_t const g_option_binary[] =
{
    as2js::Options::option_t::OPTION_BINARY,
    as2js::Options::option_t::OPTION_max
};

option_t const g_option_octal[] =
{
    as2js::Options::option_t::OPTION_OCTAL,
    as2js::Options::option_t::OPTION_max
};

//option_t const g_option_extended_operators[] =
//{
//    as2js::Options::option_t::OPTION_EXTENDED_OPERATORS,
//    as2js::Options::option_t::OPTION_max
//};

result_t const g_result_test_a_string[] =
{
    {
        as2js::Node::node_t::NODE_STRING,
        CHECK_VALUE_STRING, 0, 0.0, "Test a String", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_escaped_characters[] =
{
    {
        as2js::Node::node_t::NODE_STRING,
        CHECK_VALUE_STRING, 0, 0.0, "Escaped characters: Backspace \b, Escape \033, Formfeed \f, Newline \n, Carriage Return \r, Horizontal Tab \t, Vertical Tab \v, Double Quote \", Single Quote ', Backslash \\", false,
        g_option_extended_escape_sequences
    },
    {
        as2js::Node::node_t::NODE_STRING,
        CHECK_VALUE_STRING, 0, 0.0, "Escaped characters: Backspace \b, Escape ?, Formfeed \f, Newline \n, Carriage Return \r, Horizontal Tab \t, Vertical Tab \v, Double Quote \", Single Quote ', Backslash \\", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_empty_string[] =
{
    {
        as2js::Node::node_t::NODE_STRING,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_regex[] =
{
    {
        as2js::Node::node_t::NODE_REGULAR_EXPRESSION,
        CHECK_VALUE_STRING, 0, 0.0, "/regex/abc", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_int64_1234[] =
{
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1234, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_int64_binary_1234[] =
{
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1234, 0.0, "", false,
        g_option_binary
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, -1, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_int64_octal_207[] =
{
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 207, 0.0, "", false,
        g_option_octal
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_float64_1_234[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 1.234, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_float64_3_14159[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 3.14159, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_float64__33[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 0.33, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_float64__330000[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 330000.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_add[] =
{
    {
        as2js::Node::node_t::NODE_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_bitwise_and[] =
{
    {
        as2js::Node::node_t::NODE_BITWISE_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_bitwise_not[] =
{
    {
        as2js::Node::node_t::NODE_BITWISE_NOT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_bitwise_or[] =
{
    {
        as2js::Node::node_t::NODE_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_bitwise_xor[] =
{
    {
        as2js::Node::node_t::NODE_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_close_curvly_bracket[] =
{
    {
        as2js::Node::node_t::NODE_CLOSE_CURVLY_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_close_parenthesis[] =
{
    {
        as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_close_square_bracket[] =
{
    {
        as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_colon[] =
{
    {
        as2js::Node::node_t::NODE_COLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_comma[] =
{
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_conditional[] =
{
    {
        as2js::Node::node_t::NODE_CONDITIONAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_divide[] =
{
    {
        as2js::Node::node_t::NODE_DIVIDE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_greater[] =
{
    {
        as2js::Node::node_t::NODE_GREATER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_less[] =
{
    {
        as2js::Node::node_t::NODE_LESS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_logical_not[] =
{
    {
        as2js::Node::node_t::NODE_LOGICAL_NOT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_modulo[] =
{
    {
        as2js::Node::node_t::NODE_MODULO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_multiply[] =
{
    {
        as2js::Node::node_t::NODE_MULTIPLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_open_curvly_bracket[] =
{
    {
        as2js::Node::node_t::NODE_OPEN_CURVLY_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_open_parenthesis[] =
{
    {
        as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_open_square_bracket[] =
{
    {
        as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_member[] =
{
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_semicolon[] =
{
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_subtract[] =
{
    {
        as2js::Node::node_t::NODE_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_shift_left[] =
{
    {
        as2js::Node::node_t::NODE_SHIFT_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_shift_left[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_less_equal[] =
{
    {
        as2js::Node::node_t::NODE_LESS_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_extended_not_equal[] =
{
    {
        as2js::Node::node_t::NODE_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_minimum[] =
{
    {
        as2js::Node::node_t::NODE_MINIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_minimum[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_rotate_left[] =
{
    {
        as2js::Node::node_t::NODE_ROTATE_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_rotate_left[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_shift_right[] =
{
    {
        as2js::Node::node_t::NODE_SHIFT_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_unsigned_shift_right[] =
{
    {
        as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_shift_right[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_unsigned_shift_right[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_compare[] =
{
    {
        as2js::Node::node_t::NODE_COMPARE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_greater_equal[] =
{
    {
        as2js::Node::node_t::NODE_GREATER_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_maximum[] =
{
    {
        as2js::Node::node_t::NODE_MAXIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_maximum[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_rotate_right[] =
{
    {
        as2js::Node::node_t::NODE_ROTATE_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_rotate_right[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_not_equal[] =
{
    {
        as2js::Node::node_t::NODE_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_strictly_not_equal[] =
{
    {
        as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_equal[] =
{
    {
        as2js::Node::node_t::NODE_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_strictly_equal[] =
{
    {
        as2js::Node::node_t::NODE_STRICTLY_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_extended_assignment[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_scope[] =
{
    {
        as2js::Node::node_t::NODE_SCOPE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_match[] =
{
    {
        as2js::Node::node_t::NODE_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_not_match[] =
{
    {
        as2js::Node::node_t::NODE_NOT_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_smart_match[] =
{
    {
        as2js::Node::node_t::NODE_SMART_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_add[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_increment[] =
{
    {
        as2js::Node::node_t::NODE_INCREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_subtract[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_decrement[] =
{
    {
        as2js::Node::node_t::NODE_DECREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_multiply[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_power[] =
{
    {
        as2js::Node::node_t::NODE_POWER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_power[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_POWER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_divide[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_modulo[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MODULO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_bitwise_and[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_logical_and[] =
{
    {
        as2js::Node::node_t::NODE_LOGICAL_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_logical_and[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_bitwise_xor[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_logical_xor[] =
{
    {
        as2js::Node::node_t::NODE_LOGICAL_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_logical_xor[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_bitwise_or[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_logical_or[] =
{
    {
        as2js::Node::node_t::NODE_LOGICAL_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_assignment_logical_or[] =
{
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_range[] =
{
    {
        as2js::Node::node_t::NODE_RANGE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_rest[] =
{
    {
        as2js::Node::node_t::NODE_REST,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_identifier_test_an_identifier[] =
{
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "Test_An_Identifier", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_abstract[] =
{
    {
        as2js::Node::node_t::NODE_ABSTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_as[] =
{
    {
        as2js::Node::node_t::NODE_AS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_boolean[] =
{
    {
        as2js::Node::node_t::NODE_BOOLEAN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_break[] =
{
    {
        as2js::Node::node_t::NODE_BREAK,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};


result_t const g_result_keyword_byte[] =
{
    {
        as2js::Node::node_t::NODE_BYTE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};
result_t const g_result_keyword_case[] =
{
    {
        as2js::Node::node_t::NODE_CASE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_catch[] =
{
    {
        as2js::Node::node_t::NODE_CATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_char[] =
{
    {
        as2js::Node::node_t::NODE_CHAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_class[] =
{
    {
        as2js::Node::node_t::NODE_CLASS,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_const[] =
{
    {
        as2js::Node::node_t::NODE_CONST,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_continue[] =
{
    {
        as2js::Node::node_t::NODE_CONTINUE,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_debugger[] =
{
    {
        as2js::Node::node_t::NODE_DEBUGGER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_default[] =
{
    {
        as2js::Node::node_t::NODE_DEFAULT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_delete[] =
{
    {
        as2js::Node::node_t::NODE_DELETE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_do[] =
{
    {
        as2js::Node::node_t::NODE_DO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_double[] =
{
    {
        as2js::Node::node_t::NODE_DOUBLE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_else[] =
{
    {
        as2js::Node::node_t::NODE_ELSE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_ensure[] =
{
    {
        as2js::Node::node_t::NODE_ENSURE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_enum[] =
{
    {
        as2js::Node::node_t::NODE_ENUM,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_export[] =
{
    {
        as2js::Node::node_t::NODE_EXPORT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_extends[] =
{
    {
        as2js::Node::node_t::NODE_EXTENDS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_false[] =
{
    {
        as2js::Node::node_t::NODE_FALSE,
        CHECK_VALUE_BOOLEAN, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_final[] =
{
    {
        as2js::Node::node_t::NODE_FINAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_finally[] =
{
    {
        as2js::Node::node_t::NODE_FINALLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_float[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_for[] =
{
    {
        as2js::Node::node_t::NODE_FOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_function[] =
{
    {
        as2js::Node::node_t::NODE_FUNCTION,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_goto[] =
{
    {
        as2js::Node::node_t::NODE_GOTO,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_if[] =
{
    {
        as2js::Node::node_t::NODE_IF,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_implements[] =
{
    {
        as2js::Node::node_t::NODE_IMPLEMENTS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_import[] =
{
    {
        as2js::Node::node_t::NODE_IMPORT,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_in[] =
{
    {
        as2js::Node::node_t::NODE_IN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_inline[] =
{
    {
        as2js::Node::node_t::NODE_INLINE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_instanceof[] =
{
    {
        as2js::Node::node_t::NODE_INSTANCEOF,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_interface[] =
{
    {
        as2js::Node::node_t::NODE_INTERFACE,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_invariant[] =
{
    {
        as2js::Node::node_t::NODE_INVARIANT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_is[] =
{
    {
        as2js::Node::node_t::NODE_IS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_infinity[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, std::numeric_limits<double>::infinity(), "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_long[] =
{
    {
        as2js::Node::node_t::NODE_LONG,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_namespace[] =
{
    {
        as2js::Node::node_t::NODE_NAMESPACE,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_new[] =
{
    {
        as2js::Node::node_t::NODE_NEW,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_null[] =
{
    {
        as2js::Node::node_t::NODE_NULL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_nan[] =
{
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, std::numeric_limits<double>::quiet_NaN(), "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_native[] =
{
    {
        as2js::Node::node_t::NODE_NATIVE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_package[] =
{
    {
        as2js::Node::node_t::NODE_PACKAGE,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_private[] =
{
    {
        as2js::Node::node_t::NODE_PRIVATE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_protected[] =
{
    {
        as2js::Node::node_t::NODE_PROTECTED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_public[] =
{
    {
        as2js::Node::node_t::NODE_PUBLIC,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_require[] =
{
    {
        as2js::Node::node_t::NODE_REQUIRE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_return[] =
{
    {
        as2js::Node::node_t::NODE_RETURN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_short[] =
{
    {
        as2js::Node::node_t::NODE_SHORT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_static[] =
{
    {
        as2js::Node::node_t::NODE_STATIC,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_super[] =
{
    {
        as2js::Node::node_t::NODE_SUPER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_switch[] =
{
    {
        as2js::Node::node_t::NODE_SWITCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_synchronized[] =
{
    {
        as2js::Node::node_t::NODE_SYNCHRONIZED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_then[] =
{
    {
        as2js::Node::node_t::NODE_THEN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_this[] =
{
    {
        as2js::Node::node_t::NODE_THIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_throw[] =
{
    {
        as2js::Node::node_t::NODE_THROW,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_throws[] =
{
    {
        as2js::Node::node_t::NODE_THROWS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_transient[] =
{
    {
        as2js::Node::node_t::NODE_TRANSIENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_true[] =
{
    {
        as2js::Node::node_t::NODE_TRUE,
        CHECK_VALUE_BOOLEAN, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_try[] =
{
    {
        as2js::Node::node_t::NODE_TRY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_typeof[] =
{
    {
        as2js::Node::node_t::NODE_TYPEOF,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_undefined[] =
{
    {
        as2js::Node::node_t::NODE_UNDEFINED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_use[] =
{
    {
        as2js::Node::node_t::NODE_USE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_var[] =
{
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_void[] =
{
    {
        as2js::Node::node_t::NODE_VOID,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_volatile[] =
{
    {
        as2js::Node::node_t::NODE_VOLATILE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_while[] =
{
    {
        as2js::Node::node_t::NODE_WHILE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_with[] =
{
    {
        as2js::Node::node_t::NODE_WITH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_yield[] =
{
    {
        as2js::Node::node_t::NODE_YIELD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_file[] =
{
    {
        // we use a StringInput which filename is set to "-"
        as2js::Node::node_t::NODE_STRING,
        CHECK_VALUE_STRING, 0, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

result_t const g_result_keyword_line[] =
{
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1, 0.0, "", true,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

token_t const g_tokens[] =
{
    /******************
     * LITERALS       *
     ******************/
    {
        "\"Test a String\"",
        g_result_test_a_string
    },
    {
        "'Test a String'",
        g_result_test_a_string
    },
    {
        "\"Escaped characters: Backspace \\b, Escape \\e, Formfeed \\f, Newline \\n, Carriage Return \\r, Horizontal Tab \\t, Vertical Tab \\v, Double Quote \\\", Single Quote \\', Backslash \\\\\"",
        g_result_escaped_characters
    },
    {
        "\"\"", // empty string
        g_result_empty_string
    },
    {
        "''", // empty string
        g_result_empty_string
    },
    {
        "`/regex/abc`", // out extension
        g_result_regex
    },
    {
        "/regex/abc", // normal JavaScript "ugly" regex
        g_result_regex
    },
    {
        "1234",
        g_result_int64_1234
    },
    {
        "0x4D2",
        g_result_int64_1234
    },
    {
        "0X4D2",
        g_result_int64_1234
    },
    {
        "0X00004d2",
        g_result_int64_1234
    },
    {
        "0b10011010010",
        g_result_int64_binary_1234
    },
    {
        "0b00010011010010",
        g_result_int64_binary_1234
    },
    {
        "0317",
        g_result_int64_octal_207
    },
    {
        "1.234",
        g_result_float64_1_234
    },
    {
        "314159.0e-5",
        g_result_float64_3_14159
    },
    {
        ".0000314159e+5",
        g_result_float64_3_14159
    },
    {
        "0.00314159e3",
        g_result_float64_3_14159
    },
    {
        "3141.59e-3",
        g_result_float64_3_14159
    },
    {
        ".33",
        g_result_float64__33
    },
    {
        "33e4",
        g_result_float64__330000
    },
    {
        "33e+4",
        g_result_float64__330000
    },
    {
        "330000000e-3",
        g_result_float64__330000
    },
    {
        "33.e4",
        g_result_float64__330000
    },
    {
        "33.e+4",
        g_result_float64__330000
    },
    {
        "330000000.e-3",
        g_result_float64__330000
    },
    {
        "\xE2\x88\x9E", // 0x221E -- INFINITY
        g_result_keyword_infinity
    },
    {
        "\xEF\xBF\xBD", // 0xFFFD -- REPLACEMENT CHARACTER
        g_result_keyword_nan
    },

    /******************
     * OPERATORS      *
     ******************/
    {
        "+",
        g_result_add
    },
    {
        "&",
        g_result_bitwise_and
    },
    {
        "~",
        g_result_bitwise_not
    },
    {
        "=",
        g_result_assignment
    },
    {
        "|",
        g_result_bitwise_or
    },
    {
        "^",
        g_result_bitwise_xor
    },
    {
        "}",
        g_result_close_curvly_bracket
    },
    {
        ")",
        g_result_close_parenthesis
    },
    {
        "]",
        g_result_close_square_bracket
    },
    {
        ":",
        g_result_colon
    },
    {
        ",",
        g_result_comma
    },
    {
        "?",
        g_result_conditional
    },
    {
        "/",
        g_result_divide
    },
    {
        ">",
        g_result_greater
    },
    {
        "<",
        g_result_less
    },
    {
        "!",
        g_result_logical_not
    },
    {
        "%",
        g_result_modulo
    },
    {
        "*",
        g_result_multiply
    },
    {
        "{",
        g_result_open_curvly_bracket
    },
    {
        "(",
        g_result_open_parenthesis
    },
    {
        "[",
        g_result_open_square_bracket
    },
    {
        ".",
        g_result_member
    },
    {
        ";",
        g_result_semicolon
    },
    {
        "-",
        g_result_subtract
    },
    {
        "<<",
        g_result_shift_left
    },
    {
        "<<=",
        g_result_assignment_shift_left
    },
    {
        "<=",
        g_result_less_equal
    },
    {
        "<>",
        g_result_extended_not_equal
    },
    {
        "<?",
        g_result_minimum
    },
    {
        "<?=",
        g_result_assignment_minimum
    },
    {
        "<%",
        g_result_rotate_left
    },
    {
        "<%=",
        g_result_assignment_rotate_left
    },
    {
        ">>",
        g_result_shift_right
    },
    {
        ">>>",
        g_result_unsigned_shift_right
    },
    {
        ">>=",
        g_result_assignment_shift_right
    },
    {
        ">>>=",
        g_result_assignment_unsigned_shift_right
    },
    {
        "<=>",
        g_result_compare
    },
    {
        ">=",
        g_result_greater_equal
    },
    {
        ">?",
        g_result_maximum
    },
    {
        ">?=",
        g_result_assignment_maximum
    },
    {
        ">%",
        g_result_rotate_right
    },
    {
        ">%=",
        g_result_assignment_rotate_right
    },
    {
        "!=",
        g_result_not_equal
    },
    {
        "!==",
        g_result_strictly_not_equal
    },
    {
        "==",
        g_result_equal
    },
    {
        "===",
        g_result_strictly_equal
    },
    {
        ":=",
        g_result_extended_assignment
    },
    {
        "::",
        g_result_scope
    },
    {
        "~=",
        g_result_match
    },
    {
        "!~",
        g_result_not_match
    },
    {
        "~~",
        g_result_smart_match
    },
    {
        "+=",
        g_result_assignment_add
    },
    {
        "++",
        g_result_increment
    },
    {
        "-=",
        g_result_assignment_subtract
    },
    {
        "--",
        g_result_decrement
    },
    {
        "*=",
        g_result_assignment_multiply
    },
    {
        "**",
        g_result_power
    },
    {
        "**=",
        g_result_assignment_power
    },
    {
        "/=",
        g_result_assignment_divide
    },
    {
        "%=",
        g_result_assignment_modulo
    },
    {
        "&=",
        g_result_assignment_bitwise_and
    },
    {
        "&&",
        g_result_logical_and
    },
    {
        "&&=",
        g_result_assignment_logical_and
    },
    {
        "^=",
        g_result_assignment_bitwise_xor
    },
    {
        "^^",
        g_result_logical_xor
    },
    {
        "^^=",
        g_result_assignment_logical_xor
    },
    {
        "|=",
        g_result_assignment_bitwise_or
    },
    {
        "||",
        g_result_logical_or
    },
    {
        "||=",
        g_result_assignment_logical_or
    },
    {
        "..",
        g_result_range
    },
    {
        "...",
        g_result_rest
    },

    /**************************
     * IDENTIFIERS / KEYWORDS *
     **************************/
    {
        "Test_An_Identifier",
        g_result_identifier_test_an_identifier
    },
    {
        "abstract",
        g_result_keyword_abstract
    },
    {
        "as",
        g_result_keyword_as
    },
    {
        "boolean",
        g_result_keyword_boolean
    },
    {
        "break",
        g_result_keyword_break
    },
    {
        "byte",
        g_result_keyword_byte
    },
    {
        "case",
        g_result_keyword_case
    },
    {
        "catch",
        g_result_keyword_catch
    },
    {
        "char",
        g_result_keyword_char
    },
    {
        "class",
        g_result_keyword_class
    },
    {
        "const",
        g_result_keyword_const
    },
    {
        "continue",
        g_result_keyword_continue
    },
    {
        "debugger",
        g_result_keyword_debugger
    },
    {
        "default",
        g_result_keyword_default
    },
    {
        "delete",
        g_result_keyword_delete
    },
    {
        "do",
        g_result_keyword_do
    },
    {
        "double",
        g_result_keyword_double
    },
    {
        "else",
        g_result_keyword_else
    },
    {
        "ensure",
        g_result_keyword_ensure
    },
    {
        "enum",
        g_result_keyword_enum
    },
    {
        "export",
        g_result_keyword_export
    },
    {
        "extends",
        g_result_keyword_extends
    },
    {
        "false",
        g_result_keyword_false
    },
    {
        "final",
        g_result_keyword_final
    },
    {
        "finally",
        g_result_keyword_finally
    },
    {
        "float",
        g_result_keyword_float
    },
    {
        "for",
        g_result_keyword_for
    },
    {
        "function",
        g_result_keyword_function
    },
    {
        "goto",
        g_result_keyword_goto
    },
    {
        "if",
        g_result_keyword_if
    },
    {
        "implements",
        g_result_keyword_implements
    },
    {
        "import",
        g_result_keyword_import
    },
    {
        "in",
        g_result_keyword_in
    },
    {
        "inline",
        g_result_keyword_inline
    },
    {
        "instanceof",
        g_result_keyword_instanceof
    },
    {
        "interface",
        g_result_keyword_interface
    },
    {
        "invariant",
        g_result_keyword_invariant
    },
    {
        "is",
        g_result_keyword_is
    },
    {
        "Infinity",
        g_result_keyword_infinity
    },
    {
        "long",
        g_result_keyword_long
    },
    {
        "namespace",
        g_result_keyword_namespace
    },
    {
        "NaN",
        g_result_keyword_nan
    },
    {
        "native",
        g_result_keyword_native
    },
    {
        "new",
        g_result_keyword_new
    },
    {
        "null",
        g_result_keyword_null
    },
    {
        "package",
        g_result_keyword_package
    },
    {
        "private",
        g_result_keyword_private
    },
    {
        "protected",
        g_result_keyword_protected
    },
    {
        "public",
        g_result_keyword_public
    },
    {
        "require",
        g_result_keyword_require
    },
    {
        "return",
        g_result_keyword_return
    },
    {
        "short",
        g_result_keyword_short
    },
    {
        "static",
        g_result_keyword_static
    },
    {
        "super",
        g_result_keyword_super
    },
    {
        "switch",
        g_result_keyword_switch
    },
    {
        "synchronized",
        g_result_keyword_synchronized
    },
    {
        "then",
        g_result_keyword_then
    },
    {
        "this",
        g_result_keyword_this
    },
    {
        "throw",
        g_result_keyword_throw
    },
    {
        "throws",
        g_result_keyword_throws
    },
    {
        "transient",
        g_result_keyword_transient
    },
    {
        "true",
        g_result_keyword_true
    },
    {
        "try",
        g_result_keyword_try
    },
    {
        "typeof",
        g_result_keyword_typeof
    },
    {
        "undefined",
        g_result_keyword_undefined
    },
    {
        "use",
        g_result_keyword_use
    },
    {
        "var",
        g_result_keyword_var
    },
    {
        "void",
        g_result_keyword_void
    },
    {
        "volatile",
        g_result_keyword_volatile
    },
    {
        "while",
        g_result_keyword_while
    },
    {
        "with",
        g_result_keyword_with
    },
    {
        "yield",
        g_result_keyword_yield
    },
    {
        "__FILE__",
        g_result_keyword_file
    },
    {
        "__LINE__",
        g_result_keyword_line
    },
};
size_t const g_tokens_size(sizeof(g_tokens) / sizeof(g_tokens[0]));

as2js::Options::option_t g_options[] =
{
    as2js::Options::option_t::OPTION_ALLOW_WITH,
    as2js::Options::option_t::OPTION_BINARY,
    as2js::Options::option_t::OPTION_COVERAGE,
    as2js::Options::option_t::OPTION_DEBUG,
    as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES,
    as2js::Options::option_t::OPTION_EXTENDED_OPERATORS,
    as2js::Options::option_t::OPTION_EXTENDED_STATEMENTS,
    as2js::Options::option_t::OPTION_JSON,
    as2js::Options::option_t::OPTION_OCTAL,
    as2js::Options::option_t::OPTION_STRICT,
    as2js::Options::option_t::OPTION_TRACE,
    as2js::Options::option_t::OPTION_UNSAFE_MATH
};
size_t const g_options_size(sizeof(g_options) / sizeof(g_options[0]));


std::string to_octal_string(int32_t v)
{
    std::stringstream s;
    s << std::oct << v;
    return s.str();
}


std::string to_hex_string(int32_t v, int width)
{
    std::stringstream s;
    s << std::setfill('0') << std::setw(width) << std::hex << v;
    return s.str();
}


class test_callback : public as2js::MessageCallback
{
public:
    test_callback()
    {
        as2js::Message::set_message_callback(this);
        g_warning_count = as2js::Message::warning_count();
        g_error_count = as2js::Message::error_count();
    }

    ~test_callback()
    {
        // make sure the pointer gets reset!
        as2js::Message::set_message_callback(nullptr);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
    {
        CPPUNIT_ASSERT(!f_expected.empty());

//std::cerr<< "msg = " << pos.get_filename() << " / " << f_expected[0].f_pos.get_filename() << "\n";
//std::cerr<< "msg = " << message << " / " << f_expected[0].f_message << "\n";
//std::cerr<< "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
//std::cerr<< "error_code = " << static_cast<int>(error_code) << " / " << static_cast<int>(f_expected[0].f_error_code) << "\n";

        CPPUNIT_ASSERT(f_expected[0].f_call);
        CPPUNIT_ASSERT(message_level == f_expected[0].f_message_level);
        CPPUNIT_ASSERT(error_code == f_expected[0].f_error_code);
        CPPUNIT_ASSERT(pos.get_filename() == f_expected[0].f_pos.get_filename());
        CPPUNIT_ASSERT(pos.get_function() == f_expected[0].f_pos.get_function());
        CPPUNIT_ASSERT(pos.get_page() == f_expected[0].f_pos.get_page());
        CPPUNIT_ASSERT(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
        CPPUNIT_ASSERT(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
        CPPUNIT_ASSERT(pos.get_line() == f_expected[0].f_pos.get_line());
        CPPUNIT_ASSERT(message == f_expected[0].f_message);

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CPPUNIT_ASSERT(g_warning_count == as2js::Message::warning_count());
        }

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
        || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::Message::error_count() << "\n";
            CPPUNIT_ASSERT(g_error_count == as2js::Message::error_count());
        }

        f_expected.erase(f_expected.begin());
    }

    void got_called()
    {
        CPPUNIT_ASSERT(f_expected.empty());
    }

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::Position             f_pos;
        std::string                 f_message; // UTF-8 string
    };

    std::vector<expected_t>     f_expected;

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;


}
// no name namespace




void As2JsLexerUnitTests::test_invalid_pointers()
{
    // valid input, but not options
    {
        as2js::String str("program");
        as2js::Input::pointer_t input(new as2js::StringInput(str));
        CPPUNIT_ASSERT_THROW(new as2js::Lexer(input, nullptr), as2js::exception_invalid_data);
    }

    // valid options, but not input
    {
        as2js::Options::pointer_t options(new as2js::Options);
        CPPUNIT_ASSERT_THROW(new as2js::Lexer(nullptr, options), as2js::exception_invalid_data);
    }

    // both input and options are invalid
    CPPUNIT_ASSERT_THROW(new as2js::Lexer(nullptr, nullptr), as2js::exception_invalid_data);
}



void As2JsLexerUnitTests::test_tokens()
{
    for(size_t idx(0); idx < g_tokens_size; ++idx)
    {
        if((idx % 5) == 0)
        {
            std::cout << "." << std::flush;
        }

        // this represents 2^(# of options) which right now is 2048
        for(size_t opt(0); opt < (1 << g_options_size); ++opt)
        {
            as2js::String str;
            str.from_utf8(g_tokens[idx].f_input);
            as2js::Input::pointer_t input(new as2js::StringInput(str));
            std::map<as2js::Options::option_t, bool> option_map;

            as2js::Options::pointer_t options(new as2js::Options);
            for(size_t o(0); o < g_options_size; ++o)
            {
                as2js::Options::option_t option(g_options[o]);
                option_map[option] = (opt & (1 << o)) != 0;
                if(option_map[option])
                {
                    options->set_option(g_options[o], 1);
                }
            }

            as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
            CPPUNIT_ASSERT(lexer->get_input() == input);
            as2js::Node::pointer_t token(lexer->get_next_token());
            // select the result depending on the options currently selected
            for(result_t const *results(g_tokens[idx].f_results);; ++results)
            {
                // if this assert fails then the test data has a problem
                // (i.e. no entry matched.)
                CPPUNIT_ASSERT(results->f_token != as2js::Node::node_t::NODE_UNKNOWN);

                bool found(true);

                // a nullptr means we match
                if(results->f_options != nullptr)
                {
                    for(option_t const *required_options(results->f_options);
                                        required_options->f_option != as2js::Options::option_t::OPTION_max;
                                        ++required_options)
                    {
                        if(!option_map[required_options->f_option])
                        {
                            // flag was not set so that result is not a
                            // match; ignore
                            found = false;
                            break;
                        }
                    }
                }
                if(found)
                {
//std::cerr << "\n" << opt << " @ Working on " << *token << " -> from input: ["
//                << g_tokens[idx].f_input << "]\n";;

                    // got a match of all the special options or the entry
                    // with a nullptr was reached, use this entry to test
                    // the result validity
                    CPPUNIT_ASSERT(token->get_type() == results->f_token);

                    // no children
                    CPPUNIT_ASSERT(token->get_children_size() == 0);

                    // no links
                    CPPUNIT_ASSERT(!token->get_instance());
                    CPPUNIT_ASSERT(!token->get_type_node());
                    CPPUNIT_ASSERT(!token->get_attribute_node());
                    CPPUNIT_ASSERT(!token->get_goto_exit());
                    CPPUNIT_ASSERT(!token->get_goto_enter());

                    // no variables
                    CPPUNIT_ASSERT(token->get_variable_size() == 0);

                    // no parent
                    CPPUNIT_ASSERT(!token->get_parent());

                    // no parameters
                    CPPUNIT_ASSERT(token->get_param_size() == 0);

                    // not locked
                    CPPUNIT_ASSERT(!token->is_locked());

                    // default switch operator
                    if(token->get_type() == as2js::Node::node_t::NODE_SWITCH)
                    {
                        CPPUNIT_ASSERT(token->get_switch_operator() == as2js::Node::node_t::NODE_UNKNOWN);
                    }

                    // no flags
                    // TODO: we need to know whether the flag is supported by the current node type
                    //for(as2js::Node::flag_t flag(as2js::Node::flag_t::NODE_CATCH_FLAG_TYPED);
                    //           flag < as2js::Node::flag_t::NODE_FLAG_max;
                    //           flag = static_cast<as2js::Node::flag_t>(static_cast<int>(flag) + 1))
                    //{
                    //    CPPUNIT_ASSERT(!token->get_flag(flag));
                    //}

                    // no attributes
                    if(token->get_type() != as2js::Node::node_t::NODE_PROGRAM)
                    {
                        for(as2js::Node::attribute_t attr(as2js::Node::attribute_t::NODE_ATTR_PUBLIC);
                                        attr < as2js::Node::attribute_t::NODE_ATTR_max;
                                        attr = static_cast<as2js::Node::attribute_t>(static_cast<int>(attr) + 1))
                        {
                            switch(attr)
                            {
                            case as2js::Node::attribute_t::NODE_ATTR_TYPE:
                                switch(token->get_type())
                                {
                                case as2js::Node::node_t::NODE_ADD:
                                case as2js::Node::node_t::NODE_ARRAY:
                                case as2js::Node::node_t::NODE_ARRAY_LITERAL:
                                case as2js::Node::node_t::NODE_AS:
                                case as2js::Node::node_t::NODE_ASSIGNMENT:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_ADD:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_MODULO:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_POWER:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                                case as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
                                case as2js::Node::node_t::NODE_BITWISE_AND:
                                case as2js::Node::node_t::NODE_BITWISE_NOT:
                                case as2js::Node::node_t::NODE_BITWISE_OR:
                                case as2js::Node::node_t::NODE_BITWISE_XOR:
                                case as2js::Node::node_t::NODE_CALL:
                                case as2js::Node::node_t::NODE_CONDITIONAL:
                                case as2js::Node::node_t::NODE_DECREMENT:
                                case as2js::Node::node_t::NODE_DELETE:
                                case as2js::Node::node_t::NODE_DIVIDE:
                                case as2js::Node::node_t::NODE_EQUAL:
                                case as2js::Node::node_t::NODE_FALSE:
                                case as2js::Node::node_t::NODE_FLOAT64:
                                case as2js::Node::node_t::NODE_FUNCTION:
                                case as2js::Node::node_t::NODE_GREATER:
                                case as2js::Node::node_t::NODE_GREATER_EQUAL:
                                case as2js::Node::node_t::NODE_IDENTIFIER:
                                case as2js::Node::node_t::NODE_IN:
                                case as2js::Node::node_t::NODE_INCREMENT:
                                case as2js::Node::node_t::NODE_INSTANCEOF:
                                case as2js::Node::node_t::NODE_INT64:
                                case as2js::Node::node_t::NODE_IS:
                                case as2js::Node::node_t::NODE_LESS:
                                case as2js::Node::node_t::NODE_LESS_EQUAL:
                                case as2js::Node::node_t::NODE_LIST:
                                case as2js::Node::node_t::NODE_LOGICAL_AND:
                                case as2js::Node::node_t::NODE_LOGICAL_NOT:
                                case as2js::Node::node_t::NODE_LOGICAL_OR:
                                case as2js::Node::node_t::NODE_LOGICAL_XOR:
                                case as2js::Node::node_t::NODE_MATCH:
                                case as2js::Node::node_t::NODE_MAXIMUM:
                                case as2js::Node::node_t::NODE_MEMBER:
                                case as2js::Node::node_t::NODE_MINIMUM:
                                case as2js::Node::node_t::NODE_MODULO:
                                case as2js::Node::node_t::NODE_MULTIPLY:
                                case as2js::Node::node_t::NODE_NAME:
                                case as2js::Node::node_t::NODE_NEW:
                                case as2js::Node::node_t::NODE_NOT_EQUAL:
                                case as2js::Node::node_t::NODE_NULL:
                                case as2js::Node::node_t::NODE_OBJECT_LITERAL:
                                case as2js::Node::node_t::NODE_POST_DECREMENT:
                                case as2js::Node::node_t::NODE_POST_INCREMENT:
                                case as2js::Node::node_t::NODE_POWER:
                                case as2js::Node::node_t::NODE_PRIVATE:
                                case as2js::Node::node_t::NODE_PUBLIC:
                                case as2js::Node::node_t::NODE_RANGE:
                                case as2js::Node::node_t::NODE_ROTATE_LEFT:
                                case as2js::Node::node_t::NODE_ROTATE_RIGHT:
                                case as2js::Node::node_t::NODE_SCOPE:
                                case as2js::Node::node_t::NODE_SHIFT_LEFT:
                                case as2js::Node::node_t::NODE_SHIFT_RIGHT:
                                case as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                                case as2js::Node::node_t::NODE_STRICTLY_EQUAL:
                                case as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL:
                                case as2js::Node::node_t::NODE_STRING:
                                case as2js::Node::node_t::NODE_SUBTRACT:
                                case as2js::Node::node_t::NODE_SUPER:
                                case as2js::Node::node_t::NODE_THIS:
                                case as2js::Node::node_t::NODE_TRUE:
                                case as2js::Node::node_t::NODE_TYPEOF:
                                case as2js::Node::node_t::NODE_UNDEFINED:
                                case as2js::Node::node_t::NODE_VIDENTIFIER:
                                case as2js::Node::node_t::NODE_VOID:
                                    CPPUNIT_ASSERT(!token->get_attribute(attr));
                                    break;

                                default:
                                    // any other type and you get an exception
                                    CPPUNIT_ASSERT_THROW(!token->get_attribute(attr), as2js::exception_internal_error);
                                    break;

                                }
                                break;

                            default:
                                CPPUNIT_ASSERT(!token->get_attribute(attr));
                                break;

                            }
                        }
                    }

                    if(results->f_check_value == CHECK_VALUE_INTEGER)
                    {
//std::cerr << "int " << token->get_int64().get() << " vs " << results->f_integer;
                        CPPUNIT_ASSERT(token->get_int64().get() == results->f_integer);
                    }
                    else
                    {
                        CPPUNIT_ASSERT_THROW(token->get_int64().get() == results->f_integer, as2js::exception_internal_error);
                    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(results->f_check_value == CHECK_VALUE_FLOATING_POINT)
                    {
                        if(std::isnan(results->f_floating_point))
                        {
                            CPPUNIT_ASSERT(token->get_float64().is_NaN());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(token->get_float64().get() == results->f_floating_point);
                        }
                    }
                    else
                    {
                        CPPUNIT_ASSERT_THROW(token->get_float64().get() == results->f_integer, as2js::exception_internal_error);
                    }
#pragma GCC diagnostic pop

                    if(results->f_check_value == CHECK_VALUE_STRING)
                    {
//std::cerr << "  --> [" << token->get_string() << "]\n";
                        CPPUNIT_ASSERT(token->get_string() == results->f_string);
                    }
                    else
                    {
                        CPPUNIT_ASSERT_THROW(token->get_string() == results->f_string, as2js::exception_internal_error);
                    }

                    if(results->f_check_value == CHECK_VALUE_BOOLEAN)
                    {
                        CPPUNIT_ASSERT(token->get_boolean() == results->f_boolean);
                    }
                    else
                    {
                        CPPUNIT_ASSERT_THROW(token->get_boolean() == results->f_boolean, as2js::exception_internal_error);
                    }

                    // exit the result loop, only one result is
                    // expected to match
                    break;
                }
            }
        }
    }
}


void As2JsLexerUnitTests::test_valid_strings()
{
    // we have a few things to check in strings:
    //
    //    quotes are ' or " -- tested in test_tokens()
    //
    //    characters can be escaped with \, unknown backslashes
    //    sequences must generate errors -- known letter sequences tested
    //    in test_tokens(); those with errors are tested in the next
    //    test below
    //
    //    strings can be continuated on multiple lines
    //

    for(int idx(0); idx < 10; ++idx)
    {
        as2js::String str;
        char quote(rand() & 1 ? '"' : '\'');
        str += quote;
        str += '\\';
        str += '0';
        str += quote;
        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << "token = " << *token << "\n";
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        as2js::String expected;
        expected += '\0';
        CPPUNIT_ASSERT(token->get_string() == expected);
        token = lexer->get_next_token();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);
    }

    // all valid escape sequences, with Octal, Hexa (x), Basic Unicode (u),
    // and Extended Unicode (U)
    for(as2js::as_char_t c(0); c < 0x110000; ++c)
    {
        if(c % 50000 == 0)
        {
            std::cout << "." << std::flush;
        }
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            // avoid surrogate by themselves
            continue;
        }

        char quote(rand() & 1 ? '"' : '\'');
        if(c < 0x100)
        {
            // for octal we already test with/without the option so no need here
            {
                as2js::String str;
                str += "// comment with ";
                switch(c)
                {
                case '\r':
                case '\n':
                case 0x2028:
                case 0x2029:
                    str += '?'; // terminators end a comment in this case
                    break;

                default:
                    str += c;
                    break;

                }
                str += " character!";
                switch(rand() % 5)
                {
                case 0:
                    str += '\r';
                    break;

                case 1:
                    str += '\n';
                    break;

                case 2:
                    str += '\r';
                    str += '\n';
                    break;

                case 3:
                    str += 0x2028;
                    break;

                case 4:
                    str += 0x2029;
                    break;

                }
                str += quote;
                str += '\\';
                str += to_octal_string(c);
                str += quote;
                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                options->set_option(as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, 1);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << "token = " << *token << "\n";
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += c;
                CPPUNIT_ASSERT(token->get_string() == expected);
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);
            }

            {
                as2js::String str;
                str += quote;
                str += '\\';
                str += rand() & 1 ? 'x' : 'X';
                str += to_hex_string(c, 2);
                str += quote;
                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                options->set_option(as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, 1);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += c;
                CPPUNIT_ASSERT(token->get_string() == expected);
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);
            }
        }
        if(c < 0x10000)
        {
            as2js::String str;
            str += "/* long comment ";
            // make sure to include the character we are testing in
            // the string
            if(c == '\0')
            {
                // not too sure right now why '\0' does not work in a
                // comment...
                str += '^';
                str += '@';
            }
            else
            {
                str += c;
            }
            as2js::as_char_t previous(c);
            int line_length(rand() % 30 + 50);
            for(int k(0); k < 256; ++k)
            {
                if(k % line_length == line_length - 1)
                {
                    switch(rand() % 5)
                    {
                    case 0:
                        str += '\r';
                        break;

                    case 1:
                        str += '\n';
                        break;

                    case 2:
                        str += '\r';
                        str += '\n';
                        break;

                    case 3:
                        str += 0x2028;
                        break;

                    case 4:
                        str += 0x2029;
                        break;

                    }
                }
                as2js::as_char_t cc;
                do
                {
                    cc = ((rand() << 16) ^ rand()) & 0x1FFFFF;
                }
                while(cc > 0x10FFFF
                   || cc == '\0'
                   || (cc >= 0xD800 && c <= 0xDFFF)
                   || (cc == '/' && previous == '*'));
                str += cc;
            }
            str += "! */";
            str += 0x2028;
            str += quote;
            str += '\\';
            str += 'u';
            str += to_hex_string(c, 4);
            str += quote;
//std::cerr << " + string with comment: " << str << "\n";
            as2js::Input::pointer_t input(new as2js::StringInput(str));
            as2js::Options::pointer_t options(new as2js::Options);
            options->set_option(as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, 1);
            as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
            CPPUNIT_ASSERT(lexer->get_input() == input);
            as2js::Node::pointer_t token(lexer->get_next_token());
            CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
            CPPUNIT_ASSERT(token->get_children_size() == 0);
            as2js::String expected;
            expected += c;
            CPPUNIT_ASSERT(token->get_string() == expected);
            token = lexer->get_next_token();
            CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);
        }

        // if(c < 0x110000) -- all characters
        {
            as2js::String str;
            str += "/* long comment with multi-asterisks ";
            size_t const max_asterisk(rand() % 10 + 1);
            for(size_t a(0); a < max_asterisk; ++a)
            {
                str += '*';
            }
            str += '/';
            str += quote;
            str += '\\';
            str += 'U';
            str += to_hex_string(c, 8);
            str += quote;
            as2js::Input::pointer_t input(new as2js::StringInput(str));
            as2js::Options::pointer_t options(new as2js::Options);
            options->set_option(as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, 1);
            as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
            CPPUNIT_ASSERT(lexer->get_input() == input);
            as2js::Node::pointer_t token(lexer->get_next_token());
            CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
            CPPUNIT_ASSERT(token->get_children_size() == 0);
            as2js::String expected;
            expected += c;
            CPPUNIT_ASSERT(token->get_string() == expected);
            token = lexer->get_next_token();
            CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);
        }

        // jsut a few characters cannot really make it as is in a string,
        // everything else should work like a charm
        switch(c)
        {
        case '\0': // this should probably work but not at this time...
        case '\n':
        case '\r':
        case 0x2028:
        case 0x2029:
        case '\\': // already tested in the previous loop
            break;

        default:
            if(c != quote)
            {
                as2js::String str;
                str += quote;
                str += c;
                str += quote;
                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += c;
                CPPUNIT_ASSERT(token->get_string() == expected);
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);
            }
            break;

        }
    }

    // test a valid line terminator inside a string
    int tested_all(0);
    for(size_t idx(0); idx < 100 || tested_all != 0x1F; ++idx)
    {
        as2js::String str, expected;
        str += '\'';
        size_t const max_chars1(rand() % 10 + 2);
        for(size_t j(0); j < max_chars1; ++j)
        {
            as2js::as_char_t c((rand() % 26) + 'A');
            str += c;
            expected += c;
        }
        str += '\\';
        bool new_paragraph(false);
        switch(rand() % 5)
        {
        case 0:
            str += '\r';
            tested_all |= 0x01;
            break;

        case 1:
            str += '\r';
            str += '\n';
            tested_all |= 0x02;
            break;

        case 2:
            str += '\n';
            tested_all |= 0x04;
            break;

        case 3:
            str += 0x2028;
            tested_all |= 0x08;
            break;

        case 4:
            new_paragraph = true;
            str += 0x2029;
            tested_all |= 0x10;
            break;

        }
        size_t const max_chars2(rand() % 10 + 2);
        for(size_t j(0); j < max_chars2; ++j)
        {
            as2js::as_char_t c((rand() % 26) + 'A');
            str += c;
            expected += c;
        }
        str += '\'';
        str += '\n';

        // now see that it works as expected
        {
            as2js::Input::pointer_t input(new as2js::StringInput(str));
            as2js::Options::pointer_t options(new as2js::Options);
            as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
            CPPUNIT_ASSERT(lexer->get_input() == input);
            as2js::Node::pointer_t token(lexer->get_next_token());
            CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
            CPPUNIT_ASSERT(token->get_children_size() == 0);
            CPPUNIT_ASSERT(token->get_string() == expected);

            as2js::Position const& node_pos(token->get_position());
            CPPUNIT_ASSERT(node_pos.get_page() == 1);
            CPPUNIT_ASSERT(node_pos.get_page_line() == 1);
            CPPUNIT_ASSERT(node_pos.get_paragraph() == 1);
            CPPUNIT_ASSERT(node_pos.get_line() == 1);

            as2js::Position const& input_pos(input->get_position());
            CPPUNIT_ASSERT(input_pos.get_page() == 1);
            if(new_paragraph)
            {
                CPPUNIT_ASSERT(input_pos.get_page_line() == 1);
                CPPUNIT_ASSERT(input_pos.get_paragraph() == 2);
                CPPUNIT_ASSERT(input_pos.get_line() == 1);
            }
            else
            {
                CPPUNIT_ASSERT(input_pos.get_page_line() == 2);
                CPPUNIT_ASSERT(input_pos.get_paragraph() == 1);
                CPPUNIT_ASSERT(input_pos.get_line() == 2);
            }

            // create a new node which has to give us the same position
            // as the last node we were given
            as2js::Node::node_t new_node_type(static_cast<as2js::Node::node_t>(rand() % (static_cast<int>(as2js::Node::node_t::NODE_max) - static_cast<int>(as2js::Node::node_t::NODE_other) - 1) + static_cast<int>(as2js::Node::node_t::NODE_other) + 1));
//std::cerr << "new node type = " << static_cast<int>(new_node_type) << "\n";
            as2js::Node::pointer_t new_node(lexer->get_new_node(new_node_type));
            CPPUNIT_ASSERT(new_node->get_type() == new_node_type);
            as2js::Position const& new_node_pos(new_node->get_position());
            CPPUNIT_ASSERT(new_node_pos.get_page() == 1);
            CPPUNIT_ASSERT(new_node_pos.get_page_line() == 1);
            CPPUNIT_ASSERT(new_node_pos.get_paragraph() == 1);
            CPPUNIT_ASSERT(new_node_pos.get_line() == 1);

            // make sure there is nothing more after the string
            // (the \n is skipped silently)
            token = lexer->get_next_token();
            CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_EOF);

            as2js::Position const& final_pos(input->get_position());
            CPPUNIT_ASSERT(final_pos.get_page() == 1);
            if(new_paragraph)
            {
                CPPUNIT_ASSERT(input_pos.get_page_line() == 2);
                CPPUNIT_ASSERT(input_pos.get_paragraph() == 2);
                CPPUNIT_ASSERT(input_pos.get_line() == 2);
            }
            else
            {
                CPPUNIT_ASSERT(input_pos.get_page_line() == 3);
                CPPUNIT_ASSERT(input_pos.get_paragraph() == 1);
                CPPUNIT_ASSERT(input_pos.get_line() == 3);
            }
        }
    }
}


void As2JsLexerUnitTests::test_invalid_strings()
{
    // test unterminated strings first (quite special cases)
    {
        as2js::String str("\"unterminated"); // double quote

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNTERMINATED_STRING;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "the last string was not closed before the end of the input was reached";

        test_callback tc;
        tc.f_expected.push_back(expected);

        // if we do not turn on the OPTION_EXTENDED_ESCAPE_SEQUENCES then
        // we get an error with the \U... syntax
        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_string() == "unterminated");
    }
    {
        as2js::String str("'unterminated"); // single quote

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNTERMINATED_STRING;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "the last string was not closed before the end of the input was reached";

        test_callback tc;
        tc.f_expected.push_back(expected);

        // if we do not turn on the OPTION_EXTENDED_ESCAPE_SEQUENCES then
        // we get an error with the \U... syntax
        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_string() == "unterminated");
    }
    for(int idx(0); idx < 10; ++idx)
    {
        // unterminated if it includes a a newline
        as2js::String str;
        str += idx & 1 ? '"' : '\'';
        str += "unter";

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNTERMINATED_STRING;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");

        // terminator
        switch(idx / 2)
        {
        case 0:
            str += '\r';
            expected.f_pos.new_line();
            break;

        case 1:
            str += '\n';
            expected.f_pos.new_line();
            break;

        case 2:
            str += '\r';
            str += '\n';
            expected.f_pos.new_line();
            break;

        case 3:
            str += 0x2028;
            expected.f_pos.new_line();
            break;

        case 4:
            str += 0x2029;
            expected.f_pos.new_paragraph();
            break;

        }

        str += "minated";

        expected.f_message = "a string cannot include a line terminator";

        test_callback tc;
        tc.f_expected.push_back(expected);

        // if we do not turn on the OPTION_EXTENDED_ESCAPE_SEQUENCES then
        // we get an error with the \U... syntax
        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_string() == "unter");

        as2js::Node::pointer_t identifier(lexer->get_next_token());
        CPPUNIT_ASSERT(identifier->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
        CPPUNIT_ASSERT(identifier->get_children_size() == 0);
        CPPUNIT_ASSERT(identifier->get_string() == "minated");

        as2js::Node::pointer_t end(lexer->get_next_token());
        CPPUNIT_ASSERT(end->get_type() == as2js::Node::node_t::NODE_EOF);
    }

    // now test all the characters that are not acceptable right after
    // a blackslash (invalid escape sequences)
    for(as2js::as_char_t c(0); c < 0x110000; ++c)
    {
        if(c % 30000 == 0)
        {
            std::cout << "." << std::flush;
        }
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            // avoid surrogate by themselves
            continue;
        }
        switch(c)
        {
        case '0':
        case 'b':
        case 'e':
        case 'f':
        case 'n':
        case 'r':
        case 'u':
        case 't':
        case 'v':
        case 'x':
        case 'X':
        case '\'':
        case '"':
        case '\\':
        case '\r': // terminator within the string create "problems" in this test
        case '\n':
        case 0x2028:
        case 0x2029:
            // these are valid escape sequences
            break;

        default:
            {
                as2js::String str;
                str += '"';
                str += '\\';
                str += c;
                str += '8';
                str += '9';
                str += 'A';
                str += 'B';
                str += 'C';
                str += 'D';
                str += 'E';
                str += 'F';
                str += '"';

                test_callback::expected_t expected;
                expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
                expected.f_error_code = as2js::err_code_t::AS_ERR_UNKNOWN_ESCAPE_SEQUENCE;
                expected.f_pos.set_filename("unknown-file");
                expected.f_pos.set_function("unknown-func");
                if(c > ' ' && c < 0x7F)
                {
                    expected.f_message = "unknown escape letter '";
                    expected.f_message += static_cast<char>(c);
                    expected.f_message += "'";
                }
                else
                {
                    std::stringstream ss;
                    ss << "unknown escape letter '\\U" << std::hex << std::setw(8) << std::setfill('0') << static_cast<int32_t>(c) << "'";
                    expected.f_message = ss.str();
                }

                switch(c)
                {
                case '\f':
                    expected.f_pos.new_page();
                    break;

                // 0x2028 and 0x2029 cannot happen here since we caught them
                // earlier (see previous switch level)
                //case 0x2028:
                //    expected.f_pos.new_line();
                //    break;
                //
                //case 0x2029:
                //    expected.f_pos.new_paragraph();
                //    break;

                }
                test_callback tc;
                tc.f_expected.push_back(expected);

                // if we do not turn on the OPTION_EXTENDED_ESCAPE_SEQUENCES then
                // we get an error with the \U... syntax
                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
                tc.got_called();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                CPPUNIT_ASSERT(token->get_string() == "?89ABCDEF");

//std::cerr << std::hex << c << " -> " << *token << "\n";
            }
            break;

        }
    }
}


void As2JsLexerUnitTests::test_invalid_numbers()
{
    // 0x, 0X, 0b, 0B by themsevles are not valid numbers
    {
        as2js::String str;
        str += '0';
        str += 'x'; // lowercase

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "invalid hexadecimal number, at least one digit is required";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }
    {
        as2js::String str;
        str += '0';
        str += 'X'; // uppercase

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "invalid hexadecimal number, at least one digit is required";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }
    {
        as2js::String str;
        str += '0';
        str += 'b'; // lowercase

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "invalid binary number, at least one digit is required";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        options->set_option(as2js::Options::option_t::OPTION_BINARY, 1);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }
    {
        as2js::String str;
        str += '0';
        str += 'B'; // uppercase

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "invalid binary number, at least one digit is required";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        options->set_option(as2js::Options::option_t::OPTION_BINARY, 1);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }

    // numbers cannot be followed by letters
    // (a space is required)
    {
        as2js::String str("7pm");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after an integer";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }
    {
        as2js::String str("6em");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after an integer";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }
    {
        as2js::String str("3.5in");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after a floating point number";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_FLOAT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(token->get_float64().get() == -1.0);
#pragma GCC diagnostic pop
    }
    {
        as2js::String str("10.1em");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after a floating point number";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_FLOAT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(token->get_float64().get() == -1.0);
#pragma GCC diagnostic pop
    }
    {
        as2js::String str("9.1e+j");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after a floating point number";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_FLOAT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(token->get_float64().get() == -1.0);
#pragma GCC diagnostic pop
    }
    {
        as2js::String str("9.1e-k");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after a floating point number";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_FLOAT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(token->get_float64().get() == -1.0);
#pragma GCC diagnostic pop
    }
    {
        as2js::String str("91e+j");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_INVALID_NUMBER;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected letter after an integer";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        CPPUNIT_ASSERT(token->get_int64().get() == -1);
    }
}




// we test directly against the Unicode implementation
// of the Operating System (Unicode 6.x at time of
// writing.)
bool is_identifier_char(int32_t const c, bool const first)
{
    // rather strange special case (C had that one too way back)
    if(c == '$')
    {
        return true;
    }

    // digits are not accepted as first chars
    // (we have to test here because it would always
    // be true otherwise)
    if(c >= '0' && c <= '9')
    {
        return !first;
    }

    // special cases in JavaScript identifiers
    if(c == 0x200C    // ZWNJ
    || c == 0x200D)   // ZWJ
    {
        return true;
    }

    switch(u_charType(static_cast<UChar32>(c)))
    {
    case U_UPPERCASE_LETTER:
    case U_LOWERCASE_LETTER:
    case U_TITLECASE_LETTER:
    case U_MODIFIER_LETTER:
    case U_OTHER_LETTER:
    case U_LETTER_NUMBER:
    case U_NON_SPACING_MARK:
    case U_COMBINING_SPACING_MARK:
    case U_DECIMAL_DIGIT_NUMBER:
    case U_CONNECTOR_PUNCTUATION:
        return true;

    default:
        return false;

    }
}


void As2JsLexerUnitTests::test_identifiers()
{
    // identifiers can include all sorts of letters and can use escape
    // sequences to add a character otherwise rather difficult to type
    for(as2js::as_char_t c(0); c < 0x110000; ++c)
    {
        if(c % 50000 == 0)
        {
            std::cout << "." << std::flush;
        }

        if((c >= 0xD800 && c <= 0xDFFF)
        || (c & 0xFFFF) >= 0xFFFE)
        {
            // skip plain surrogates
            // and known invalid characters
            continue;
        }

        if(is_identifier_char(c, true))
        {
//std::cerr << "Next letter " << std::hex << c << "\n";
            // one letter
            {
                as2js::String str;
                str += c;

                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << *token;
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += c;
                CPPUNIT_ASSERT(token->get_string() == expected);
            }

            // two letters
            {
                as2js::String str;
                str += c;
                str += 'x';

                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += c;
                expected += 'x';
                CPPUNIT_ASSERT(token->get_string() == expected);
            }

            // use escape sequence instead:
            {
                std::stringstream ss;
                ss << "not_at_the_start";
                if(c < 0x100 && rand() % 3 == 0)
                {
                    ss << "\\x" << std::hex << c;
                }
                else if(c < 0x10000 && rand() % 3 == 0)
                {
                    ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << c;
                }
                else
                {
                    ss << "\\U" << std::hex << std::setfill('0') << std::setw(8) << c;
                }
                ss << "$"; // end with a dollar for fun

                as2js::String str(ss.str());

                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                options->set_option(as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, 1);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += "not_at_the_start";
                expected += c;
                expected += '$';
//std::cerr << *token << " org [" << ss.str() << "] expected [" << expected << "]\n";
                CPPUNIT_ASSERT(token->get_string() == expected);
            }
            {
                std::stringstream ss;
                if(c < 0x100 && rand() % 3 == 0)
                {
                    ss << "\\x" << std::hex << c;
                }
                else if(c < 0x10000 && rand() % 3 == 0)
                {
                    ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << c;
                }
                else
                {
                    ss << "\\U" << std::hex << std::setfill('0') << std::setw(8) << c;
                }
                ss << "_"; // end with an underscore

                as2js::String str(ss.str());

                as2js::Input::pointer_t input(new as2js::StringInput(str));
                as2js::Options::pointer_t options(new as2js::Options);
                options->set_option(as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES, 1);
                as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
                CPPUNIT_ASSERT(lexer->get_input() == input);
                as2js::Node::pointer_t token(lexer->get_next_token());
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(token->get_children_size() == 0);
                as2js::String expected;
                expected += c;
                expected += '_';
//std::cerr << *token << " expected [" << expected << "]\n";
                CPPUNIT_ASSERT(token->get_string() == expected);
            }
        }
    }
}


void As2JsLexerUnitTests::test_invalid_input()
{
    {
        as2js::String str;
        str += 0x2FFF;
        str += "wrong_again";

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected punctuation '\\U00002fff'";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << *token;
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        as2js::String expected_identifier;
        expected_identifier += "wrong_again";
        CPPUNIT_ASSERT(token->get_string() == expected_identifier);
    }
    {
        as2js::String str("@oops");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected punctuation '@'";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
//std::cerr << *token;
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        as2js::String expected_identifier;
        expected_identifier += "oops";
        CPPUNIT_ASSERT(token->get_string() == expected_identifier);
    }
    {
        as2js::String str("#re_oops");

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "unexpected punctuation '#'";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
        tc.got_called();
//std::cerr << *token;
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        as2js::String expected_identifier;
        expected_identifier += "re_oops";
        CPPUNIT_ASSERT(token->get_string() == expected_identifier);
    }
    {
        as2js::String str;
        str += '\\';
        str += 0x2028;
        str += "no_continuation";

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNKNOWN_ESCAPE_SEQUENCE;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_pos.new_line();
        expected.f_message = "unknown escape letter '\\U00002028'";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << *token;
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        as2js::String expected_identifier;
        expected_identifier += "no_continuation";
        CPPUNIT_ASSERT(token->get_string() == expected_identifier);
    }
    for(int idx(0xD800 - 2); idx < 0xE000; ++idx)
    {
        as2js::String str;
        int32_t character(idx == 0xD800 - 2 ? 0xFFFE : (idx == 0xD800 - 1 ? 0xFFFF : idx));
        str += character;
        str += "invalid";

        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        std::stringstream ss;
        ss << std::hex << character;
        expected.f_message = "invalid character '\\U0000" + ss.str() + "' found as is in the input stream";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::Input::pointer_t input(new as2js::StringInput(str));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);
        as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << *token;
        tc.got_called();
        CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
        CPPUNIT_ASSERT(token->get_children_size() == 0);
        as2js::String expected_identifier;
        expected_identifier += "invalid";
        CPPUNIT_ASSERT(token->get_string() == expected_identifier);
    }
}


namespace
{

char const g_mixed_tokens_one[] =
    /* LINE 1 */    "This is a 'long list' __LINE__ of tokens\n"
    /* LINE 2 */    "so we can __LINE__ better test that\n"
    /* LINE 3 */    "the lexer works as __LINE__ expected.\n"

    //
    // All operators (in order found in node.h):
    //   + = & ~ | ^ } ) ] : , ? / > < ! % * { ( [ . ; -
    //   += &= |= ^= /= &&= ||= ^^= >?= <?= %= *= **= <%= >%= <<= >>= >>>= -=
    //   () <=> --x == >= ++x <= && || ^^ ~= >? <? != !~ x++ x-- ** <% >% << >> >>> ~~ === !==
    //
                    // all operators should work the same with and without spaces
    /* LINE 4 */    "var a = __LINE__ + 1000 * 34 / 2 << 3 % 5.01;\n"
    /* LINE 5 */    "var a=__LINE__+1000*34/2<<3%5.01;\n"
    /* LINE 6 */    "use binary(1); use octal(1); var $ &= - __LINE__ += 0b1111101000 *= 0x22 /= 02 <<= 03 %= 5.01;\n"
    /* LINE 7 */    "var $&=-__LINE__+=0b1111101000*=0x22/=02<<=03%=5.01;\n"
    /* LINE 8 */    "var _$_ |= ~ __LINE__ ^ 0b1010101010 & 0x10201 - 02 >> 03710 ? 5.01 : 6.02;\n"
    /* LINE 9 */    "var _$_|=~__LINE__^0b1010101010&0x10201-02>>03710?5.01:6.02;\n"
    /* LINE 10 */   "use extended_operators(1); var $_ **= ! __LINE__ ^= 0b1010101010 ~= 0x10201 -= 02 >>= 03710 ~~ 5.01;\n"
    /* LINE 11 */   "var $_**=!__LINE__^=0b1010101010~=0x10201-=02>>=03710~~5.01;\n"
    /* LINE 12 */   "var f_field <?= $ . foo(__LINE__, a ++ >? $) ^ $_ [ 0b1111111111 ] ** 0xFF10201000 >>> 0112 ^^ 3710 == 5.01;\n"
    /* LINE 13 */   "var f_field<?=$.foo(__LINE__,a++>?$)^$_[0b1111111111]**0xFF10201000>>>0112^^3710==5.01;\n"
    /* LINE 14 */   "{ var f_field >?= \xEF\xBC\x91 . foo ( __LINE__, -- a <? $ ) != $_ [ 0b11111011111 ] <=> 0xFF10201000 >>>= 0112 ^^= 3710 === 5.01; }\n"
    /* LINE 15 */   "{var f_field>?=\xEF\xBC\x91.foo(__LINE__,--a<?$)!=$_[0b11111011111]<=>0xFF10201000>>>=0112^^=3710===5.01;}\n"
    /* LINE 16 */   "var b &&= __LINE__ && 1000 || 34 <% 2 >% 3 !== 5.01 , a --;\n"
    /* LINE 17 */   "var b&&=__LINE__&&1000||34<%2>%3!==5.01,a--;\n"
    /* LINE 18 */   "var c ||= __LINE__ <= 1000 >= 34 <%= 2 >%= 3 !== 5.01 , ++ a;\n"
    /* LINE 19 */   "var c||=__LINE__<=1000>=34<%=2>%=3!==5.01,++a;\n"
    /* LINE 20 */   "var c |= __LINE__ | 1000 > 34 < 2 !~ 3 .. 5 . length;\n"
    /* LINE 21 */   "var c|=__LINE__|1000>34<2!~3..5.length;\n"

    /* LINE 22 */   "abstract function long_shot(a: String, b: Number, c: double, ...);\n"
    /* LINE 23 */   "use extended_operators(2); var q = 91.e+j;\n"
;

result_t const g_mixed_results_one[] =
{
    // LINE 1 --    "This is a 'long list' __LINE__ of tokens\n"
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "This", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRING,
        CHECK_VALUE_STRING, 0, 0.0, "long list", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1, 0.0, "of", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "of", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "tokens", false,
        nullptr
    },

    // LINE 2 --    "so we can __LINE__ better test that\n"
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "so", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "we", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "can", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "better", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "test", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "that", false,
        nullptr
    },

    // LINE 3 --    "the lexer works as __LINE__ expected.\n"
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "the", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "lexer", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "works", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_AS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "expected", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 4 --    "var a = __LINE__ + 1000 * 34 / 2 << 3 % 5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 4, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MULTIPLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DIVIDE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SHIFT_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MODULO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 5 --    "var a=__LINE__+1000*34/2<<3%5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 5, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MULTIPLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DIVIDE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SHIFT_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MODULO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 6 --    "use binary(1); use octal(1); var $ &= - __LINE__ += 0b1111101000 *= 0x22 /= 2 <<= 3 %= 5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 6, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MODULO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 7 --    "var $&=-__LINE__+=1000*=34/=2<<=3%=5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 7, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MODULO,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 8 --    "var _$_ |= ~ __LINE__ ^ 0b1010101010 & 0x10201 - 02 >> 03710 ? 5.01 : 6.02;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "_$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_NOT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 8, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 682, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 66049, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SHIFT_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1992, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CONDITIONAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 6.02, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 9 --    "var _$_|=~__LINE__^0b1010101010&0x10201-02>>03710?5.01:6.02;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "_$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_NOT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 9, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 682, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 66049, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SHIFT_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1992, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CONDITIONAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 6.02, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 10 --   "use extended_operators(1); var $_ **= ! __LINE__ ^= 0b1010101010 ~= 0x10201 -= 02 >>= 03710 ~~ 5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_POWER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_NOT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 10, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 682, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 66049, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1992, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SMART_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 11 --   "var $_**=!__LINE__^=0b1010101010~=0x10201-=02>>=03710~~5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_POWER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_NOT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 11, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 682, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 66049, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1992, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SMART_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 12 --   "var f_field <?= $.foo(__LINE__, a >? $) ^ $_ [ 0b1111111111 ] ** 0xFF10201000 >>> 0112 ^^ 3710 == 5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "f_field", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "foo", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 12, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INCREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MAXIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1023, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_POWER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1095487197184, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 74, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3710, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 13 --   "var f_field<?=$.foo(__LINE__,a>?$)^$_[0b1111111111]**0xFF10201000>>>0112^^3710==5.01;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "f_field", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "foo", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 13, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INCREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MAXIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1023, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_POWER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1095487197184, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 74, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3710, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 14 --   "{ var f_field >?= \xFF11.foo(__LINE__, --a <? $) != $_ [ 0b11111011111 ] <=> 0xFF10201000 >>>= 0112 ^^ 3710 == 5.01; }\n"
    {
        as2js::Node::node_t::NODE_OPEN_CURVLY_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "f_field", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "\xEF\xBC\x91", false, // char 0xFF11
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "foo", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 14, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DECREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MINIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2015, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMPARE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1095487197184, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 74, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3710, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRICTLY_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_CURVLY_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 15 --   "{var f_field>?=\xFF11.foo(__LINE__,--a<?$)!=$_[0b11111011111]<=>0xFF10201000>>>=0112^^=3710===5.01;}\n"
    {
        as2js::Node::node_t::NODE_OPEN_CURVLY_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "f_field", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "\xEF\xBC\x91", false, // char 0xFF11
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "foo", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 15, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DECREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MINIMUM,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "$_", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2015, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMPARE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1095487197184, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 74, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3710, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRICTLY_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_CURVLY_BRACKET,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 16 --   "var b &&= __LINE__ && 1000 || 34 <% 2 >% 3 !== 5.01 , a --;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "b", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 16, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ROTATE_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ROTATE_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DECREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 17 --   "var b&&=__LINE__&&1000||34<%2>%3!==5.01,a--;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "b", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 17, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_AND,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LOGICAL_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ROTATE_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ROTATE_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DECREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 18 --   "var c ||= __LINE__ <= 1000 >= 34 <%= 2 >%= 3 !== 5.01 , ++ a;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "c", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 18, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LESS_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_GREATER_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INCREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 19 --   "var c||=__LINE__<=1000>=34<%=2>%=3!==5.01,++a;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "c", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 19, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LESS_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_GREATER_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FLOAT64,
        CHECK_VALUE_FLOATING_POINT, 0, 5.01, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INCREMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 20 --   "var c |= __LINE__ | 1000 > 34 < 2 !~ 3 .. 5 . length;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "c", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 20, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_GREATER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LESS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_NOT_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_RANGE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 5, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "length", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 21 --   "var c|=__LINE__|1000>34<2!~3..5.length;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "c", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 21, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_BITWISE_OR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 1000, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_GREATER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 34, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_LESS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 2, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_NOT_MATCH,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 3, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_RANGE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 5, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "length", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 22 --   "abstract function long_shot(a: String, b: Number, c: double, ...);\n"
    {
        as2js::Node::node_t::NODE_ABSTRACT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_FUNCTION,
        CHECK_VALUE_STRING, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "long_shot", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_OPEN_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "a", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "String", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "b", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "Number", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "c", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_DOUBLE,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_COMMA,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_REST,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_CLOSE_PARENTHESIS,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // LINE 23 --   "use extended_operators(2); var q = 91.e+j;\n"
    {
        as2js::Node::node_t::NODE_VAR,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "q", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ASSIGNMENT,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_INT64,
        CHECK_VALUE_INTEGER, 91, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_MEMBER,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "e", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_ADD,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_IDENTIFIER,
        CHECK_VALUE_STRING, 0, 0.0, "j", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_SEMICOLON,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },

    // Test over
    {
        as2js::Node::node_t::NODE_EOF,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    },
    {
        as2js::Node::node_t::NODE_UNKNOWN,
        CHECK_VALUE_IGNORE, 0, 0.0, "", false,
        nullptr
    }
};

token_t const g_mixed_tokens[] =
{
    { g_mixed_tokens_one, g_mixed_results_one }
};
size_t const g_mixed_tokens_size = sizeof(g_mixed_tokens) / sizeof(g_mixed_tokens[0]);

}
// no name namespace


void As2JsLexerUnitTests::test_mixed_tokens()
{
    for(size_t idx(0); idx < g_mixed_tokens_size; ++idx)
    {
//std::cerr << "IN:{" << g_mixed_tokens[idx].f_input << "}\n";
        as2js::String input_string;
        input_string.from_utf8(g_mixed_tokens[idx].f_input);
        as2js::Input::pointer_t input(new as2js::StringInput(input_string));
        as2js::Options::pointer_t options(new as2js::Options);
        as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
        CPPUNIT_ASSERT(lexer->get_input() == input);

        // contrary to the type test here we do not mess around with the
        // options and we know exactly what we're expecting and thus we
        // only need one result per entry and that's exactly what we get
        // (at least for now, the truth is that we could still check each
        // list of options... we may add that later!)
        for(result_t const *results(g_mixed_tokens[idx].f_results);
                            results->f_token != as2js::Node::node_t::NODE_UNKNOWN;
                            ++results)
        {
            as2js::Node::pointer_t token(lexer->get_next_token());
//std::cerr << *token;

            // handle pragma just like the parser
            while(token->get_type() == as2js::Node::node_t::NODE_USE)
            {
                // must be followed by an identifier
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                as2js::String const pragma_name(token->get_string());
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_OPEN_PARENTHESIS);
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_INT64);
                as2js::Options::option_t opt(as2js::Options::option_t::OPTION_UNKNOWN);
                if(pragma_name == "binary")
                {
                    opt = as2js::Options::option_t::OPTION_BINARY;
                }
                else if(pragma_name == "extended_escape_sequences")
                {
                    opt = as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES;
                }
                else if(pragma_name == "extended_operators")
                {
                    // we do need this one here because we have '<>' and ':='
                    // that are extended operators to be forbidden unless
                    // this is turned on
                    opt = as2js::Options::option_t::OPTION_EXTENDED_OPERATORS;
                }
                else if(pragma_name == "octal")
                {
                    opt = as2js::Options::option_t::OPTION_OCTAL;
                }
                CPPUNIT_ASSERT(opt != as2js::Options::option_t::OPTION_UNKNOWN);
//std::cerr << "  use " << static_cast<int>(opt) << " = " << token->get_int64().get() << "\n";
                options->set_option(opt, token->get_int64().get());
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_CLOSE_PARENTHESIS);
                token = lexer->get_next_token();
                CPPUNIT_ASSERT(token->get_type() == as2js::Node::node_t::NODE_SEMICOLON);

                // get the next token, it can be another option
                token = lexer->get_next_token();
//std::cerr << *token;
            }


            // token match
            CPPUNIT_ASSERT(token->get_type() == results->f_token);

            // no children
            CPPUNIT_ASSERT(token->get_children_size() == 0);

            // no links
            CPPUNIT_ASSERT(!token->get_instance());
            CPPUNIT_ASSERT(!token->get_type_node());
            CPPUNIT_ASSERT(!token->get_attribute_node());
            CPPUNIT_ASSERT(!token->get_goto_exit());
            CPPUNIT_ASSERT(!token->get_goto_enter());

            // no variables
            CPPUNIT_ASSERT(token->get_variable_size() == 0);

            // no parent
            CPPUNIT_ASSERT(!token->get_parent());

            // no parameters
            CPPUNIT_ASSERT(token->get_param_size() == 0);

            // not locked
            CPPUNIT_ASSERT(!token->is_locked());

            // default switch operator
            if(token->get_type() == as2js::Node::node_t::NODE_SWITCH)
            {
                CPPUNIT_ASSERT(token->get_switch_operator() == as2js::Node::node_t::NODE_UNKNOWN);
            }

            // ignore flags here, they were tested in the node test already

            // no attributes
            if(token->get_type() != as2js::Node::node_t::NODE_PROGRAM)
            {
                for(as2js::Node::attribute_t attr(as2js::Node::attribute_t::NODE_ATTR_PUBLIC);
                                attr < as2js::Node::attribute_t::NODE_ATTR_max;
                                attr = static_cast<as2js::Node::attribute_t>(static_cast<int>(attr) + 1))
                {
                    switch(attr)
                    {
                    case as2js::Node::attribute_t::NODE_ATTR_TYPE:
                        switch(token->get_type())
                        {
                        case as2js::Node::node_t::NODE_ADD:
                        case as2js::Node::node_t::NODE_ARRAY:
                        case as2js::Node::node_t::NODE_ARRAY_LITERAL:
                        case as2js::Node::node_t::NODE_AS:
                        case as2js::Node::node_t::NODE_ASSIGNMENT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_ADD:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MODULO:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_POWER:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
                        case as2js::Node::node_t::NODE_BITWISE_AND:
                        case as2js::Node::node_t::NODE_BITWISE_NOT:
                        case as2js::Node::node_t::NODE_BITWISE_OR:
                        case as2js::Node::node_t::NODE_BITWISE_XOR:
                        case as2js::Node::node_t::NODE_CALL:
                        case as2js::Node::node_t::NODE_CONDITIONAL:
                        case as2js::Node::node_t::NODE_DECREMENT:
                        case as2js::Node::node_t::NODE_DELETE:
                        case as2js::Node::node_t::NODE_DIVIDE:
                        case as2js::Node::node_t::NODE_EQUAL:
                        case as2js::Node::node_t::NODE_FALSE:
                        case as2js::Node::node_t::NODE_FLOAT64:
                        case as2js::Node::node_t::NODE_FUNCTION:
                        case as2js::Node::node_t::NODE_GREATER:
                        case as2js::Node::node_t::NODE_GREATER_EQUAL:
                        case as2js::Node::node_t::NODE_IDENTIFIER:
                        case as2js::Node::node_t::NODE_IN:
                        case as2js::Node::node_t::NODE_INCREMENT:
                        case as2js::Node::node_t::NODE_INSTANCEOF:
                        case as2js::Node::node_t::NODE_INT64:
                        case as2js::Node::node_t::NODE_IS:
                        case as2js::Node::node_t::NODE_LESS:
                        case as2js::Node::node_t::NODE_LESS_EQUAL:
                        case as2js::Node::node_t::NODE_LIST:
                        case as2js::Node::node_t::NODE_LOGICAL_AND:
                        case as2js::Node::node_t::NODE_LOGICAL_NOT:
                        case as2js::Node::node_t::NODE_LOGICAL_OR:
                        case as2js::Node::node_t::NODE_LOGICAL_XOR:
                        case as2js::Node::node_t::NODE_MATCH:
                        case as2js::Node::node_t::NODE_MAXIMUM:
                        case as2js::Node::node_t::NODE_MEMBER:
                        case as2js::Node::node_t::NODE_MINIMUM:
                        case as2js::Node::node_t::NODE_MODULO:
                        case as2js::Node::node_t::NODE_MULTIPLY:
                        case as2js::Node::node_t::NODE_NAME:
                        case as2js::Node::node_t::NODE_NEW:
                        case as2js::Node::node_t::NODE_NOT_EQUAL:
                        case as2js::Node::node_t::NODE_NULL:
                        case as2js::Node::node_t::NODE_OBJECT_LITERAL:
                        case as2js::Node::node_t::NODE_POST_DECREMENT:
                        case as2js::Node::node_t::NODE_POST_INCREMENT:
                        case as2js::Node::node_t::NODE_POWER:
                        case as2js::Node::node_t::NODE_PRIVATE:
                        case as2js::Node::node_t::NODE_PUBLIC:
                        case as2js::Node::node_t::NODE_RANGE:
                        case as2js::Node::node_t::NODE_ROTATE_LEFT:
                        case as2js::Node::node_t::NODE_ROTATE_RIGHT:
                        case as2js::Node::node_t::NODE_SCOPE:
                        case as2js::Node::node_t::NODE_SHIFT_LEFT:
                        case as2js::Node::node_t::NODE_SHIFT_RIGHT:
                        case as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                        case as2js::Node::node_t::NODE_STRICTLY_EQUAL:
                        case as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL:
                        case as2js::Node::node_t::NODE_STRING:
                        case as2js::Node::node_t::NODE_SUBTRACT:
                        case as2js::Node::node_t::NODE_SUPER:
                        case as2js::Node::node_t::NODE_THIS:
                        case as2js::Node::node_t::NODE_TRUE:
                        case as2js::Node::node_t::NODE_TYPEOF:
                        case as2js::Node::node_t::NODE_UNDEFINED:
                        case as2js::Node::node_t::NODE_VIDENTIFIER:
                        case as2js::Node::node_t::NODE_VOID:
                            CPPUNIT_ASSERT(!token->get_attribute(attr));
                            break;

                        default:
                            // any other type and you get an exception
                            CPPUNIT_ASSERT_THROW(!token->get_attribute(attr), as2js::exception_internal_error);
                            break;

                        }
                        break;

                    default:
                        CPPUNIT_ASSERT(!token->get_attribute(attr));
                        break;

                    }
                }
            }

            if(results->f_check_value == CHECK_VALUE_INTEGER)
            {
//std::cerr << "int " << token->get_int64().get() << " vs " << results->f_integer;
                CPPUNIT_ASSERT(token->get_int64().get() == results->f_integer);
            }
            else
            {
                CPPUNIT_ASSERT_THROW(token->get_int64().get() == results->f_integer, as2js::exception_internal_error);
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(results->f_check_value == CHECK_VALUE_FLOATING_POINT)
            {
                if(std::isnan(results->f_floating_point))
                {
                    CPPUNIT_ASSERT(token->get_float64().is_NaN());
                }
                else
                {
                    CPPUNIT_ASSERT(token->get_float64().get() == results->f_floating_point);
                }
            }
            else
            {
                CPPUNIT_ASSERT_THROW(token->get_float64().get() == results->f_integer, as2js::exception_internal_error);
            }
#pragma GCC diagnostic pop

            if(results->f_check_value == CHECK_VALUE_STRING)
            {
//std::cerr << "  --> [" << token->get_string() << "]\n";
                as2js::String str;
                str.from_utf8(results->f_string);
                CPPUNIT_ASSERT(token->get_string() == str);
            }
            else
            {
                // no need to convert the results->f_string should should be ""
                CPPUNIT_ASSERT_THROW(token->get_string() == results->f_string, as2js::exception_internal_error);
            }

            if(results->f_check_value == CHECK_VALUE_BOOLEAN)
            {
                CPPUNIT_ASSERT(token->get_boolean() == results->f_boolean);
            }
            else
            {
                CPPUNIT_ASSERT_THROW(token->get_boolean() == results->f_boolean, as2js::exception_internal_error);
            }
        }
    }
}


// vim: ts=4 sw=4 et
