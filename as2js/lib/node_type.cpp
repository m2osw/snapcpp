/* node_type.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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


}
// no name namespace


/** \brief Retrieve the type of the node.
 *
 * This function gets the type of the node and returns it. The type
 * is one of the node_t::NODE_... values.
 *
 * Note the value of the node types are not all sequencial. The lower
 * portion used one to one with characters has many sparse places.
 * However, the node constructor ensures that only valid strings get
 * created.
 *
 * There are some functions available to convert a certain number of
 * nodes. These are used by the compiler and optimizer to implement
 * their functions.
 *
 * \li to_unknown() -- change any node to NODE_UNKNOWN
 * \li to_as() -- change a NODE_CALL to a NODE_AS
 * \li to_boolean_type_only() -- check whether a node represents NODE_TRUE
 *                               or NODE_FALSE
 * \li to_boolean() -- change to a NODE_TRUE or NODE_FALSE if possible
 * \li to_call() -- change a getter or setter to a NODE_CALL
 * \li to_int64() -- force a number to a NODE_INT64
 * \li to_float64() -- force a number to a NODE_FLOAT64
 * \li to_number() -- change a string to a NODE_FLOAT64
 * \li to_string() -- change a number to a NODE_STRING
 * \li to_videntifier() -- change a NODE_IDENTIFIER to a NODE_VIDENTIFIER
 * \li to_var_attributes() -- change a NODE_VARIABLE to a NODE_VAR_ATTRIBUTES
 *
 * \return The current type of the node.
 */
Node::node_t Node::get_type() const
{
    return f_type;
}


/** \breif Convert the type of 'this' node to a string.
 *
 * The type of the node (NODE_...) can be retrieved as a string using this
 * function. In pretty much all cases this is done whenever an error occurs
 * and not in normal circumstances. It is also used to debug the node tree.
 *
 * \return A null terminated C-like string with the node name.
 */
char const *Node::get_type_name() const
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

    throw exception_internal_error("INTERNAL ERROR: node type name not found!?."); // LCOV_EXCL_LINE
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


/** \brief Check whether a node has side effects.
 *
 * This function checks whether a node, or any of its children, has a
 * side effect.
 *
 * Having a side effect means that the function of the node is to modify
 * something. For example an assignment modifies its destination which
 * is an obvious side effect.
 *
 * The test is run against this node and all of its children because if
 * any one node implies a modification, the tree as a whole implies a
 * modification and thus the function must return true.
 *
 * \return true if this node has a side effect.
 */
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
    //    [imagine a serial number generator
    //    though...]
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
    //    children may change and then side
    //    effects may appear and disappear.
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

    for(size_t idx(0); idx < f_children.size(); ++idx)
    {
        if(f_children[idx] && f_children[idx]->has_side_effects())
        {
            return true;
        }
    }

    return false;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
