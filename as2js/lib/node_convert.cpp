/* node_convert.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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
/***  DATA CONVERSION  ************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Transform any node to NODE_UNKNOWN
 *
 * This function mark the node as unknown. Absolutely any node can be
 * marked as unknown. It is particularly used by the compiler and
 * optimizer to cancel nodes that they cannot otherwise delete at
 * the time they are working on the tree.
 *
 * All the children of an unknown node are ignored too.
 *
 * To remove all the unknown nodes once the compiler is finished,
 * one can call the clean_tree() function.
 */
void Node::to_unknown()
{
    modifying();

    // whatever the type of node we can always convert it to an unknown
    // node since that's similar to "deleting" the node
    f_type = node_t::NODE_UNKNOWN;
    // clear the node's data to avoid other problems?
}


/** \brief Transform a call in a NODE_AS node.
 *
 * This function transform a node defined as NODE_CALL into a NODE_AS.
 * The special casting syntax looks exactly like a function call. For
 * this reason the parser returns it as such. The compiler, however,
 * can determine whether the function name is really a function name
 * or if it is a type name. If it is a type, then the tree is changed
 * to represent an AS instruction instead:
 *
 * \code
 *     type ( expression )
 *     expression AS type
 * \endcode
 *
 * \todo
 * We will need to verify that this is correct and does not introduce
 * other problems. However, remember that we do not use prototypes in
 * our world. We have well defined classes so it should work just fine.
 *
 * \return true if the conversion happens.
 */
bool Node::to_as()
{
    modifying();

    // "a call to a getter" may be transformed from CALL to AS
    // because a getter can very much look like a cast (false positive)
    if(node_t::NODE_CALL == f_type)
    {
        f_type = node_t::NODE_AS;
        return true;
    }

    return false;
}


/** \brief Check whether a node can be converted to Boolean.
 *
 * This function is constant and can be used to see whether a node
 * represent true or false without actually converting the node.
 *
 * \li NODE_TRUE -- returned as is
 * \li NODE_FALSE -- returned as is
 * \li NODE_NULL -- returns NODE_FALSE
 * \li NODE_UNDEFINED -- returns NODE_FALSE
 * \li NODE_INT64 -- returns NODE_TRUE unless the interger is zero
 *                   in which case NODE_FALSE is returned
 * \li NODE_FLOAT64 -- returns NODE_TRUE unless the floating point is zero
 *                     in which case NODE_FALSE is returned
 * \li NODE_STRING -- returns NODE_TRUE unless the string is empty in
 *                    which case NODE_FALSE is returned
 * \li Any other node type -- returns NODE_UNDEFINED
 *
 * \return NODE_TRUE, NODE_FALSE, or NODE_UNDEFINED depending on 'this' node
 */
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


/** \brief Convert this node to a boolean node.
 *
 * This function converts 'this' node to a Boolean node:
 *
 * \li NODE_TRUE -- no conversion
 * \li NODE_FALSE -- no conversion
 * \li NODE_NULL -- converted to NODE_FALSE
 * \li NODE_UNDEFINED -- converted to NODE_FALSE
 * \li NODE_INT64 -- converted to NODE_TRUE unless it is 0
 *                   in which case it gets converted to NODE_FALSE
 * \li NODE_FLOAT64 -- converted to NODE_TRUE unless it is 0.0
 *                     in which case it gets converted to NODE_FALSE
 * \li NODE_STRING -- converted to NODE_TRUE unless the string is empty
 *                    in which case it gets converted to NODE_FALSE
 *
 * \return true if the conversion succeeds.
 */
bool Node::to_boolean()
{
    modifying();

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


/** \brief Convert a member or assignment to a function call.
 *
 * This function is used to convert a getter to a function call.
 *
 * \code
 *     // Convert a getter to a function call
 *     a = foo.field;
 *     a = foo.field_getter();
 *
 *     // Convert a setter to a function call
 *     foo.field = a;
 *     foo.field_setter(a);
 * \endcode
 *
 * The function returns false if 'this' node is not a NODE_MEMBER or
 * a NODE_ASSIGNMENT.
 *
 * \return true if the conversion succeeded.
 */
bool Node::to_call()
{
    modifying();

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


/** \brief Convert this node to a NODE_INT64.
 *
 * This function converts the node to an integer number,
 * just like JavaScript would do. This means converting the following
 * type of nodes:
 *
 * \li NODE_INT64 -- no conversion
 * \li NODE_FLOAT64 -- convert to integer
 * \li NODE_TRUE -- convert to 1
 * \li NODE_FALSE -- convert to 0
 * \li NODE_NULL -- convert to 0
 * \li NODE_UNDEFINED -- convert to 0 (NaN is not possible in an integer)
 *
 * This function does not convert strings. You may use the to_number()
 * to get NODE_STRING converted although it will convert it to a
 * floating pointer number instead. To still get an integer call both
 * functions in a row:
 *
 * \code
 *    node->to_number();
 *    node->to_int64();
 * \endcode
 *
 * \return true if the conversion succeeded.
 */
bool Node::to_int64()
{
    modifying();

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


/** \brief Convert this node to a NODE_FLOAT64.
 *
 * This function converts the node to a floatingp point number,
 * just like JavaScript would do. This means converting the following
 * type of nodes:
 *
 * \li NODE_INT64 -- convert to a float
 * \li NODE_FLOAT64 -- no conversion
 * \li NODE_TRUE -- convert to 1.0
 * \li NODE_FALSE -- convert to 0.0
 * \li NODE_NULL -- convert to 0.0
 * \li NODE_UNDEFINED -- convert to NaN
 *
 * This function does not convert strings. You may use the to_number()
 * to get NODE_STRING converted.
 *
 * \return true if the conversion succeeded.
 */
bool Node::to_float64()
{
    modifying();

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


/** \brief Convert this node to a label.
 *
 * This function converts a NODE_IDENTIFIER node to a NODE_LABEL node.
 *
 * \return true if the conversion succeeded.
 */
bool Node::to_label()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_IDENTIFIER:
        f_type = node_t::NODE_LABEL;
        break;

    default:
        // failure (cannot convert)
        return false;

    }

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
 * \li NODE_NULL -- convert to 0 (INT64)
 * \li NODE_UNDEFINED -- convert to NaN (FLOAT64)
 * \li NODE_STRING -- converted to a float, NaN if not a valid float,
 *                    however, zero if empty.
 *
 * \return true if the conversion succeeded.
 */
bool Node::to_number()
{
    modifying();

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
        // failure (cannot convert)
        return false;

    }

    return true;
}


/** \brief Transform a node to a string.
 *
 * This function transform a node from what it is to a string. If the
 * transformation is successful, the function returns true. Note that
 * the function does not throw if the type of 'this' cannot be
 * converted to a string.
 *
 * The nodes that can be converted to a string are:
 *
 * \li NODE_STRING -- unchanged
 * \li NODE_IDENTIFIER -- the identifier is now a string
 * \li NODE_UNDEFINED -- changed to "undefined"
 * \li NODE_NULL -- changed to "null"
 * \li NODE_TRUE -- changed to "true"
 * \li NODE_FALSE -- changed to "false"
 * \li NODE_INT64 -- changed to a string representation
 * \li NODE_FLOAT64 -- changed to a string representation
 *
 * \return true if the conversion succeeded, false otherwise.
 */
bool Node::to_string()
{
    modifying();

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


/** \brief Transform an identifier into a NODE_VIDENTIFIER.
 *
 * This function is used to transform an identifier in a variable
 * identifier. By default identifiers may represent object names.
 * However, when written between parenthesis, they always represent
 * a variable. This can be important as certain syntax are not
 * at all equivalent:
 *
 * \code
 *    (a).field      // a becomes a NODE_VIDENTIFIER
 *    a.field
 * \code
 *
 * In the first case, (a) is transform with the content of variable
 * 'a' and that is used to access 'field'.
 *
 * In the second case, 'a' represents an object and we are access
 * that object's 'field' directly.
 *
 * \todo
 * Determine whether that really applies to JavaScript.
 *
 * \exception exception_internal_error
 * This exception is raised if the input node is not a NODE_IDENTIFIER.
 */
void Node::to_videntifier()
{
    modifying();

    if(node_t::NODE_IDENTIFIER != f_type)
    {
        throw exception_internal_error("to_videntifier() called with a node other than a NODE_IDENTIFIER node");
    }

    f_type = node_t::NODE_VIDENTIFIER;
}


/** \brief Transform a variable into a variable of attributes.
 *
 * When compiling the tree, the code in compiler_variable.cpp may detect
 * that a variable is specifically used to represent a list of attributes.
 * When that happens, the compiler transforms the variable calling
 * this function.
 *
 * The distinction makes it a lot easier to deal with the variable later.
 *
 * \exception exception_internal_error
 * This exception is raised if 'this' node is not a NODE_VARIABLE.
 */
void Node::to_var_attributes()
{
    modifying();

    if(node_t::NODE_VARIABLE != f_type)
    {
        throw exception_internal_error("to_var_attribute() called with a node other than a NODE_VARIABLE node");
    }

    f_type = node_t::NODE_VAR_ATTRIBUTES;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
