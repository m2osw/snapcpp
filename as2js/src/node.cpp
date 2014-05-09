/* node.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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

#include    <algorithm>
#include    <cmath>
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
};

#define    TO_STR_sub(s)            #s
#define    NODE_TYPE_NAME(node)     { Node::NODE_##node, TO_STR_sub(node) }

#if defined(_DEBUG) || defined(DEBUG)
g_node_type_name_file_line = __LINE__; // for errors detected in this .cpp file
#endif
type_name_t const g_node_type_name[] =
{
    NODE_TYPE_NAME(EOF),
    NODE_TYPE_NAME(UNKNOWN),
    NODE_TYPE_NAME(ADD),
    NODE_TYPE_NAME(BITWISE_AND),
    NODE_TYPE_NAME(BITWISE_NOT),
    NODE_TYPE_NAME(ASSIGNMENT),
    NODE_TYPE_NAME(BITWISE_OR),
    NODE_TYPE_NAME(BITWISE_XOR),
    NODE_TYPE_NAME(CLOSE_CURVLY_BRACKET),
    NODE_TYPE_NAME(CLOSE_PARENTHESIS),
    NODE_TYPE_NAME(CLOSE_SQUARE_BRACKET),
    NODE_TYPE_NAME(COLON),
    NODE_TYPE_NAME(COMMA),
    NODE_TYPE_NAME(CONDITIONAL),
    NODE_TYPE_NAME(DIVIDE),
    NODE_TYPE_NAME(GREATER),
    NODE_TYPE_NAME(LESS),
    NODE_TYPE_NAME(LOGICAL_NOT),
    NODE_TYPE_NAME(MODULO),
    NODE_TYPE_NAME(MULTIPLY),
    NODE_TYPE_NAME(OPEN_CURVLY_BRACKET),
    NODE_TYPE_NAME(OPEN_PARENTHESIS),
    NODE_TYPE_NAME(OPEN_SQUARE_BRACKET),
    NODE_TYPE_NAME(MEMBER),
    NODE_TYPE_NAME(SEMICOLON),
    NODE_TYPE_NAME(SUBTRACT),
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
    NODE_TYPE_NAME(FOR_IN),
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
    { Node::NODE_NULL, "NULL" },
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
    { Node::NODE_UNKNOWN, nullptr }
};
size_t const g_node_type_name_size = sizeof(g_node_type_name) / sizeof(g_node_type_name[0]);

struct operator_to_string_t
{
    Node::node_t    f_node;
    const char *    f_name;
};

#if defined(_DEBUG) || defined(DEBUG)
int g_operator_to_string_file_line = __LINE__; // for errors detected in this .cpp file
#endif
operator_to_string_t const g_operator_to_string[] =
{
    // single character -- sorted in ASCII
    { Node::NODE_LOGICAL_NOT,                     "!" },
    { Node::NODE_MODULO,                          "%" },
    { Node::NODE_BITWISE_AND,                     "&" },
    { Node::NODE_MULTIPLY,                        "*" },
    { Node::NODE_ADD,                             "+" },
    { Node::NODE_SUBTRACT,                        "-" },
    { Node::NODE_DIVIDE,                          "/" },
    { Node::NODE_LESS,                            "<" },
    { Node::NODE_ASSIGNMENT,                      "=" },
    { Node::NODE_GREATER,                         ">" },
    { Node::NODE_BITWISE_XOR,                     "^" },
    { Node::NODE_BITWISE_OR,                      "|" },
    { Node::NODE_BITWISE_NOT,                     "~" },

    // two or more characters transformed to an enum only
    { Node::NODE_ASSIGNMENT_ADD,                  "+=" },
    { Node::NODE_ASSIGNMENT_BITWISE_AND,          "&=" },
    { Node::NODE_ASSIGNMENT_BITWISE_OR,           "|=" },
    { Node::NODE_ASSIGNMENT_BITWISE_XOR,          "^=" },
    { Node::NODE_ASSIGNMENT_DIVIDE,               "/=" },
    { Node::NODE_ASSIGNMENT_LOGICAL_AND,          "&&=" },
    { Node::NODE_ASSIGNMENT_LOGICAL_OR,           "||=" },
    { Node::NODE_ASSIGNMENT_LOGICAL_XOR,          "^^=" },
    { Node::NODE_ASSIGNMENT_MAXIMUM,              "?>=" },
    { Node::NODE_ASSIGNMENT_MINIMUM,              "?<=" },
    { Node::NODE_ASSIGNMENT_MODULO,               "%=" },
    { Node::NODE_ASSIGNMENT_MULTIPLY,             "*=" },
    { Node::NODE_ASSIGNMENT_POWER,                "**=" },
    { Node::NODE_ASSIGNMENT_ROTATE_LEFT,          "!<=" },
    { Node::NODE_ASSIGNMENT_ROTATE_RIGHT,         "!>=" },
    { Node::NODE_ASSIGNMENT_SHIFT_LEFT,           "<<=" },
    { Node::NODE_ASSIGNMENT_SHIFT_RIGHT,          ">>=" },
    { Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED, ">>>=" },
    { Node::NODE_ASSIGNMENT_SUBTRACT,             "-=" },
    { Node::NODE_CALL,                            "()" },
    { Node::NODE_DECREMENT,                       "--" },
    { Node::NODE_EQUAL,                           "==" },
    { Node::NODE_GREATER_EQUAL,                   ">=" },
    { Node::NODE_INCREMENT,                       "++" },
    { Node::NODE_LESS_EQUAL,                      "<=" },
    { Node::NODE_LOGICAL_AND,                     "&&" },
    { Node::NODE_LOGICAL_OR,                      "||" },
    { Node::NODE_LOGICAL_XOR,                     "^^" },
    { Node::NODE_MATCH,                           "~=" },
    { Node::NODE_MAXIMUM,                         "?>" },
    { Node::NODE_MINIMUM,                         "?<" },
    { Node::NODE_NOT_EQUAL,                       "!=" },
    { Node::NODE_POST_DECREMENT,                  "--" },
    { Node::NODE_POST_INCREMENT,                  "++" },
    { Node::NODE_POWER,                           "**" },
    { Node::NODE_ROTATE_LEFT,                     "!<" },
    { Node::NODE_ROTATE_RIGHT,                    "!>" },
    { Node::NODE_SHIFT_LEFT,                      "<<" },
    { Node::NODE_SHIFT_RIGHT,                     ">>" },
    { Node::NODE_SHIFT_RIGHT_UNSIGNED,            ">>>" },
    { Node::NODE_STRICTLY_EQUAL,                  "===" },
    { Node::NODE_STRICTLY_NOT_EQUAL,              "!==" }

// the following doesn't make it in user redefinable operators yet
    //{ NODE_CONDITIONAL,                   "" },
    //{ NODE_DELETE,                        "" },
    //{ NODE_FOR_IN,                        "" },
    //{ NODE_IN,                            "" },
    //{ NODE_INSTANCEOF,                    "" },
    //{ NODE_IS,                            "" },
    //{ NODE_LIST,                          "" },
    //{ NODE_NEW,                           "" },
    //{ NODE_RANGE,                         "" },
    //{ NODE_SCOPE,                         "" },
};

size_t const g_operator_to_string_size = sizeof(g_operator_to_string) / sizeof(g_operator_to_string[0]);


}
// no name namespace




Node::Node(node_t type)
    : f_type(static_cast<int32_t>(type))
    //, f_flags_and_attributes() -- auto-init
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
}


Node::Node(pointer_t const& source, pointer_t& parent)
    : f_type(source->f_type)
    , f_flags_and_attributes(source->f_flags_and_attributes)
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
    case NODE_STRING:
    case NODE_INT64:
    case NODE_FLOAT64:
    case NODE_TRUE:
    case NODE_FALSE:
    case NODE_NULL:
    case NODE_UNDEFINED:
    case NODE_REGULAR_EXPRESSION:
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
                    std::cerr << "INTERNAL ERROR at offset " << idx
                              << " (line ~#" << (idx + g_node_type_name_file_line + 5)
                              << ", node type " << g_node_type_name[idx].f_type
                              << " vs. " << g_node_type_name[idx - 1].f_type
                              << "): the g_node_type_name table is not sorted properly. We cannot binary search it."
                              << std::endl;
                    throw exception_internal_error("INTERNAL ERROR: node type names not properly sorted, cannot properly search for names using a binary search.");
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
        r = g_node_type_name[p].f_type - f_type;
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


/**********************************************************************/
/**********************************************************************/
/***  DATA CONVERSION  ************************************************/
/**********************************************************************/
/**********************************************************************/

bool Node::to_boolean()
{
    switch(f_type) {
    case NODE_TRUE:
    case NODE_FALSE:
        // already a boolean
        break;

    case NODE_NULL:
    case NODE_UNDEFINED:
        f_type = static_cast<int32_t>(NODE_FALSE);
        break;

    case NODE_INT64:
        f_type = static_cast<int32_t>(f_int.get() != 0 ? NODE_TRUE : NODE_FALSE);
        break;

    case NODE_FLOAT64:
    {
        double value = f_float.get();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        f_type = static_cast<int32_t>(value != 0.0 && !std::isnan(value) ? NODE_TRUE : NODE_FALSE);
#pragma GCC diagnostic pop
    }
        break;

    case NODE_STRING:
        f_type = static_cast<int32_t>(f_str.empty() ? NODE_FALSE : NODE_TRUE);
        break;

    // At this time Data doesn't support any of these:
    //case CHARACTER:
    //case NAMESPACE:
    //case COMPOUNDATTRIBUTE:
    //case CLASS:
    //case SIMPLEINSTANCE:
    //case METHODCLOSURE:
    //case DATE:
    //case REGEXP:
    //case PACKAGE:
    //    f_type = NODE_TRUE;
    //    break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


bool Node::to_number()
{
    switch(f_type) {
    case NODE_INT64:
    case NODE_FLOAT64:
        break;

    case NODE_TRUE:
        f_type = static_cast<int32_t>(NODE_INT64);
        f_int.set(1);
        break;

    case NODE_NULL:
    case NODE_FALSE:
        f_type = static_cast<int32_t>(NODE_INT64);
        f_int.set(0);
        break;

    case NODE_UNDEFINED:
        f_type = static_cast<int32_t>(NODE_FLOAT64);
        f_float.set(FP_NAN);
        break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


bool Node::to_string()
{
    switch(f_type) {
    case NODE_STRING:
        break;

    case NODE_UNDEFINED:
        f_type = static_cast<int32_t>(NODE_STRING);
        f_str = "undefined";
        break;

    case NODE_NULL:
        f_type = static_cast<int32_t>(NODE_STRING);
        f_str = "null";
        break;

    case NODE_TRUE:
        f_type = static_cast<int32_t>(NODE_STRING);
        f_str = "true";
        break;

    case NODE_FALSE:
        f_type = static_cast<int32_t>(NODE_STRING);
        f_str = "false";
        break;

    case NODE_INT64:
    {
        f_type = static_cast<int32_t>(NODE_STRING);
        std::stringstream ss;
        ss << f_int.get();
        f_str = ss.str();
    }
        break;

    case NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    {
        double const value(f_float.get());
        f_type = static_cast<int32_t>(NODE_STRING);
        if(std::isnan(value))
        {
            f_str = "NaN";
        }
        else if(value == 0.0)
        {
            f_str = "0";
        }
        else if(std::isinf(value) < 0)
        {
            f_str = "-Infinity";
        }
        else if(std::isinf(value) > 0)
        {
            f_str = "Infinity";
        }
        else
        {
            std::stringstream ss;
            ss << value;
            f_str = ss.str();
        }
    }
#pragma GCC diagnostic pop
        break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


void Node::to_videntifier()
{
    if(NODE_IDENTIFIER != f_type)
    {
        throw exception_internal_error("to_videntifier() called with a node other than a NODE_IDENTIFIER node");
    }

    f_type = static_cast<int32_t>(NODE_VIDENTIFIER); // FIXME cast
}


void Node::set_boolean(bool value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case NODE_TRUE:
    case NODE_FALSE:
        break;

    default:
        throw exception_internal_error("set_boolean() called with a non-Boolean node type");

    }

    f_type = static_cast<int32_t>(value ? NODE_TRUE : NODE_FALSE);  // FIXME cast
}


void Node::set_int64(Int64 value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case NODE_INT64:
    case NODE_SWITCH: // comparison operator
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
    case NODE_FLOAT64:
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
    case NODE_BREAK: // name of label
    case NODE_CLASS: // name of class
    case NODE_CONTINUE: // name of label
    case NODE_IMPORT: // name of package
    case NODE_NAMESPACE: // name of namespace
    case NODE_PACKAGE: // name of package
    case NODE_STRING:
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
    case NODE_TRUE:
        return true;

    case NODE_FALSE:
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
    case NODE_INT64:
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
    case NODE_FLOAT64:
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
    case NODE_STRING:
        break;

    default:
        throw exception_internal_error("get_string() called with a non-string node type");

    }

    return f_str;
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
bool Node::get_flag(flag_attribute_t f) const
{
    verify_flag_attribute(f);
    return f_flags_and_attributes[f];
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
void Node::set_flag(flag_attribute_t f, bool v)
{
    verify_flag_attribute(f);
    f_flags_and_attributes[f] = v;
}


/** \brief Verify that f corresponds to the data type.
 *
 * This function verifies that f corresponds to a valid flag according
 * to the type of this Node object.
 *
 * \param[in] f  The flag or attribute to check.
 */
void Node::verify_flag_attribute(flag_attribute_t f) const
{
    switch(f)
    {
    case NODE_CATCH_FLAG_TYPED:
        if(f_type != NODE_CATCH)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES:
        if(f_type != NODE_DIRECTIVE_LIST)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_FOR_FLAG_FOREACH:
        if(f_type != NODE_FOR)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_FUNCTION_FLAG_GETTER:
    case NODE_FUNCTION_FLAG_SETTER:
    case NODE_FUNCTION_FLAG_OUT:
    case NODE_FUNCTION_FLAG_VOID:
    case NODE_FUNCTION_FLAG_NEVER:
    case NODE_FUNCTION_FLAG_NOPARAMS:
    case NODE_FUNCTION_FLAG_OPERATOR:
        if(f_type != NODE_FUNCTION)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_IDENTIFIER_FLAG_WITH:
    case NODE_IDENTIFIER_FLAG_TYPED:
        if(f_type != NODE_IDENTIFIER
        && f_type != NODE_VIDENTIFIER
        && f_type != NODE_STRING)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_IMPORT_FLAG_IMPLEMENTS:
        if(f_type != NODE_IMPORT)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_PACKAGE_FLAG_FOUND_LABELS:
    case NODE_PACKAGE_FLAG_REFERENCED:
        if(f_type != NODE_PACKAGE)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_PARAM_MATCH_FLAG_UNPROTOTYPED:
        if(f_type != NODE_PARAM_MATCH)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_PARAMETERS_FLAG_CONST:
    case NODE_PARAMETERS_FLAG_IN:
    case NODE_PARAMETERS_FLAG_OUT:
    case NODE_PARAMETERS_FLAG_NAMED:
    case NODE_PARAMETERS_FLAG_REST:
    case NODE_PARAMETERS_FLAG_UNCHECKED:
    case NODE_PARAMETERS_FLAG_UNPROTOTYPED:
    case NODE_PARAMETERS_FLAG_REFERENCED:    // referenced from a parameter or a variable
    case NODE_PARAMETERS_FLAG_PARAMREF:      // referenced from another parameter
    case NODE_PARAMETERS_FLAG_CATCH:         // a parameter defined in a catch()
        if(f_type != NODE_PARAMETERS)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_SWITCH_FLAG_DEFAULT:           // we found a 'default:' label in that switch
        if(f_type != NODE_SWITCH)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    case NODE_VAR_FLAG_CONST:
    case NODE_VAR_FLAG_LOCAL:
    case NODE_VAR_FLAG_MEMBER:
    case NODE_VAR_FLAG_ATTRIBUTES:
    case NODE_VAR_FLAG_ENUM:                 // there is a NODE_SET and it somehow needs to be copied
    case NODE_VAR_FLAG_COMPILED:             // Expression() was called on the NODE_SET
    case NODE_VAR_FLAG_INUSE:                // this variable was referenced
    case NODE_VAR_FLAG_ATTRS:                // currently being read for attributes (to avoid loops)
    case NODE_VAR_FLAG_DEFINED:              // was already parsed
    case NODE_VAR_FLAG_DEFINING:             // currently defining, can't read
    case NODE_VAR_FLAG_TOADD:                // to be added in the directive list
        if(f_type != NODE_VARIABLE
        && f_type != NODE_VAR
        && f_type != NODE_PARAM)
        {
            throw exception_internal_error("flag / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    // member visibility
    case NODE_ATTR_PUBLIC:
    case NODE_ATTR_PRIVATE:
    case NODE_ATTR_PROTECTED:
    case NODE_ATTR_INTERNAL:

    // function member type
    case NODE_ATTR_STATIC:
    case NODE_ATTR_ABSTRACT:
    case NODE_ATTR_VIRTUAL:
    case NODE_ATTR_ARRAY:

    // function/variable is defined in your system (execution env.)
    case NODE_ATTR_INTRINSIC:

    // operator overload (function member)
    case NODE_ATTR_CONSTRUCTOR:

    // function & member constrains
    case NODE_ATTR_FINAL:
    case NODE_ATTR_ENUMERABLE:

    // conditional compilation
    case NODE_ATTR_TRUE:
    case NODE_ATTR_FALSE:
    case NODE_ATTR_UNUSED:                      // if definition is used, error!

    // class attribute (whether a class can be enlarged at run time)
    case NODE_ATTR_DYNAMIC:

    // switch attributes
    case NODE_ATTR_FOREACH:
    case NODE_ATTR_NOBREAK:
    case NODE_ATTR_AUTOBREAK:
        // TBD -- we'll need to see whether we want to limit the attributes
        //        on a per node type basis and how we can do that properly
        if(f_type == NODE_PROGRAM)
        {
            throw exception_internal_error("attribute / type missmatch in Node::verify_flag_attribute()");
        }
        break;

    // attributes were defined
    case NODE_ATTR_DEFINED:
        // all nodes can receive this flag
        break;

    case NODE_FLAG_ATTRIBUTE_MAX:
        throw exception_internal_error("invalid attribute / flag in Node::verify_flag_attribute()");

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }
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
                              << " (line ~#" << (idx + g_operator_to_string_file_line + 5)
                              << ", node type " << g_operator_to_string[idx].f_node
                              << " vs. " << g_operator_to_string[idx - 1].f_node
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
        r = g_operator_to_string[p].f_node - op;
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

    return NODE_UNKNOWN;
}



// TODO: understand what the heck we were trying to do with the ReplaceWith()
//void Node::replace_with(pointer_t& node)
//{
//    modifying();
//
//    if(node->f_parent)
//    {
//        throw exception_internal_error("replace_with() called with a node that already has a parent");
//    }
//
//    node->f_parent = f_parent;
//    f_parent.ClearNode();
//}


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
    f_children[index]->set_parent();
}


void Node::append_child(pointer_t& child)
{
    child->set_parent(shared_from_this());
}


void Node::insert_child(int index, pointer_t& child)
{
    modifying();

    child->set_parent(shared_from_this(), index);
}


void Node::set_child(int index, pointer_t& child)
{
    modifying();

    delete_child(index);
    insert_child(index, child);
}


Node::pointer_t Node::get_child(int index) const
{
    return f_children[index];
}


void Node::set_offset(int32_t offset)
{
    f_offset = offset;
}


int32_t Node::get_offset() const
{
    return f_offset;
}


void Node::set_link(link_t index, pointer_t& link)
{
    modifying();

    if(index >= LINK_max)
    {
        throw exception_index_out_of_range("set_link() called with an index out of bounds.");
    }

    // make sure the size is reserved on first set
    if(f_link.empty())
    {
        f_link.resize(LINK_max);
    }

    if(link)
    {
        // link already set?
        if(f_link[index])
        {
            throw exception_internal_error("a link was set twice at the same offset");
        }

        f_link[index] = link;
    }
    else
    {
        f_link[index].reset();
    }
}


Node::pointer_t Node::get_link(link_t index)
{
    if(index >= LINK_max)
    {
        throw exception_index_out_of_range("get_link() called with an index out of bounds.");
    }

    if(f_link.empty())
    {
        return nullptr;
    }

    return f_link[index];
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
    //     compiled yet and they could have
    //     side effects too; now this is much
    //     less likely a problem because then
    //     the programmer is most certainly
    //     creating a really weird program
    //     with all sorts of side effects that
    //     he wants no one else to know about,
    //     etc. etc. etc.
    //
    // Problem III:
    //    Note that we don't memorize whether
    //    a node has side effects because its
    //    children may change and then the side
    //    effects may disappear
    //

    switch(f_type) {
    case NODE_ASSIGNMENT:
    case NODE_ASSIGNMENT_ADD:
    case NODE_ASSIGNMENT_BITWISE_AND:
    case NODE_ASSIGNMENT_BITWISE_OR:
    case NODE_ASSIGNMENT_BITWISE_XOR:
    case NODE_ASSIGNMENT_DIVIDE:
    case NODE_ASSIGNMENT_LOGICAL_AND:
    case NODE_ASSIGNMENT_LOGICAL_OR:
    case NODE_ASSIGNMENT_LOGICAL_XOR:
    case NODE_ASSIGNMENT_MAXIMUM:
    case NODE_ASSIGNMENT_MINIMUM:
    case NODE_ASSIGNMENT_MODULO:
    case NODE_ASSIGNMENT_MULTIPLY:
    case NODE_ASSIGNMENT_POWER:
    case NODE_ASSIGNMENT_ROTATE_LEFT:
    case NODE_ASSIGNMENT_ROTATE_RIGHT:
    case NODE_ASSIGNMENT_SHIFT_LEFT:
    case NODE_ASSIGNMENT_SHIFT_RIGHT:
    case NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case NODE_ASSIGNMENT_SUBTRACT:
    case NODE_CALL:
    case NODE_DECREMENT:
    case NODE_DELETE:
    case NODE_INCREMENT:
    case NODE_NEW:
    case NODE_POST_DECREMENT:
    case NODE_POST_INCREMENT:
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


void Node::add_variable(pointer_t& variable)
{
    if(NODE_VARIABLE != variable->f_type)
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


void Node::add_label(pointer_t& label)
{
    if(NODE_LABEL != label->f_type)
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

    out << std::setw(4) << std::setfill('0') << f_type << std::setfill('\0') << ": " << get_type_name();
    if(f_type > ' ' && f_type < 0x7F)
    {
        out << " = '" << f_type << "'";
    }

    switch(f_type) {
    case NODE_IDENTIFIER:
    case NODE_VIDENTIFIER:
    case NODE_STRING:
    case NODE_GOTO:
    case NODE_LABEL:
    case NODE_IMPORT:
    case NODE_CLASS:
    case NODE_INTERFACE:
    case NODE_ENUM:
        sub_function::display_str(out, f_str);
        break;

    case NODE_PACKAGE:
        sub_function::display_str(out, f_str);
        if(f_flags_and_attributes[NODE_PACKAGE_FLAG_FOUND_LABELS])
        {
            out << " FOUND-LABELS";
        }
        break;

    case NODE_INT64:
        out << ": " << f_int.get() << ", 0x" << std::hex << std::setw(16) << std::setfill('0') << f_int.get() << std::dec << std::setw(0) << std::setfill('\0');
        break;

    case NODE_FLOAT64:
        out << ": " << f_float.get();
        break;

    case NODE_FUNCTION:
        sub_function::display_str(out, f_str);
        if(f_flags_and_attributes[NODE_FUNCTION_FLAG_GETTER])
        {
            out << " GETTER";
        }
        if(f_flags_and_attributes[NODE_FUNCTION_FLAG_SETTER])
        {
            out << " SETTER";
        }
        break;

    case NODE_PARAM:
        sub_function::display_str(out, f_str);
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_CONST])
        {
            out << " CONST";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_IN])
        {
            out << " IN";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_OUT])
        {
            out << " OUT";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_NAMED])
        {
            out << " NAMED";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_REST])
        {
            out << " REST";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_UNCHECKED])
        {
            out << " UNCHECKED";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_UNPROTOTYPED])
        {
            out << " UNPROTOTYPED";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_REFERENCED])
        {
            out << " REFERENCED";
        }
        if(f_flags_and_attributes[NODE_PARAMETERS_FLAG_PARAMREF])
        {
            out << " PARAMREF";
        }
        break;

    case NODE_PARAM_MATCH:
        out << ":";
        if(f_flags_and_attributes[NODE_PARAM_MATCH_FLAG_UNPROTOTYPED])
        {
            out << " UNPROTOTYPED";
        }
        break;

    case NODE_VARIABLE:
    case NODE_VAR_ATTRIBUTES:
        sub_function::display_str(out, f_str);
        if(f_flags_and_attributes[NODE_VAR_FLAG_CONST])
        {
            out << " CONST";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_LOCAL])
        {
            out << " LOCAL";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_MEMBER])
        {
            out << " MEMBER";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_ATTRIBUTES])
        {
            out << " ATTRIBUTES";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_ENUM])
        {
            out << " ENUM";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_COMPILED])
        {
            out << " COMPILED";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_INUSE])
        {
            out << " INUSE";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_ATTRS])
        {
            out << " ATTRS";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_DEFINED])
        {
            out << " DEFINED";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_DEFINING])
        {
            out << " DEFINING";
        }
        if(f_flags_and_attributes[NODE_VAR_FLAG_TOADD])
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


void Node::display(std::ostream& out, int indent, pointer_t const& parent, char c) const
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
        display_attributes(std::ostream& out, flag_attribute_set_t attrs)
            : f_out(out)
            , f_flags_and_attributes(attrs)
        {
            display_attribute(NODE_ATTR_PUBLIC,         "PUBLIC"        );
            display_attribute(NODE_ATTR_PRIVATE,        "PRIVATE"       );
            display_attribute(NODE_ATTR_PROTECTED,      "PROTECTED"     );
            display_attribute(NODE_ATTR_STATIC,         "STATIC"        );
            display_attribute(NODE_ATTR_ABSTRACT,       "ABSTRACT"      );
            display_attribute(NODE_ATTR_VIRTUAL,        "VIRTUAL"       );
            display_attribute(NODE_ATTR_INTERNAL,       "INTERNAL"      );
            display_attribute(NODE_ATTR_INTRINSIC,      "INTRINSIC"     );
            display_attribute(NODE_ATTR_CONSTRUCTOR,    "CONSTRUCTOR"   );
            display_attribute(NODE_ATTR_FINAL,          "FINAL"         );
            display_attribute(NODE_ATTR_ENUMERABLE,     "ENUMERABLE"    );
            display_attribute(NODE_ATTR_TRUE,           "TRUE"          );
            display_attribute(NODE_ATTR_FALSE,          "FALSE"         );
            display_attribute(NODE_ATTR_UNUSED,         "UNUSED"        );
            display_attribute(NODE_ATTR_DYNAMIC,        "DYNAMIC"       );
            display_attribute(NODE_ATTR_FOREACH,        "FOREACH"       );
            display_attribute(NODE_ATTR_NOBREAK,        "NOBREAK"       );
            display_attribute(NODE_ATTR_AUTOBREAK,      "AUTOBREAK"     );
            display_attribute(NODE_ATTR_DEFINED,        "DEFINED"       );
        }

        void display_attribute(flag_attribute_t a, char const *n)
        {
            if(f_flags_and_attributes[a])
            {
                f_out << " " << n;
            }
        }

        std::ostream&               f_out;
        controlled_vars::fbool_t    f_first;
        flag_attribute_set_t        f_flags_and_attributes;
    } display_attr(out, f_flags_and_attributes);

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



}
// namespace as2js

// vim: ts=4 sw=4 et
