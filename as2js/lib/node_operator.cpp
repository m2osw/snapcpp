/* node_operator.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE OPERATOR  **************************************************/
/**********************************************************************/
/**********************************************************************/

namespace
{


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
    { Node::node_t::NODE_ASSIGNMENT_ADD,                  "+=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_BITWISE_AND,          "&=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_BITWISE_OR,           "|=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR,          "^=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_DIVIDE,               "/=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND,          "&&=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR,           "||=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR,          "^^=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MAXIMUM,              ">?=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MINIMUM,              "<?=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MODULO,               "%=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_MULTIPLY,             "*=",   __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_POWER,                "**=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT,          "<%=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT,         ">%=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT,           "<<=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT,          ">>=",  __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED, ">>>=", __LINE__ },
    { Node::node_t::NODE_ASSIGNMENT_SUBTRACT,             "-=",   __LINE__ },
    { Node::node_t::NODE_CALL,                            "()",   __LINE__ },
    { Node::node_t::NODE_COMPARE,                         "<=>",  __LINE__ },
    { Node::node_t::NODE_DECREMENT,                       "--x",  __LINE__ },
    { Node::node_t::NODE_EQUAL,                           "==",   __LINE__ },
    { Node::node_t::NODE_GREATER_EQUAL,                   ">=",   __LINE__ },
    { Node::node_t::NODE_INCREMENT,                       "++x",  __LINE__ },
    { Node::node_t::NODE_LESS_EQUAL,                      "<=",   __LINE__ },
    { Node::node_t::NODE_LOGICAL_AND,                     "&&",   __LINE__ },
    { Node::node_t::NODE_LOGICAL_OR,                      "||",   __LINE__ },
    { Node::node_t::NODE_LOGICAL_XOR,                     "^^",   __LINE__ },
    { Node::node_t::NODE_MATCH,                           "~=",   __LINE__ },
    { Node::node_t::NODE_MAXIMUM,                         ">?",   __LINE__ },
    { Node::node_t::NODE_MINIMUM,                         "<?",   __LINE__ },
    { Node::node_t::NODE_NOT_EQUAL,                       "!=",   __LINE__ },
    { Node::node_t::NODE_NOT_MATCH,                       "!~",   __LINE__ },
    { Node::node_t::NODE_POST_DECREMENT,                  "x--",  __LINE__ },
    { Node::node_t::NODE_POST_INCREMENT,                  "x++",  __LINE__ },
    { Node::node_t::NODE_POWER,                           "**",   __LINE__ },
    { Node::node_t::NODE_ROTATE_LEFT,                     "<%",   __LINE__ },
    { Node::node_t::NODE_ROTATE_RIGHT,                    ">%",   __LINE__ },
    { Node::node_t::NODE_SHIFT_LEFT,                      "<<",   __LINE__ },
    { Node::node_t::NODE_SHIFT_RIGHT,                     ">>",   __LINE__ },
    { Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED,            ">>>",  __LINE__ },
    { Node::node_t::NODE_SMART_MATCH,                     "~~",   __LINE__ },
    { Node::node_t::NODE_STRICTLY_EQUAL,                  "===",  __LINE__ },
    { Node::node_t::NODE_STRICTLY_NOT_EQUAL,              "!==",  __LINE__ }

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

}
// no name namespace



/** \brief Transform an operator to a string.
 *
 * This function transform the specified operator (\p op) to a
 * printable string. It is generaly used to print out an error
 * message.
 *
 * If the function cannot find the operator, then it returns a
 * null pointer.
 *
 * \param[in] op  The operator to convert to a string.
 *
 * \return A basic null terminated C string with the operator name or nullptr.
 */
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
                    std::cerr << "INTERNAL ERROR at offset " << idx                                                     // LCOV_EXCL_LINE
                              << " (line #" << g_operator_to_string[idx].f_line                                         // LCOV_EXCL_LINE
                              << ", node type " << static_cast<uint32_t>(g_operator_to_string[idx].f_node)              // LCOV_EXCL_LINE
                              << " vs. " << static_cast<uint32_t>(g_operator_to_string[idx - 1].f_node)                 // LCOV_EXCL_LINE
                              << "): the g_operator_to_string table isn't sorted properly. We can't binary search it."  // LCOV_EXCL_LINE
                              << std::endl;                                                                             // LCOV_EXCL_LINE
                    throw exception_internal_error("INTERNAL ERROR: node types not properly sorted, cannot properly search for operators using a binary search."); // LCOV_EXCL_LINE
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

    return nullptr;
}


/** \brief Transform a string in an operator.
 *
 * The user may declare operators in his classes. Because of that
 * the lexer returns identifiers and strings that need to later be
 * converted to an operator. This function is used for this purpose.
 *
 * If the operator is invalid, then the function returns NODE_UNKNOWN.
 *
 * \param[in] str  The string representing the operator to convert.
 *
 * \return The node type representing this operator.
 */
Node::node_t Node::string_to_operator(String const& str)
{
    for(size_t idx(0); idx < g_operator_to_string_size; ++idx)
    {
        // not sorted by name so we use a slow poke search...
        if(str == g_operator_to_string[idx].f_name)
        {
            return g_operator_to_string[idx].f_node;
        }
    }

    if(str == "<>")
    {
        // this is an overload of the '!='
        return node_t::NODE_NOT_EQUAL;
    }
    if(str == ":=")
    {
        // this is an overload of the '='
        return node_t::NODE_ASSIGNMENT;
    }

    return node_t::NODE_UNKNOWN;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
