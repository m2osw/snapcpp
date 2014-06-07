/* node.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/node.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <controlled_vars/controlled_vars_auto_enum_init.h>

#include    <algorithm>
#include    <sstream>
#include    <iomanip>


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  NODE  ***********************************************************/
/**********************************************************************/
/**********************************************************************/

namespace
{

struct type_name_t
{
    Node::node_t    f_type;
    const char *    f_name;
    int             f_line;
};

#define    TO_STR_sub(s)            #s
#define    NODE_TYPE_NAME(node)     { Node::node_t::NODE_##node, TO_STR_sub(node), __LINE__ }

type_name_t const g_node_type_name[] =
{
    // EOF is -1 on most C/C++ computers... so we have to do this one by hand
    { Node::node_t::NODE_EOF, "EOF", __LINE__ },
    NODE_TYPE_NAME(UNKNOWN),

    // the one character types have to be ordered by their character
    // which means it does not match the alphabetical order we
    // generally use
    NODE_TYPE_NAME(LOGICAL_NOT),                      // 0x21
    NODE_TYPE_NAME(MODULO),                           // 0x25
    NODE_TYPE_NAME(BITWISE_AND),                      // 0x26
    NODE_TYPE_NAME(OPEN_PARENTHESIS),                 // 0x28
    NODE_TYPE_NAME(CLOSE_PARENTHESIS),                // 0x29
    NODE_TYPE_NAME(MULTIPLY),                         // 0x2A
    NODE_TYPE_NAME(ADD),                              // 0x2B
    NODE_TYPE_NAME(COMMA),                            // 0x2C
    NODE_TYPE_NAME(SUBTRACT),                         // 0x2D
    NODE_TYPE_NAME(MEMBER),                           // 0x2E
    NODE_TYPE_NAME(DIVIDE),                           // 0x2F
    NODE_TYPE_NAME(COLON),                            // 0x3A
    NODE_TYPE_NAME(SEMICOLON),                        // 0x3B
    NODE_TYPE_NAME(LESS),                             // 0x3C
    NODE_TYPE_NAME(ASSIGNMENT),                       // 0x3D
    NODE_TYPE_NAME(GREATER),                          // 0x3E
    NODE_TYPE_NAME(CONDITIONAL),                      // 0x3F
    NODE_TYPE_NAME(OPEN_SQUARE_BRACKET),              // 0x5B
    NODE_TYPE_NAME(CLOSE_SQUARE_BRACKET),             // 0x5D
    NODE_TYPE_NAME(BITWISE_XOR),                      // 0x5E
    NODE_TYPE_NAME(OPEN_CURVLY_BRACKET),              // 0x7B
    NODE_TYPE_NAME(BITWISE_OR),                       // 0x7C
    NODE_TYPE_NAME(CLOSE_CURVLY_BRACKET),             // 0x7D
    NODE_TYPE_NAME(BITWISE_NOT),                      // 0x7E

    NODE_TYPE_NAME(ARRAY),
    NODE_TYPE_NAME(ARRAY_LITERAL),
    NODE_TYPE_NAME(AS),
    NODE_TYPE_NAME(ASSIGNMENT_ADD),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_AND),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_OR),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_XOR),
    NODE_TYPE_NAME(ASSIGNMENT_DIVIDE),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_AND),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_OR),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_XOR),
    NODE_TYPE_NAME(ASSIGNMENT_MAXIMUM),
    NODE_TYPE_NAME(ASSIGNMENT_MINIMUM),
    NODE_TYPE_NAME(ASSIGNMENT_MODULO),
    NODE_TYPE_NAME(ASSIGNMENT_MULTIPLY),
    NODE_TYPE_NAME(ASSIGNMENT_POWER),
    NODE_TYPE_NAME(ASSIGNMENT_ROTATE_LEFT),
    NODE_TYPE_NAME(ASSIGNMENT_ROTATE_RIGHT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_LEFT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_RIGHT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_RIGHT_UNSIGNED),
    NODE_TYPE_NAME(ASSIGNMENT_SUBTRACT),
    NODE_TYPE_NAME(ATTRIBUTES),
    NODE_TYPE_NAME(AUTO),
    NODE_TYPE_NAME(BREAK),
    NODE_TYPE_NAME(CALL),
    NODE_TYPE_NAME(CASE),
    NODE_TYPE_NAME(CATCH),
    NODE_TYPE_NAME(CLASS),
    NODE_TYPE_NAME(CONST),
    NODE_TYPE_NAME(CONTINUE),
    NODE_TYPE_NAME(DEBUGGER),
    NODE_TYPE_NAME(DECREMENT),
    NODE_TYPE_NAME(DEFAULT),
    NODE_TYPE_NAME(DELETE),
    NODE_TYPE_NAME(DIRECTIVE_LIST),
    NODE_TYPE_NAME(DO),
    NODE_TYPE_NAME(ELSE),
    NODE_TYPE_NAME(EMPTY),
    NODE_TYPE_NAME(ENTRY),
    NODE_TYPE_NAME(ENUM),
    NODE_TYPE_NAME(EQUAL),
    NODE_TYPE_NAME(EXCLUDE),
    NODE_TYPE_NAME(EXTENDS),
    NODE_TYPE_NAME(FALSE),
    NODE_TYPE_NAME(FINALLY),
    NODE_TYPE_NAME(FLOAT64),
    NODE_TYPE_NAME(FOR),
    NODE_TYPE_NAME(FUNCTION),
    NODE_TYPE_NAME(GOTO),
    NODE_TYPE_NAME(GREATER_EQUAL),
    NODE_TYPE_NAME(IDENTIFIER),
    NODE_TYPE_NAME(IF),
    NODE_TYPE_NAME(IMPLEMENTS),
    NODE_TYPE_NAME(IMPORT),
    NODE_TYPE_NAME(IN),
    NODE_TYPE_NAME(INCLUDE),
    NODE_TYPE_NAME(INCREMENT),
    NODE_TYPE_NAME(INSTANCEOF),
    NODE_TYPE_NAME(INT64),
    NODE_TYPE_NAME(INTERFACE),
    NODE_TYPE_NAME(IS),
    NODE_TYPE_NAME(LABEL),
    NODE_TYPE_NAME(LESS_EQUAL),
    NODE_TYPE_NAME(LIST),
    NODE_TYPE_NAME(LOGICAL_AND),
    NODE_TYPE_NAME(LOGICAL_OR),
    NODE_TYPE_NAME(LOGICAL_XOR),
    NODE_TYPE_NAME(MATCH),
    NODE_TYPE_NAME(MAXIMUM),
    NODE_TYPE_NAME(MINIMUM),
    NODE_TYPE_NAME(NAME),
    NODE_TYPE_NAME(NAMESPACE),
    NODE_TYPE_NAME(NEW),
    NODE_TYPE_NAME(NOT_EQUAL),
    //NODE_TYPE_NAME(NULL), -- macro does not work in this case
    { Node::node_t::NODE_NULL, "NULL", __LINE__ },
    NODE_TYPE_NAME(OBJECT_LITERAL),
    NODE_TYPE_NAME(PACKAGE),
    NODE_TYPE_NAME(PARAM),
    NODE_TYPE_NAME(PARAMETERS),
    NODE_TYPE_NAME(PARAM_MATCH),
    NODE_TYPE_NAME(POST_DECREMENT),
    NODE_TYPE_NAME(POST_INCREMENT),
    NODE_TYPE_NAME(POWER),
    NODE_TYPE_NAME(PRIVATE),
    NODE_TYPE_NAME(PROGRAM),
    NODE_TYPE_NAME(PUBLIC),
    NODE_TYPE_NAME(RANGE),
    NODE_TYPE_NAME(REGULAR_EXPRESSION),
    NODE_TYPE_NAME(REST),
    NODE_TYPE_NAME(RETURN),
    NODE_TYPE_NAME(ROOT),
    NODE_TYPE_NAME(ROTATE_LEFT),
    NODE_TYPE_NAME(ROTATE_RIGHT),
    NODE_TYPE_NAME(SCOPE),
    NODE_TYPE_NAME(SET),
    NODE_TYPE_NAME(SHIFT_LEFT),
    NODE_TYPE_NAME(SHIFT_RIGHT),
    NODE_TYPE_NAME(SHIFT_RIGHT_UNSIGNED),
    NODE_TYPE_NAME(STRICTLY_EQUAL),
    NODE_TYPE_NAME(STRICTLY_NOT_EQUAL),
    NODE_TYPE_NAME(STRING),
    NODE_TYPE_NAME(SUPER),
    NODE_TYPE_NAME(SWITCH),
    NODE_TYPE_NAME(THIS),
    NODE_TYPE_NAME(THROW),
    NODE_TYPE_NAME(TRUE),
    NODE_TYPE_NAME(TRY),
    NODE_TYPE_NAME(TYPE),
    NODE_TYPE_NAME(TYPEOF),
    NODE_TYPE_NAME(UNDEFINED),
    NODE_TYPE_NAME(USE),
    NODE_TYPE_NAME(VAR),
    NODE_TYPE_NAME(VARIABLE),
    NODE_TYPE_NAME(VAR_ATTRIBUTES),
    NODE_TYPE_NAME(VIDENTIFIER),
    NODE_TYPE_NAME(VOID),
    NODE_TYPE_NAME(WHILE),
    NODE_TYPE_NAME(WITH),

    // end list
    //{ Node::node_t::NODE_UNKNOWN, nullptr, __LINE__ }
};
size_t const g_node_type_name_size = sizeof(g_node_type_name) / sizeof(g_node_type_name[0]);

struct operator_to_string_t
{
    Node::node_t    f_node;
    const char *    f_name;
    int             f_line;
};

operator_to_string_t const g_operator_to_string[] =
{
    // single character -- sorted in ASCII
    { Node::node_t::NODE_LOGICAL_NOT,                     "!", __LINE__ },
    { Node::node_t::NODE_MODULO,                          "%", __LINE__ },
    { Node::node_t::NODE_BITWISE_AND,                     "&", __LINE__ },
    { Node::node_t::NODE_MULTIPLY,                        "*", __LINE__ },
    { Node::node_t::NODE_ADD,                             "+", __LINE__ },
    { Node::node_t::NODE_SUBTRACT,                        "-", __LINE__ },
    { Node::node_t::NODE_DIVIDE,                          "/", __LINE__ },
    { Node::node_t::NODE_LESS,                            "<", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT,                      "=", __LINE__ },
    { Node::node_t::NODE_GREATER,                         ">", __LINE__ },
    { Node::node_t::NODE_BITWISE_XOR,                     "^", __LINE__ },
    { Node::node_t::NODE_BITWISE_OR,                      "|", __LINE__ },
    { Node::node_t::NODE_BITWISE_NOT,                     "~", __LINE__ },

    // two or more characters transformed to an enum only
    { Node::node_t::NODE_ASSIGNMENT_ADD,                  "+=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_BITWISE_AND,          "&=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,           "|=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR,          "^=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_DIVIDE,               "/=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND,          "&&=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR,           "||=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR,          "^^=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MAXIMUM,              "?>=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MINIMUM,              "?<=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MODULO,               "%=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MULTIPLY,             "*=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_POWER,                "**=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT,          "!<=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,         "!>=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT,           "<<=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,          ">>=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED, ">>>=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SUBTRACT,             "-=", __LINE__ },
    { Node::node_t::NODE_CALL,                            "()", __LINE__ },
    { Node::node_t::NODE_DECREMENT,                       "--", __LINE__ },
    { Node::node_t::NODE_EQUAL,                           "==", __LINE__ },
    { Node::node_t::NODE_GREATER_EQUAL,                   ">=", __LINE__ },
    { Node::node_t::NODE_INCREMENT,                       "++", __LINE__ },
    { Node::node_t::NODE_LESS_EQUAL,                      "<=", __LINE__ },
    { Node::node_t::NODE_LOGICAL_AND,                     "&&", __LINE__ },
    { Node::node_t::NODE_LOGICAL_OR,                      "||", __LINE__ },
    { Node::node_t::NODE_LOGICAL_XOR,                     "^^", __LINE__ },
    { Node::node_t::NODE_MATCH,                           "~=", __LINE__ },
    { Node::node_t::NODE_MAXIMUM,                         "?>", __LINE__ },
    { Node::node_t::NODE_MINIMUM,                         "?<", __LINE__ },
    { Node::node_t::NODE_NOT_EQUAL,                       "!=", __LINE__ },
    { Node::node_t::NODE_POST_DECREMENT,                  "--", __LINE__ },
    { Node::node_t::NODE_POST_INCREMENT,                  "++", __LINE__ },
    { Node::node_t::NODE_POWER,                           "**", __LINE__ },
    { Node::node_t::NODE_ROTATE_LEFT,                     "!<", __LINE__ },
    { Node::node_t::NODE_ROTATE_RIGHT,                    "!>", __LINE__ },
    { Node::node_t::NODE_SHIFT_LEFT,                      "<<", __LINE__ },
    { Node::node_t::NODE_SHIFT_RIGHT,                     ">>", __LINE__ },
    { Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED,            ">>>", __LINE__ },
    { Node::node_t::NODE_STRICTLY_EQUAL,                  "===", __LINE__ },
    { Node::node_t::NODE_STRICTLY_NOT_EQUAL,              "!==", __LINE__ }

// the following doesn't make it in user redefinable operators yet
    //{ Node::node_t::NODE_CONDITIONAL,                   "", __LINE__ },
    //{ Node::node_t::NODE_DELETE,                        "", __LINE__ },
    //{ Node::node_t::NODE_IN,                            "", __LINE__ },
    //{ Node::node_t::NODE_INSTANCEOF,                    "", __LINE__ },
    //{ Node::node_t::NODE_IS,                            "", __LINE__ },
    //{ Node::node_t::NODE_LIST,                          "", __LINE__ },
    //{ Node::node_t::NODE_NEW,                           "", __LINE__ },
    //{ Node::node_t::NODE_RANGE,                         "", __LINE__ },
    //{ Node::node_t::NODE_SCOPE,                         "", __LINE__ },
};

size_t const g_operator_to_string_size = sizeof(g_operator_to_string) / sizeof(g_operator_to_string[0]);

enum
{
    ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION,
    ATTRIBUTES_GROUP_FUNCTION_TYPE,
    ATTRIBUTES_GROUP_SWITCH_TYPE,
    ATTRIBUTES_GROUP_MEMBER_VISIBILITY
};
char const *g_attribute_groups[] =
{
    [ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION] = "true and false",
    [ATTRIBUTES_GROUP_FUNCTION_TYPE] = "static, abstract, virtual, and constructor",
    [ATTRIBUTES_GROUP_SWITCH_TYPE] = "foreach, nobreak, and autobreak",
    [ATTRIBUTES_GROUP_MEMBER_VISIBILITY] = "public, private, and protected"
};


}
// no name namespace




Node::Node(node_t type)
    : f_type(type)
    //, f_flags() -- auto-init
    //, f_attributes() -- auto-init
    //, f_switch_operator(NODE_UNKNOWN) -- auto-init
    //, f_lock(0) -- auto-init
    //, f_position() -- auto-init
    //, f_int() -- auto-init
    //, f_float() -- auto-init
    //, f_str() -- auto-init
    //, f_parent() -- auto-init
    //, f_offset(0) -- auto-init
    //, f_children() -- auto-init
    //, f_link() -- auto-init
    //, f_variables() -- auto-init
    //, f_labels() -- auto-init
{
    switch(type)
    {
    case node_t::NODE_EOF:
    case node_t::NODE_UNKNOWN:
    case node_t::NODE_ADD:
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_CLOSE_CURVLY_BRACKET:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COLON:
    case node_t::NODE_COMMA:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_GREATER:
    case node_t::NODE_LESS:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_MEMBER:
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_ARRAY:
    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_AS:
    case node_t::NODE_ASSIGNMENT_ADD:
    case node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node_t::NODE_ASSIGNMENT_DIVIDE:
    case node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case node_t::NODE_ASSIGNMENT_MAXIMUM:
    case node_t::NODE_ASSIGNMENT_MINIMUM:
    case node_t::NODE_ASSIGNMENT_MODULO:
    case node_t::NODE_ASSIGNMENT_MULTIPLY:
    case node_t::NODE_ASSIGNMENT_POWER:
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_ASSIGNMENT_SUBTRACT:
    case node_t::NODE_ATTRIBUTES:
    case node_t::NODE_AUTO:
    case node_t::NODE_BREAK:
    case node_t::NODE_CALL:
    case node_t::NODE_CASE:
    case node_t::NODE_CATCH:
    case node_t::NODE_CLASS:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DEBUGGER:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DEFAULT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DIRECTIVE_LIST:
    case node_t::NODE_DO:
    case node_t::NODE_ELSE:
    case node_t::NODE_EMPTY:
    case node_t::NODE_ENTRY:
    case node_t::NODE_ENUM:
    case node_t::NODE_EQUAL:
    case node_t::NODE_EXCLUDE:
    case node_t::NODE_EXTENDS:
    case node_t::NODE_FALSE:
    case node_t::NODE_FINALLY:
    case node_t::NODE_FLOAT64:
    case node_t::NODE_FOR:
    case node_t::NODE_FUNCTION:
    case node_t::NODE_GOTO:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_IF:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_IMPORT:
    case node_t::NODE_IN:
    case node_t::NODE_INCLUDE:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_INT64:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_IS:
    case node_t::NODE_LABEL:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LIST:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_NAME:
    case node_t::NODE_NAMESPACE:
    case node_t::NODE_NEW:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_NULL:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_PACKAGE:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_POWER:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROGRAM:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_RANGE:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_REST:
    case node_t::NODE_RETURN:
    case node_t::NODE_ROOT:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SET:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_STRING:
    case node_t::NODE_SUPER:
    case node_t::NODE_SWITCH:
    case node_t::NODE_THIS:
    case node_t::NODE_THROW:
    case node_t::NODE_TRUE:
    case node_t::NODE_TRY:
    case node_t::NODE_TYPE:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_USE:
    case node_t::NODE_VAR:
    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_WHILE:
    case node_t::NODE_WITH:
        break;

    default:
        // ERROR: some values are not valid as a type
        throw exception_incompatible_node_type("invalid type used to create a node");

    }
}


Node::Node(pointer_t const& source, pointer_t& parent)
    : f_type(source->f_type)
    , f_flags(source->f_flags)
    , f_attributes(source->f_attributes)
    , f_switch_operator(source->f_switch_operator)
    //, f_lock(0) -- auto-init
    , f_position(source->f_position)
    , f_int(source->f_int)
    , f_float(source->f_float)
    , f_str(source->f_str)
    //, f_parent(parent) -- requires a function call to work properly
    //, f_offset(0) -- auto-init
    //, f_children() -- auto-init
    , f_link(source->f_link)
    //, f_variables() -- auto-init
    //, f_labels() -- auto-init
{
    switch(f_type)
    {
    case node_t::NODE_STRING:
    case node_t::NODE_INT64:
    case node_t::NODE_FLOAT64:
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
    case node_t::NODE_NULL:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_REGULAR_EXPRESSION:
        break;

    default:
        // ERROR: only constants can be cloned at this time
        throw exception_incompatible_node_type("only nodes representing constants can be cloned");

    }

    set_parent(parent);
}


/**********************************************************************/
/**********************************************************************/
/***  DATA DISPLAY  ***************************************************/
/**********************************************************************/
/**********************************************************************/


Node::node_t Node::get_type() const
{
    return f_type;
}


const char *Node::get_type_name() const
{
#if defined(_DEBUG) || defined(DEBUG)
    {
        // make sure that the node types are properly sorted
        static bool checked = false;
        if(!checked)
        {
            // check only once
            checked = true;
            for(size_t idx = 1; idx < g_node_type_name_size; ++idx)
            {
                if(g_node_type_name[idx].f_type <= g_node_type_name[idx - 1].f_type)
                {
                    // if the table is properly defined then we cannot reach
                    // these lines
                    std::cerr << "INTERNAL ERROR at offset " << idx                                                     // LCOV_EXCL_LINE
                              << " (line #" << g_node_type_name[idx].f_line                                             // LCOV_EXCL_LINE
                              << ", node type " << static_cast<uint32_t>(g_node_type_name[idx].f_type)                  // LCOV_EXCL_LINE
                              << " vs. " << static_cast<uint32_t>(g_node_type_name[idx - 1].f_type)                     // LCOV_EXCL_LINE
                              << "): the g_node_type_name table is not sorted properly. We cannot binary search it."    // LCOV_EXCL_LINE
                              << std::endl;                                                                             // LCOV_EXCL_LINE
                    throw exception_internal_error("INTERNAL ERROR: node type names not properly sorted, cannot properly search for names using a binary search."); // LCOV_EXCL_LINE
                }
            }
        }
    }
#endif

    size_t i, j, p;
    int    r;

    i = 0;
    j = g_node_type_name_size;
    while(i < j)
    {
        p = (j - i) / 2 + i;
        r = static_cast<int>(g_node_type_name[p].f_type) - static_cast<int>(static_cast<node_t>(f_type));
        if(r == 0)
        {
            return g_node_type_name[p].f_name;
        }
        if(r < 0)
        {
            i = p + 1;
        }
        else
        {
            j = p;
        }
    }

    return "<undefined type name>";
}


/** \brief Return true if node represens a number.
 *
 * This function returns true if the node is an integer or a
 * floating point value.
 *
 * Note that this function returns false on a string that
 * represents a valid number.
 *
 * Note that JavaScript also considered Boolean values and null as
 * valid numbers. To test such, use is_nan() instead
 *
 * \return true if this node represents a number
 */
bool Node::is_number() const
{
    return f_type == node_t::NODE_INT64 || f_type == node_t::NODE_FLOAT64;
}


/** \brief Check whether this node represents a NaN if converted to a number.
 *
 * When converting a node to a number (to_number() function) we accept a
 * certain number of parameters as numbers:
 *
 * \li Integers (unchanged)
 * \li Float points (unchanged)
 * \li True (1) or False (0)
 * \li Null (0)
 * \li Strings that represent valid numbers as a whole
 *
 * \return true if the value could not be converted to a number.
 */
bool Node::is_nan() const
{
    if(f_type == node_t::NODE_STRING)
    {
        return f_str.is_number();
    }

    return f_type != node_t::NODE_INT64
        && f_type != node_t::NODE_FLOAT64
        && f_type != node_t::NODE_TRUE
        && f_type != node_t::NODE_FALSE
        && f_type != node_t::NODE_NULL;
}


/** \brief Check whether a node is an integer.
 *
 * This function checks whether the type of the node is NODE_INT64.
 *
 * \return true if the node type is NODE_INT64.
 */
bool Node::is_int64() const
{
    return f_type == node_t::NODE_INT64;
}


/** \brief Check whether a node is a floating point.
 *
 * This function checks whether the type of the node is NODE_FLOAT64.
 *
 * \return true if the node type is NODE_FLOAT64.
 */
bool Node::is_float64() const
{
    return f_type == node_t::NODE_FLOAT64;
}


/** \brief Check whether a node is a Boolean value.
 *
 * This function checks whether the type of the node is NODE_TRUE or
 * NODE_FALSE.
 *
 * \return true if the node type represents a boolean value.
 */
bool Node::is_boolean() const
{
    return f_type == node_t::NODE_TRUE || f_type == node_t::NODE_FALSE;
}


/** \brief Check whether a node represents the true Boolean value.
 *
 * This function checks whether the type of the node is NODE_TRUE.
 *
 * \return true if the node type represents true.
 */
bool Node::is_true() const
{
    return f_type == node_t::NODE_TRUE;
}


/** \brief Check whether a node represents the false Boolean value.
 *
 * This function checks whether the type of the node is NODE_FALSE.
 *
 * \return true if the node type represents false.
 */
bool Node::is_false() const
{
    return f_type == node_t::NODE_FALSE;
}


/** \brief Check whether a node is a string.
 *
 * This function checks whether the type of the node is NODE_STRING.
 *
 * \return true if the node type represents a string value.
 */
bool Node::is_string() const
{
    return f_type == node_t::NODE_STRING;
}


/** \brief Check whether a node is the special value undefined.
 *
 * This function checks whether the type of the node is NODE_UNDEFINED.
 *
 * \return true if the node type represents the undefined value.
 */
bool Node::is_undefined() const
{
    return f_type == node_t::NODE_UNDEFINED;
}


/** \brief Check whether a node is the special value null.
 *
 * This function checks whether the type of the node is NODE_NULL.
 *
 * \return true if the node type represents the null value.
 */
bool Node::is_null() const
{
    return f_type == node_t::NODE_NULL;
}


/** \brief Check whether a node is an identifier.
 *
 * This function checks whether the type of the node is NODE_IDENTIFIER
 * or NODE_VIDENTIFIER.
 *
 * \return true if the node type represents an identifier value.
 */
bool Node::is_identifier() const
{
    return f_type == node_t::NODE_IDENTIFIER || f_type == node_t::NODE_VIDENTIFIER;
}


/**********************************************************************/
/**********************************************************************/
/***  DATA CONVERSION  ************************************************/
/**********************************************************************/
/**********************************************************************/


void Node::to_unknown()
{
    // whatever the type of node we can always convert it to an unknown
    // node since that's similar to "deleting" the node
    f_type = node_t::NODE_UNKNOWN;
    // clear the node's data to avoid other problems?
}


bool Node::to_as()
{
    // "a call to a getter" may be transformed from CALL to AS
    // because a getter can very much look like a cast (false positive)
    if(node_t::NODE_CALL == f_type)
    {
        f_type = node_t::NODE_AS;
        return true;
    }

    return false;
}


Node::node_t Node::to_boolean_type_only() const
{
    switch(f_type)
    {
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        // already a boolean
        return f_type;

    case node_t::NODE_NULL:
    case node_t::NODE_UNDEFINED:
        return node_t::NODE_FALSE;

    case node_t::NODE_INT64:
        return f_int.get() != 0 ? node_t::NODE_TRUE : node_t::NODE_FALSE;

    case node_t::NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return f_float.get() != 0.0 && !f_float.is_NaN() ? node_t::NODE_TRUE : node_t::NODE_FALSE;
#pragma GCC diagnostic pop

    case node_t::NODE_STRING:
        return f_str.is_true() ? node_t::NODE_TRUE : node_t::NODE_FALSE;

    default:
        // failure (cannot convert)
        return node_t::NODE_UNDEFINED;

    }
    /*NOTREACHED*/
}


bool Node::to_boolean()
{
    switch(f_type)
    {
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        // already a boolean
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_UNDEFINED:
        f_type = node_t::NODE_FALSE;
        break;

    case node_t::NODE_INT64:
        f_type = f_int.get() != 0 ? node_t::NODE_TRUE : node_t::NODE_FALSE;
        break;

    case node_t::NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        f_type = f_float.get() != 0.0 && !f_float.is_NaN() ? node_t::NODE_TRUE : node_t::NODE_FALSE;
#pragma GCC diagnostic pop
        break;

    case node_t::NODE_STRING:
        f_type = f_str.is_true() ? node_t::NODE_TRUE : node_t::NODE_FALSE;
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    return true;
}


bool Node::to_call()
{
    // getters are transformed from MEMBER to CALL
    // setters are transformed from ASSIGNMENT to CALL
    if(node_t::NODE_MEMBER == f_type        // member getter
    || node_t::NODE_ASSIGNMENT == f_type)   // assignment setter
    {
        f_type = node_t::NODE_CALL;
        return true;
    }

    return false;
}


bool Node::to_int64()
{
    switch(f_type)
    {
    case node_t::NODE_INT64:
        return true;

    case node_t::NODE_FLOAT64:
        f_int.set(f_float.get());
        break;

    case node_t::NODE_TRUE:
        f_int.set(1);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
    case node_t::NODE_UNDEFINED: // should return NaN, not possible with an integer...
        f_int.set(0);
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    f_type = node_t::NODE_INT64;
    return true;
}


bool Node::to_float64()
{
    switch(f_type)
    {
    case node_t::NODE_INT64:
        f_float.set(f_int.get());
        break;

    case node_t::NODE_FLOAT64:
        return true;

    case node_t::NODE_TRUE:
        f_float.set(1.0);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        f_float.set(0.0);
        break;

    case node_t::NODE_UNDEFINED:
        f_float.set_NaN();
        break;

    default:
        // failure (cannot convert)
        return false;

    }

    f_type = node_t::NODE_FLOAT64;
    return true;
}


/** \brief Convert this node to a number.
 *
 * This function converts the node to a number just like JavaScript would do.
 * This means converting the following type of nodes:
 *
 * \li NODE_INT64 -- no conversion
 * \li NODE_FLOAT64 -- no conversion
 * \li NODE_TRUE -- convert to 1 (INT64)
 * \li NODE_FALSE -- convert to 0 (INT64)
 * \li NODE_UNDEFINED -- convert to NaN (FLOAT64)
 */
bool Node::to_number()
{
    switch(f_type)
    {
    case node_t::NODE_INT64:
    case node_t::NODE_FLOAT64:
        break;

    case node_t::NODE_TRUE:
        f_type = node_t::NODE_INT64;
        f_int.set(1);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        f_type = node_t::NODE_INT64;
        f_int.set(0);
        break;

    case node_t::NODE_UNDEFINED:
        f_type = node_t::NODE_FLOAT64;
        f_float.set_NaN();
        break;

    case node_t::NODE_STRING:
        // JavaScript tends to force conversions from stings to numbers
        // when possible (actually it always is, only strings often become
        // NaN as a result)
        f_type = node_t::NODE_FLOAT64;
        f_float.set(f_str.to_float64());
        break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


bool Node::to_string()
{
    switch(f_type)
    {
    case node_t::NODE_STRING:
        return true;

    case node_t::NODE_IDENTIFIER:
        // this happens with special identifiers that are strings in the end
        break;

    case node_t::NODE_UNDEFINED:
        f_str = "undefined";
        break;

    case node_t::NODE_NULL:
        f_str = "null";
        break;

    case node_t::NODE_TRUE:
        f_str = "true";
        break;

    case node_t::NODE_FALSE:
        f_str = "false";
        break;

    case node_t::NODE_INT64:
        f_str = std::to_string(f_int.get());
        break;

    case node_t::NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    {
        Float64::float64_type const value(f_float.get());
        if(f_float.is_NaN())
        {
            f_str = "NaN";
        }
        else if(value == 0.0)
        {
            // make sure it does not become "0.0"
            f_str = "0";
        }
        else if(f_float.is_negative_infinity())
        {
            f_str = "-Infinity";
        }
        else if(f_float.is_positive_infinity())
        {
            f_str = "Infinity";
        }
        else
        {
            f_str = std::to_string(value);
        }
    }
#pragma GCC diagnostic pop
        break;

    default:
        // failure (cannot convert)
        return false;

    }
    f_type = node_t::NODE_STRING;

    return true;
}


void Node::to_videntifier()
{
    if(node_t::NODE_IDENTIFIER != f_type)
    {
        throw exception_internal_error("to_videntifier() called with a node other than a NODE_IDENTIFIER node");
    }

    f_type = node_t::NODE_VIDENTIFIER;
}


void Node::to_var_attributes()
{
    if(node_t::NODE_VARIABLE != f_type)
    {
        throw exception_internal_error("to_var_attribute() called with a node other than a NODE_VARIABLE node");
    }

    f_type = node_t::NODE_VAR_ATTRIBUTES;
}


void Node::set_boolean(bool value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case node_t::NODE_TRUE:
    case node_t::NODE_FALSE:
        break;

    default:
        throw exception_internal_error("set_boolean() called with a non-Boolean node type");

    }

    f_type = value ? node_t::NODE_TRUE : node_t::NODE_FALSE;
}


void Node::set_int64(Int64 value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case node_t::NODE_INT64:
        break;

    default:
        throw exception_internal_error("set_int64() called with a non-int64 node type");

    }

    f_int = value;
}


void Node::set_float64(Float64 value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case node_t::NODE_FLOAT64:
        break;

    default:
        throw exception_internal_error("set_float64() called with a non-float64 node type");

    }

    f_float = value;
}


void Node::set_string(String const& value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case node_t::NODE_BREAK:        // name of label
    case node_t::NODE_CLASS:        // name of class
    case node_t::NODE_CONTINUE:     // name of label
    case node_t::NODE_IMPORT:       // name of package
    case node_t::NODE_NAMESPACE:    // name of namespace
    case node_t::NODE_PACKAGE:      // name of package
    case node_t::NODE_STRING:
        break;

    default:
        throw exception_internal_error("set_string() called with a non-string node type");

    }

    f_str = value;
}


bool Node::get_boolean() const
{
    // only the corresponding node type accepts a get() call
    switch(f_type)
    {
    case node_t::NODE_TRUE:
        return true;

    case node_t::NODE_FALSE:
        return false;

    default:
        throw exception_internal_error("get_boolean() called with a non-Boolean node type");

    }
    /*NOTREACHED*/
}


Int64 Node::get_int64() const
{
    // only the corresponding node type accepts a get() call
    switch(f_type)
    {
    case node_t::NODE_INT64:
        break;

    default:
        throw exception_internal_error("get_int64() called with a non-int64 node type");

    }

    return f_int;
}


Float64 Node::get_float64() const
{
    // only the corresponding node type accepts a get() call
    switch(f_type)
    {
    case node_t::NODE_FLOAT64:
        break;

    default:
        throw exception_internal_error("get_float64() called with a non-float64 node type");

    }

    return f_float;
}


String const& Node::get_string() const
{
    // only the corresponding node type accepts a get() call
    switch(f_type)
    {
    case node_t::NODE_BREAK:        // name of label
    case node_t::NODE_CLASS:        // name of class
    case node_t::NODE_CONTINUE:     // name of label
    case node_t::NODE_IMPORT:       // name of package
    case node_t::NODE_NAMESPACE:    // name of namespace
    case node_t::NODE_PACKAGE:      // name of package
    case node_t::NODE_STRING:
        break;

    default:
        throw exception_internal_error("get_string() called with a non-string node type");

    }

    return f_str;
}


/** \brief Create a new node with the given type.
 *
 * This function creates a new node that is expected to be used as a
 * replacement of this node.
 *
 * Note that this node does not get modified by this call.
 *
 * \param[in] type  The type of the new node.
 *
 * \return A new node pointer.
 */
Node::pointer_t Node::create_replacement(node_t type) const
{
    Node::pointer_t n(new Node(type));

    // this is why we want to have a function instead of doing new Node().
    n->f_position = f_position;

    return n;
}


/** \brief Get the current status of a flag.
 *
 * This function returns true or false depending on the current status
 * of the specified flag.
 *
 * The function verifies that the specified flag (\p f) corresponds to
 * the type of data you are dealing with.
 *
 * If the flag was never set, this function returns false.
 *
 * \param[in] f  The flag to retrieve.
 *
 * \return true if the flag was set, false otherwise.
 */
bool Node::get_flag(flag_t f) const
{
    verify_flag(f);
    return f_flags[static_cast<size_t>(f)];
}


/** \brief Set a flag.
 *
 * This function sets the specified flag \p f to the specified value \p v
 * in this Node object.
 *
 * The function verifies that the specified flag (\p f) corresponds to
 * the type of data you are dealing with.
 *
 * \param[in] f  The flag to set.
 * \param[in] v  The new value for the flag.
 */
void Node::set_flag(flag_t f, bool v)
{
    verify_flag(f);
    f_flags[static_cast<size_t>(f)] = v;
}


/** \brief Verify that f corresponds to the node type.
 *
 * This function verifies that \p f corresponds to a valid flag according
 * to the type of this Node object.
 *
 * \param[in] f  The flag to check.
 */
void Node::verify_flag(flag_t f) const
{
    switch(f)
    {
    case flag_t::NODE_CATCH_FLAG_TYPED:
        if(f_type != node_t::NODE_CATCH)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES:
        if(f_type != node_t::NODE_DIRECTIVE_LIST)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_FOR_FLAG_FOREACH:
        if(f_type != node_t::NODE_FOR)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_FUNCTION_FLAG_GETTER:
    case flag_t::NODE_FUNCTION_FLAG_SETTER:
    case flag_t::NODE_FUNCTION_FLAG_OUT:
    case flag_t::NODE_FUNCTION_FLAG_VOID:
    case flag_t::NODE_FUNCTION_FLAG_NEVER:
    case flag_t::NODE_FUNCTION_FLAG_NOPARAMS:
    case flag_t::NODE_FUNCTION_FLAG_OPERATOR:
        if(f_type != node_t::NODE_FUNCTION)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_IDENTIFIER_FLAG_WITH:
    case flag_t::NODE_IDENTIFIER_FLAG_TYPED:
        if(f_type != node_t::NODE_IDENTIFIER
        && f_type != node_t::NODE_VIDENTIFIER
        && f_type != node_t::NODE_STRING)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_IMPORT_FLAG_IMPLEMENTS:
        if(f_type != node_t::NODE_IMPORT)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS:
    case flag_t::NODE_PACKAGE_FLAG_REFERENCED:
        if(f_type != node_t::NODE_PACKAGE)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED:
        if(f_type != node_t::NODE_PARAM_MATCH)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_PARAMETERS_FLAG_CONST:
    case flag_t::NODE_PARAMETERS_FLAG_IN:
    case flag_t::NODE_PARAMETERS_FLAG_OUT:
    case flag_t::NODE_PARAMETERS_FLAG_NAMED:
    case flag_t::NODE_PARAMETERS_FLAG_REST:
    case flag_t::NODE_PARAMETERS_FLAG_UNCHECKED:
    case flag_t::NODE_PARAMETERS_FLAG_UNPROTOTYPED:
    case flag_t::NODE_PARAMETERS_FLAG_REFERENCED:    // referenced from a parameter or a variable
    case flag_t::NODE_PARAMETERS_FLAG_PARAMREF:      // referenced from another parameter
    case flag_t::NODE_PARAMETERS_FLAG_CATCH:         // a parameter defined in a catch()
        if(f_type != node_t::NODE_PARAMETERS)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_SWITCH_FLAG_DEFAULT:           // we found a 'default:' label in that switch
        if(f_type != node_t::NODE_SWITCH)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_VAR_FLAG_CONST:
    case flag_t::NODE_VAR_FLAG_LOCAL:
    case flag_t::NODE_VAR_FLAG_MEMBER:
    case flag_t::NODE_VAR_FLAG_ATTRIBUTES:
    case flag_t::NODE_VAR_FLAG_ENUM:                 // there is a NODE_SET and it somehow needs to be copied
    case flag_t::NODE_VAR_FLAG_COMPILED:             // Expression() was called on the NODE_SET
    case flag_t::NODE_VAR_FLAG_INUSE:                // this variable was referenced
    case flag_t::NODE_VAR_FLAG_ATTRS:                // currently being read for attributes (to avoid loops)
    case flag_t::NODE_VAR_FLAG_DEFINED:              // was already parsed
    case flag_t::NODE_VAR_FLAG_DEFINING:             // currently defining, can't read
    case flag_t::NODE_VAR_FLAG_TOADD:                // to be added in the directive list
        if(f_type != node_t::NODE_VARIABLE
        && f_type != node_t::NODE_VAR
        && f_type != node_t::NODE_PARAM)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag()");
        }
        break;

    case flag_t::NODE_FLAG_max:
        throw exception_internal_error("invalid attribute / flag in Node::verify_flag()");

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }
}


/** \brief Get the current status of an attribute.
 *
 * This function returns true or false depending on the current status
 * of the specified attribute.
 *
 * The function verifies that the specified attribute (\p a) corresponds to
 * the type of data you are dealing with.
 *
 * If the attribute was never set, this function returns false.
 *
 * \param[in] a  The attribute to retrieve.
 *
 * \return true if the attribute was set, false otherwise.
 */
bool Node::get_attribute(attribute_t a) const
{
    verify_attribute(a);
    return f_attributes[static_cast<size_t>(a)];
}


/** \brief Set an attribute.
 *
 * This function sets the specified attribute \p a to the specified value \p v
 * in this Node object.
 *
 * The function verifies that the specified attribute (\p a) corresponds to
 * the type of data you are dealing with.
 *
 * \param[in] a  The flag to set.
 * \param[in] v  The new value for the flag.
 */
void Node::set_attribute(attribute_t a, bool v)
{
    verify_attribute(a);
    if(v)
    {
        verify_exclusive_attributes(a);
    }
    f_attributes[static_cast<size_t>(a)] = v;
}


/** \brief Verify that f corresponds to the data type.
 *
 * This function verifies that f corresponds to a valid flag according
 * to the type of this Node object.
 *
 * \param[in] f  The flag or attribute to check.
 */
void Node::verify_attribute(attribute_t f) const
{
    switch(f)
    {
    // member visibility
    case attribute_t::NODE_ATTR_PUBLIC:
    case attribute_t::NODE_ATTR_PRIVATE:
    case attribute_t::NODE_ATTR_PROTECTED:
    case attribute_t::NODE_ATTR_INTERNAL:

    // function member type
    case attribute_t::NODE_ATTR_STATIC:
    case attribute_t::NODE_ATTR_ABSTRACT:
    case attribute_t::NODE_ATTR_VIRTUAL:
    case attribute_t::NODE_ATTR_ARRAY:

    // function/variable is defined in your system (execution env.)
    case attribute_t::NODE_ATTR_INTRINSIC:

    // function/variable will be removed in future releases, do not use
    case attribute_t::NODE_ATTR_DEPRECATED:
    case attribute_t::NODE_ATTR_UNSAFE:

    // operator overload (function member)
    case attribute_t::NODE_ATTR_CONSTRUCTOR:

    // function & member constrains
    case attribute_t::NODE_ATTR_FINAL:
    case attribute_t::NODE_ATTR_ENUMERABLE:

    // conditional compilation
    case attribute_t::NODE_ATTR_TRUE:
    case attribute_t::NODE_ATTR_FALSE:
    case attribute_t::NODE_ATTR_UNUSED:                      // if definition is used, error!

    // class attribute (whether a class can be enlarged at run time)
    case attribute_t::NODE_ATTR_DYNAMIC:

    // switch attributes
    case attribute_t::NODE_ATTR_FOREACH:
    case attribute_t::NODE_ATTR_NOBREAK:
    case attribute_t::NODE_ATTR_AUTOBREAK:
        // TBD -- we'll need to see whether we want to limit the attributes
        //        on a per node type basis and how we can do that properly
        if(f_type == node_t::NODE_PROGRAM)
        {
            throw exception_internal_error("attribute / type missmatch in Node::verify_attribute()");
        }
        break;

    // attributes were defined
    case attribute_t::NODE_ATTR_DEFINED:
        // all nodes can receive this flag
        break;

    case attribute_t::NODE_ATTRIBUTE_max:
        throw exception_internal_error("invalid attribute / flag in Node::verify_attribute()");

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }
}


/** \brief Verify that we can indeed set an attribute.
 *
 * Many attributes are mutually exclusive. This function checks that
 * only one of a group of attributes gets set.
 *
 * This function is not called if you clear an attribute since in that
 * case the default applies.
 *
 * \param[in] a  The attribute being set.
 */
void Node::verify_exclusive_attributes(attribute_t a) const
{
    bool conflict(false);
    char const *names;
    switch(a)
    {
    case attribute_t::NODE_ATTR_ARRAY:
    case attribute_t::NODE_ATTR_DEPRECATED:
    case attribute_t::NODE_ATTR_UNSAFE:
    case attribute_t::NODE_ATTR_DEFINED:
    case attribute_t::NODE_ATTR_DYNAMIC:
    case attribute_t::NODE_ATTR_ENUMERABLE:
    case attribute_t::NODE_ATTR_FINAL:
    case attribute_t::NODE_ATTR_INTERNAL:
    case attribute_t::NODE_ATTR_INTRINSIC:
    case attribute_t::NODE_ATTR_UNUSED:
        // these attributes have no conflicts
        return;

    // member visibility
    case attribute_t::NODE_ATTR_PUBLIC:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PRIVATE)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PROTECTED)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_MEMBER_VISIBILITY];
        break;

    case attribute_t::NODE_ATTR_PRIVATE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PUBLIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PROTECTED)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_MEMBER_VISIBILITY];
        break;

    case attribute_t::NODE_ATTR_PROTECTED:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PUBLIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_PRIVATE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_MEMBER_VISIBILITY];
        break;

    // function type group
    case attribute_t::NODE_ATTR_STATIC:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_VIRTUAL)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_ABSTRACT:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_STATIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_VIRTUAL)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_VIRTUAL:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_STATIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    case attribute_t::NODE_ATTR_CONSTRUCTOR:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_STATIC)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_CONSTRUCTOR)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_ABSTRACT)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_FUNCTION_TYPE];
        break;

    // switch type group
    case attribute_t::NODE_ATTR_FOREACH:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NOBREAK)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_AUTOBREAK)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_SWITCH_TYPE];
        break;

    case attribute_t::NODE_ATTR_NOBREAK:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_FOREACH)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_AUTOBREAK)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_SWITCH_TYPE];
        break;

    case attribute_t::NODE_ATTR_AUTOBREAK:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_FOREACH)]
                || f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_NOBREAK)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_SWITCH_TYPE];
        break;

    // conditional compilation group
    case attribute_t::NODE_ATTR_TRUE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_FALSE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION];
        break;

    case attribute_t::NODE_ATTR_FALSE:
        conflict = f_attributes[static_cast<size_t>(attribute_t::NODE_ATTR_TRUE)];
        names = g_attribute_groups[ATTRIBUTES_GROUP_CONDITIONAL_COMPILATION];
        break;

    case attribute_t::NODE_ATTRIBUTE_max:
        throw exception_internal_error("invalid attribute / flag in Node::verify_attribute()");

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }

    if(conflict)
    {
        // this can be a user error so we emit an error instead of throwing
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_ATTRIBUTES, f_position);
        msg << "Attributes " << names << " are mutually exclusive. Only one of them can be used.";
    }
}


/** \brief Retrieve the switch operator.
 *
 * A switch statement can be constrained to use a specific operator
 * using the with() syntax as in:
 *
 * \code
 * switch(foo) with(===)
 * {
 *    ...
 * }
 * \endcode
 *
 * This operator is saved in the switch node and can later be retrieved
 * with this function.
 *
 * \exception exception_internal_error
 * If the function is called on a node of a type other than NODE_SWITCH
 * then this exception is raised.
 *
 * \return The operator of the switch statement, or NODE_UNKNOWN if undefined.
 */
Node::node_t Node::get_switch_operator() const
{
    if(node_t::NODE_SWITCH != f_type)
    {
        throw exception_internal_error("INTERNAL ERROR: set_switch_operator() called on a node which is not a switch node.");
    }

    return f_switch_operator;
}


/** \brief Set the switch statement operator.
 *
 * This function saves the operator defined following the switch statement
 * using the with() instruction as in:
 *
 * \code
 * switch(foo) with(===)
 * {
 *    ...
 * }
 * \endcode
 *
 * \exception exception_internal_error
 * If the function is called on a node of a type other than NODE_SWITCH
 * then this exception is raised.
 *
 * \param[in] op  The new operator to save in this switch statement.
 */
void Node::set_switch_operator(node_t op)
{
    if(node_t::NODE_SWITCH != f_type)
    {
        throw exception_internal_error("INTERNAL ERROR: set_switch_operator() called on a node which is not a switch node.");
    }

    f_switch_operator = op;
}


/** \brief Define the size of the parameter index and depth vectors.
 *
 * This function defines the size of the depth and index parameter
 * vectors. Until this function is called, trying to set a depth
 * or index parameter will fail.
 *
 * Also, the function cannot be called more than once and the size
 * parameter cannot be zero.
 *
 * \param[in] size  The number of parameters (size > 0 must be true).
 */
void Node::set_param_size(size_t size)
{
    if(f_param_depth.size() != 0)
    {
        throw exception_internal_error("INTERNAL ERROR: set_param_depth() called twice.");
    }
    if(size == 0)
    {
        throw exception_internal_error("INTERNAL ERROR: set_param_depth() was called with a size of zero.");
    }
    f_param_depth.resize(size);
    f_param_index.resize(size);
}


/** \brief Return the size of the parameter index and depth vectors.
 *
 * This function returns zero unless the set_param_size() was successfully
 * called with a valid size.
 *
 * \return The current size of the parameter index and depth vectors.
 */
size_t Node::get_param_size() const
{
    return f_param_depth.size();
}


/** \brief Get the depth at the specified index.
 *
 * This function returns the depth parameter at the specified index.
 *
 * \return The depth of the type of this parameter.
 */
Node::depth_t Node::get_param_depth(size_t idx) const
{
    return f_param_depth[idx];
}


/** \brief Set the depth of a parameter.
 *
 * When we search for a match of a function call, we check its parameters.
 * If a parameter has a higher class type definition, then it wins over
 * the others. This depth value represents that information.
 *
 * \param[in] j  The index of the parameter for which we define the depth.
 *               (The order is the function being called order.)
 * \param[in] depth  The new depth.
 */
void Node::set_param_depth(size_t j, depth_t depth)
{
    f_param_depth[j] = depth;
}


/** \brief Get the index of the parameter.
 *
 * When a user writes a function call, he can spell out the parameter
 * names as in:
 *
 * \code
 * pos = find(size => 123, characer => 'c', haystack => str);
 * \endcode
 *
 * Those parameters, in the function definition, may not be in the
 * same order:
 *
 * \code
 * function find(haystack: string, character: string, size: number = -1);
 * \endcode
 *
 * The parameter index vector holds the indices so we can reorganize the
 * call as in:
 *
 * \code
 * pos = find(str, 'c', 123);
 * \endcode
 *
 * The really cool thing is that you could call a function with
 * multiple definitions and still get the parameters in the right
 * order even though both functions define their parameters
 * in a different order.
 *
 * \param[in] j  The index of the parameter in the function being called.
 *
 * \return The index in the function definition.
 */
size_t Node::get_param_index(size_t j) const
{
    return f_param_index[j];
}


/** \brief Set the parameter index.
 *
 * Save the index of the parameter in the function being called, opposed
 * to the index of the parameter in the function call.
 *
 * \param[in] idx  The index in the function call.
 * \param[in] j  The index in the function being called.
 */
void Node::set_param_index(size_t idx, size_t j)
{
    f_param_index[idx] = j;
}


void Node::set_position(Position const& position)
{
    f_position = position;
}


Position const& Node::get_position() const
{
    return f_position;
}


char const *Node::operator_to_string(node_t op)
{
#if defined(_DEBUG) || defined(DEBUG)
    {
        // make sure that the node types are properly sorted
        static bool checked = false;
        if(!checked)
        {
            // check only once
            checked = true;
            for(size_t idx = 1; idx < g_operator_to_string_size; ++idx)
            {
                if(g_operator_to_string[idx].f_node <= g_operator_to_string[idx - 1].f_node)
                {
                    std::cerr << "INTERNAL ERROR at offset " << idx
                              << " (line #" << g_operator_to_string[idx].f_line
                              << ", node type " << static_cast<uint32_t>(g_operator_to_string[idx].f_node)
                              << " vs. " << static_cast<uint32_t>(g_operator_to_string[idx - 1].f_node)
                              << "): the g_operator_to_string table isn't sorted properly. We can't binary search it."
                              << std::endl;
                    throw exception_internal_error("INTERNAL ERROR: node types not properly sorted, cannot properly search for operators using a binary search.");
                }
            }
        }
    }
#endif

    size_t i, j, p;
    int    r;

    i = 0;
    j = g_operator_to_string_size;
    while(i < j)
    {
        p = (j - i) / 2 + i;
        r = static_cast<int>(g_operator_to_string[p].f_node) - static_cast<int>(op);
        if(r == 0)
        {
            return g_operator_to_string[p].f_name;
        }
        if(r < 0)
        {
            i = p + 1;
        }
        else
        {
            j = p;
        }
    }

    return 0;
}



Node::node_t Node::string_to_operator(String const& str)
{
    for(size_t idx(0); idx < g_operator_to_string_size; ++idx)
    {
        if(str == g_operator_to_string[idx].f_name)
        {
            return g_operator_to_string[idx].f_node;
        }
    }

    return node_t::NODE_UNKNOWN;
}



/** \brief Replace this node with the \p node parameter.
 *
 * This function replaces this node with the specified node. This is used
 * a lot in the optimizer and once in the compiler.
 *
 * It is useful in a case such as an if() statement that has a resulting
 * Boolean value which is known at compile time. For example:
 *
 * \code
 *  if(true)
 *      blah;
 *  else
 *      foo;
 * \endcode
 *
 * can be optimized by just this:
 *
 * \code
 *  blah;
 * \endcode
 *
 * In that case what we do is replace the NODE_IF (this) with the content
 * of the 'blah' node.
 *
 * This function is very similar to the set_child() when you do not know
 * the index position of this node in its parent.
 *
 * \warning
 * This function modifies the tree in a why that may break loops over
 * node children.
 *
 * \param[in] node  The node to replace this node with.
 */
void Node::replace_with(pointer_t node)
{
    f_parent->set_child(get_offset(), node);
}


/** \brief This function sets the parent of a node.
 *
 * This function is the only function that handles the tree of nodes,
 * in other words, the only one that modifies the f_parent and
 * f_children pointers. It is done that way to make 100% sure (assuming
 * it is itself correct) that we do not mess up the tree.
 *
 * This node loses its current parent, and thus is removed from the
 * list of children of that parent. Then is is assigned the new
 * parent as passed to this function.
 *
 * If an index is specified, the child is inserted at that specific
 * location. Otherwise the child is appended.
 *
 * The function does nothing if the current parent is the same as the
 * new parent and the default index is used (-1).
 *
 * Use an index of 0 to insert the item at the start of the list of children.
 * Use an index of get_children_size() to force the child at the end of the
 * list even if the parent remains the same.
 *
 * Helper functions are available to make more sense of the usage of this
 * function:
 *
 * \li delete_child() -- delete a child at that specific index.
 * \li append_child() -- append a child to this parent.
 * \li insert_child() -- insert a child to this parent.
 * \li set_child() -- replace a child with another in this parent.
 *
 * \param[in] parent  The new parent of the node. May be set to nullptr.
 * \param[in] index  The position where the new item is inserted in the parent
 *                   array of children.
 */
void Node::set_parent(pointer_t parent, int index)
{
    modifying();

    // already a child of that parent?
    // (although in case of an insert, we force the re-parent
    // to the right location)
    if(parent == f_parent && index == -1)
    {
        return;
    }

    if(f_parent)
    {
        // very similar to the get_offset() call only we want the iterator
        // in this case, not the index
        pointer_t me(shared_from_this());
        vector_of_pointers_t::iterator it(std::find(f_parent->f_children.begin(), f_parent->f_children.end(), me));
        if(it == f_parent->f_children.end())
        {
            throw exception_internal_error("trying to remove a child from a parent which does not know about that child");
        }
        f_parent->f_children.erase(it);
        f_parent.reset();
    }

    if(parent)
    {
        if(index == -1)
        {
            parent->f_children.push_back(shared_from_this());
        }
        else
        {
            if(static_cast<size_t>(index) > f_children.size())
            {
                throw exception_index_out_of_range("trying to insert a node at the wrong position");
            }
            parent->f_children.insert(f_children.begin() + index, shared_from_this());
        }
        f_parent = parent;
    }
}


Node::pointer_t Node::get_parent() const
{
    return f_parent;
}


size_t Node::get_children_size() const
{
    return f_children.size();
}


void Node::delete_child(int index)
{
    modifying();

    // remove the node from the parent, but the node itself does not
    // actually get deleted (that part is expected to be automatic
    // because of the shared pointers)
    //
    // HOWEVER, the vector changes making the index invalid after that!
    f_children[index]->set_parent();
}


void Node::append_child(pointer_t child)
{
    child->set_parent(shared_from_this());
}


void Node::insert_child(int index, pointer_t child)
{
    modifying();

    child->set_parent(shared_from_this(), index);
}


/** \brief Replace the current child at position \p index with \p child.
 *
 * This function replace the child in this node and at \p index with
 * the new specified \p child.
 *
 * \param[in] index  The position where the new child is to be placed.
 * \param[in] child  The new child replacing the existing child at \p index.
 */
void Node::set_child(int index, pointer_t child)
{
    modifying();

    delete_child(index);
    insert_child(index, child);
}


Node::vector_of_pointers_t const& Node::get_children() const
{
    return f_children;
}


Node::pointer_t Node::get_child(int index) const
{
    return f_children[index];
}


Node::pointer_t Node::find_first_child(node_t type) const
{
    Node::pointer_t child;
    return find_next_child(child, type);
}


Node::pointer_t Node::find_next_child(pointer_t child, node_t type) const
{
    size_t const max(f_children.size());
    for(size_t idx(0); idx < max; ++idx)
    {
        // if child is defined, skip up to it first
        if(child && child == f_children[idx])
        {
            child.reset();
        }
        else if(f_children[idx]->get_type() == type)
        {
            return f_children[idx];
        }
    }

    // not found...
    return pointer_t();
}


/** \brief Remove all unknown nodes.
 *
 * This function goes in the entire tree starting at this node and
 * remove all the children that are marked as NODE_UNKNOWN.
 *
 * This allows many functions to clear out many nodes without
 * having to have very special handling of their loops while
 * scanning all the children of a node.
 *
 * \note
 * The nodes themselves do not get deleted by this function. If
 * it was their last reference then it will be deleted by the
 * shared pointer code.
 */
void Node::clean_tree()
{
    size_t idx(f_children.size());
    while(idx > 0)
    {
        --idx;
        if(f_children[idx]->get_type() == node_t::NODE_UNKNOWN)
        {
            delete_child(idx);
        }
        else
        {
            f_children[idx]->clean_tree();  // recursive
        }
    }
}


/** \brief Find the offset of this node in its parent array of children.
 *
 * This function searches for a node in its parent list of children and
 * returns the corresponding index so we can apply functions to that
 * child from the parent.
 *
 * \return The offset (index, position) of the child in its parent
 *         f_children vector.
 */
size_t Node::get_offset() const
{
    if(!f_parent)
    {
        // no parent
        throw exception_no_parent("get_offset() only works against nodes that have a parent.");
    }

    pointer_t me = const_cast<Node *>(this)->shared_from_this();
    vector_of_pointers_t::iterator it(std::find(f_parent->f_children.begin(), f_parent->f_children.end(), me));
    if(it == f_parent->f_children.end())
    {
        // if this happen, we have a bug in the set_parent() function
        throw exception_internal_error("get_offset() could not find this node in its parent");
    }

    // found ourselves in our parent
    return it - f_parent->f_children.begin();
}


void Node::set_link(link_t index, pointer_t link)
{
    modifying();

    if(index >= link_t::LINK_max)
    {
        throw exception_index_out_of_range("set_link() called with an index out of bounds.");
    }

    // make sure the size is reserved on first set
    if(f_link.empty())
    {
        f_link.resize(static_cast<vector_of_pointers_t::size_type>(link_t::LINK_max));
    }

    if(link)
    {
        // link already set?
        if(f_link[static_cast<size_t>(index)])
        {
            throw exception_internal_error("a link was set twice at the same offset");
        }

        f_link[static_cast<size_t>(index)] = link;
    }
    else
    {
        f_link[static_cast<size_t>(index)].reset();
    }
}


Node::pointer_t Node::get_link(link_t index)
{
    if(index >= link_t::LINK_max)
    {
        throw exception_index_out_of_range("get_link() called with an index out of bounds.");
    }

    if(f_link.empty())
    {
        return nullptr;
    }

    return f_link[static_cast<size_t>(index)];
}


bool Node::has_side_effects() const
{
    //
    // Well... I'm wondering if we can really
    // trust this current version.
    //
    // Problem I:
    //    some identifiers can be getters and
    //    they can have side effects; though
    //    a getter should be considered constant
    //    toward the object being read and thus
    //    it should be fine in 99% of cases
    //    [imagine a serial number generator...]
    //
    // Problem II:
    //    some operators may not have been
    //    compiled yet and they could have
    //    side effects too; now this is much
    //    less likely a problem because then
    //    the programmer is most certainly
    //    creating a really weird program
    //    with all sorts of side effects that
    //    he wants no one else to know about,
    //    etc. etc. etc.
    //
    // Problem III:
    //    Note that we do not memorize whether
    //    a node has side effects because its
    //    children may change and then the side
    //    effects may disappear
    //

    switch(f_type)
    {
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_ASSIGNMENT_ADD:
    case node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node_t::NODE_ASSIGNMENT_DIVIDE:
    case node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case node_t::NODE_ASSIGNMENT_MAXIMUM:
    case node_t::NODE_ASSIGNMENT_MINIMUM:
    case node_t::NODE_ASSIGNMENT_MODULO:
    case node_t::NODE_ASSIGNMENT_MULTIPLY:
    case node_t::NODE_ASSIGNMENT_POWER:
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_ASSIGNMENT_SUBTRACT:
    case node_t::NODE_CALL:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DELETE:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_NEW:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
        return true;

    //case NODE_IDENTIFIER:
    //
    // TODO: Test whether this is a reference to a getter
    //       function (needs to be compiled already...)
    //    
    //    break;

    default:
        break;

    }

    for(size_t idx = 0; idx < f_children.size(); ++idx)
    {
        if(f_children[idx] && f_children[idx]->has_side_effects())
        {
            return true;
        }
    }

    return false;
}


void Node::modifying() const
{
    if(f_lock != 0)
    {
        throw exception_locked_node("trying to modify a locked node.");
    }
}


bool Node::is_locked() const
{
    return f_lock != 0;
}


void Node::lock()
{
    ++f_lock;
}


void Node::unlock()
{
    if(f_lock <= 0)
    {
        throw exception_internal_error("somehow the Node::unlock() function was called when the lock counter is zero");
    }

    --f_lock;
}


void Node::add_variable(pointer_t variable)
{
    if(node_t::NODE_VARIABLE != variable->f_type)
    {
        throw exception_incompatible_node_type("invalid type of node to call add_variable() with");
    }
    // TODO: test the destination (i.e. this) to make sure only valid nodes
    //       accept variables

    f_variables.push_back(variable);
}


size_t Node::get_variable_size() const
{
    return f_variables.size();
}


Node::pointer_t Node::get_variable(int index) const
{
    return f_variables[index];
}


void Node::add_label(pointer_t label)
{
    if(node_t::NODE_LABEL != label->f_type)
    {
        throw exception_incompatible_node_type("invalid type of node to call add_label() with");
    }
    // TODO: test the destination (i.e. this) to make sure only valid nodes
    //       accept variables

    f_labels[label->f_str] = label;
}


size_t Node::get_label_size() const
{
    return f_labels.size();
}


//Node::pointer_t Node::get_label(size_t index) const
//{
//    if(index >= f_labels.size())
//    {
//        throw exception_index_out_of_range("get_label() called with an invalid index");
//    }
//
//    return *(f_labels.begin() + index);
//}


Node::pointer_t Node::find_label(String const& name) const
{
    map_of_pointers_t::const_iterator it(f_labels.find(name));
    return it == f_labels.end() ? pointer_t() : it->second;
}











/**********************************************************************/
/**********************************************************************/
/***  NODE DISPLAY  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void Node::display_data(std::ostream& out) const
{
    struct sub_function
    {
        static void display_str(std::ostream& out, String str)
        {
            out << ": '";
            for(as_char_t const *s(str.c_str()); *s != '\0'; ++s)
            {
                if(*s < 0x7f)
                {
                    if(*s == '\'')
                    {
                        out << "\\'";
                    }
                    else
                    {
                        out << static_cast<char>(*s);
                    }
                }
                else
                {
                    out << "\\U+" << std::hex << *s << std::dec;
                }
            }
            out << "'";
        }
    };

    out << std::setw(4) << std::setfill('0') << static_cast<node_t>(f_type) << std::setfill('\0') << ": " << get_type_name();
    if(static_cast<int>(static_cast<node_t>(f_type)) > ' ' && static_cast<int>(static_cast<node_t>(f_type)) < 0x7F)
    {
        out << " = '" << static_cast<char>(static_cast<node_t>(f_type)) << "'";
    }

    switch(f_type)
    {
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_STRING:
    case node_t::NODE_GOTO:
    case node_t::NODE_LABEL:
    case node_t::NODE_IMPORT:
    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_ENUM:
        sub_function::display_str(out, f_str);
        break;

    case node_t::NODE_PACKAGE:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS)])
        {
            out << " FOUND-LABELS";
        }
        break;

    case node_t::NODE_INT64:
        out << ": " << f_int.get() << ", 0x" << std::hex << std::setw(16) << std::setfill('0') << f_int.get() << std::dec << std::setw(0) << std::setfill('\0');
        break;

    case node_t::NODE_FLOAT64:
        out << ": " << f_float.get();
        break;

    case node_t::NODE_FUNCTION:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_GETTER)])
        {
            out << " GETTER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_SETTER)])
        {
            out << " SETTER";
        }
        break;

    case node_t::NODE_PARAM:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_IN)])
        {
            out << " IN";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_OUT)])
        {
            out << " OUT";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_NAMED)])
        {
            out << " NAMED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_REST)])
        {
            out << " REST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_UNCHECKED)])
        {
            out << " UNCHECKED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_UNPROTOTYPED)])
        {
            out << " UNPROTOTYPED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_REFERENCED)])
        {
            out << " REFERENCED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_PARAMREF)])
        {
            out << " PARAMREF";
        }
        break;

    case node_t::NODE_PARAM_MATCH:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED)])
        {
            out << " UNPROTOTYPED";
        }
        break;

    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_LOCAL)])
        {
            out << " LOCAL";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_MEMBER)])
        {
            out << " MEMBER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_ATTRIBUTES)])
        {
            out << " ATTRIBUTES";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_ENUM)])
        {
            out << " ENUM";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_COMPILED)])
        {
            out << " COMPILED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_INUSE)])
        {
            out << " INUSE";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_ATTRS)])
        {
            out << " ATTRS";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_DEFINED)])
        {
            out << " DEFINED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_DEFINING)])
        {
            out << " DEFINING";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_TOADD)])
        {
            out << " TOADD";
        }
        break;

    default:
        break;

    }

    //size_t const size = f_user_data.size();
    //if(size > 0)
    //{
    //    out << " Raw Data:" << std::hex << std::setfill('0');
    //    for(size_t idx(0); idx < size; ++idx)
    //    {
    //        out << " " << std::setw(8) << f_user_data[idx];
    //    }
    //    out << std::dec << std::setfill('\0');
    //}
}


void Node::display(std::ostream& out, int indent, pointer_t parent, char c) const
{
    // this pointer
    out << this << ":" << std::setfill('\0') << std::setw(2) << indent << c << " " << std::setw(indent) << "";

    // verify parent
    if(parent != f_parent)
    {
        out << ">>WRONG PARENT: " << f_parent.get() << " vs " << parent.get() << "<< ";
    }

    // display node data (integer, string, float, etc.)
    display_data(out);

    // display information about the links
    bool first = true;
    for(size_t lnk(0); lnk < f_link.size(); ++lnk)
    {
        if(f_link[lnk])
        {
            if(first)
            {
                first = false;
                out << " Lnk:";
            }
            out << " [" << lnk << "%d]=" << f_link[lnk].get();
        }
    }

    // display the different attributes if any
    struct display_attributes
    {
        display_attributes(std::ostream& out, attribute_set_t attrs)
            : f_out(out)
            , f_attributes(attrs)
        {
            display_attribute(attribute_t::NODE_ATTR_PUBLIC,         "PUBLIC"        );
            display_attribute(attribute_t::NODE_ATTR_PRIVATE,        "PRIVATE"       );
            display_attribute(attribute_t::NODE_ATTR_PROTECTED,      "PROTECTED"     );
            display_attribute(attribute_t::NODE_ATTR_STATIC,         "STATIC"        );
            display_attribute(attribute_t::NODE_ATTR_ABSTRACT,       "ABSTRACT"      );
            display_attribute(attribute_t::NODE_ATTR_VIRTUAL,        "VIRTUAL"       );
            display_attribute(attribute_t::NODE_ATTR_INTERNAL,       "INTERNAL"      );
            display_attribute(attribute_t::NODE_ATTR_INTRINSIC,      "INTRINSIC"     );
            display_attribute(attribute_t::NODE_ATTR_DEPRECATED,     "DEPRECATED"    );
            display_attribute(attribute_t::NODE_ATTR_UNSAFE,         "UNSAFE"        );
            display_attribute(attribute_t::NODE_ATTR_CONSTRUCTOR,    "CONSTRUCTOR"   );
            display_attribute(attribute_t::NODE_ATTR_FINAL,          "FINAL"         );
            display_attribute(attribute_t::NODE_ATTR_ENUMERABLE,     "ENUMERABLE"    );
            display_attribute(attribute_t::NODE_ATTR_TRUE,           "TRUE"          );
            display_attribute(attribute_t::NODE_ATTR_FALSE,          "FALSE"         );
            display_attribute(attribute_t::NODE_ATTR_UNUSED,         "UNUSED"        );
            display_attribute(attribute_t::NODE_ATTR_DYNAMIC,        "DYNAMIC"       );
            display_attribute(attribute_t::NODE_ATTR_FOREACH,        "FOREACH"       );
            display_attribute(attribute_t::NODE_ATTR_NOBREAK,        "NOBREAK"       );
            display_attribute(attribute_t::NODE_ATTR_AUTOBREAK,      "AUTOBREAK"     );
            display_attribute(attribute_t::NODE_ATTR_DEFINED,        "DEFINED"       );
        }

        void display_attribute(attribute_t a, char const *n)
        {
            if(f_attributes[static_cast<size_t>(a)])
            {
                f_out << " " << n;
            }
        }

        std::ostream&               f_out;
        controlled_vars::fbool_t    f_first;
        attribute_set_t             f_attributes;
    } display_attr(out, f_attributes);

    // end the line with our position
    out << " " << f_position << std::endl;

    // now print 
    pointer_t me = const_cast<Node *>(this)->shared_from_this();
    for(size_t idx(0); idx < f_children.size(); ++idx)
    {
        f_children[idx]->display(out, indent + 1, me, '-');
    }
    pointer_t null_ptr;
    for(size_t idx(0); idx < f_variables.size(); ++idx)
    {
        f_variables[idx]->display(out, indent + 1, null_ptr, '=');
    }
    for(map_of_pointers_t::const_iterator it(f_labels.begin());
                                          it != f_labels.end();
                                          ++it)
    {
        it->second->display(out, indent + 1, null_ptr, ':');
    }
}




std::ostream& operator << (std::ostream& out, Node const& node)
{
    node.display(out, 2, node.get_parent(), '.');
    return out;
}




NodeLock::NodeLock(Node::pointer_t node)
    : f_node(node)
{
    if(f_node)
    {
        f_node->lock();
    }
}


NodeLock::~NodeLock()
{
    unlock();
}


void NodeLock::unlock()
{
    if(f_node)
    {
        f_node->unlock();
        f_node.reset();
    }
}


}
// namespace as2js

// vim: ts=4 sw=4 et
