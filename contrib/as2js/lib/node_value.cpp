/* node_value.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/node.h"

#include    "as2js/exceptions.h"


/** \file
 * \brief Define a set of functions used to change the value of literals.
 *
 * The system supports a few direct literals:
 *
 * \li integers
 * \li floating points
 * \li strings
 * \li identifiers
 * \li labels
 * \li class
 *
 * Each one of these can be set a value representing the literal as read
 * in the source file. The functions below handle that value.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE VALUE  *****************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Set the Boolean value of this node.
 *
 * This function saves a Boolean value in this node.
 *
 * Note that only two types of nodes can be assigned a Boolean value:
 *
 * NODE_TRUE and NODE_FALSE
 *
 * This function converst the C++ Boolean value to either NODE_TRUE
 * or NODE_FALSE.
 *
 * \exception exception_internal_error
 * This exception is raised if the set_boolean() function is called on a
 * type of node that is not a Boolean node.
 *
 * \param[in] value  The C++ Boolean value to save in this node.
 */
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


/** \brief Set the Int64 of this node.
 *
 * This function saves an Int64 in this node.
 *
 * Note that only one type of node can be assigned an Int64:
 *
 * NODE_INT64
 *
 * \exception exception_internal_error
 * This exception is raised if the set_int64() function is called on a
 * type of node that does not support a string.
 *
 * \param[in] value  The Int64 to save in this node.
 */
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


/** \brief Set the Float64 of this node.
 *
 * This function saves a Float64 in this node.
 *
 * Note that only one type of node can be assigned a Float64:
 *
 * NODE_FLOAT64
 *
 * \exception exception_internal_error
 * This exception is raised if the set_float64() function is called on a
 * type of node that does not support a string.
 *
 * \param[in] value  The Float64 to save in this node.
 */
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


/** \brief Set the string of this node.
 *
 * This function saves a string in this node. The string is a
 * standard String object (full Unicode support.)
 *
 * Note that only a few types of nodes can be assigned a string:
 *
 * NODE_BREAK, NODE_CLASS, NODE_CONTINUE, NODE_ENUM, NODE_FUNCTION,
 * NODE_GOTO, NODE_IDENTIFIER, NODE_IMPORT, NODE_INTERFACE, NODE_LABEL,
 * NODE_NAME, NODE_NAMESPACE, NODE_PACKAGE, NODE_PARAM,
 * NODE_REGULAR_EXPRESSION, NODE_STRING, NODE_VARIABLE, NODE_VAR_ATTRIBUTES,
 * and NODE_VIDENTIFIER
 *
 * \exception exception_internal_error
 * This exception is raised if the set_string() function is called on a
 * type of node that does not support a string.
 *
 * \param[in] value  The String to save in this node.
 */
void Node::set_string(String const& value)
{
    // only the corresponding node type accepts a set() call
    switch(f_type)
    {
    case node_t::NODE_BREAK:                // name of label: BREAK [IDENTIFIER | DEFAULT];
    case node_t::NODE_CLASS:                // name of class: CLASS IDENTIFIER
    case node_t::NODE_CONTINUE:             // name of label: CONTINUE [IDENTIFIER | DEFAULT];
    case node_t::NODE_ENUM:                 // name of enumeration: ENUM [IDENTIFIER] ...;
    case node_t::NODE_FUNCTION:             // name of function: FUNCTION [[GET | SET] IDENTIFIER | STRING] ...;
    case node_t::NODE_GOTO:                 // name of label: GOTO IDENTIFIER;
    case node_t::NODE_IDENTIFIER:           // the identifier string: IDENTIFIER
    case node_t::NODE_IMPORT:               // name of package: IMPORT [IDENTIFIER ('.' IDENTIFIER)* | STRING] ...;
    case node_t::NODE_INTERFACE:            // name of interface: INTERFACE IDENTIFIER
    case node_t::NODE_LABEL:                // name of label: IDENTIFIER ':'
    case node_t::NODE_NAME:                 // name of object field: { IDENTIFIER ':' ... }
    case node_t::NODE_NAMESPACE:            // name of namespace: NAMESPACE IDENTIFIER
    case node_t::NODE_PACKAGE:              // name of package: PACKAGE [IDENTIFIER ('.' IDENTIFIER)* | STRING] ...;
    case node_t::NODE_PARAM:                // name of parameter: FUNCTION '(' IDENTIFIER ... ')' ...
    case node_t::NODE_REGULAR_EXPRESSION:   // name of parameter: `...` or /.../...
    case node_t::NODE_STRING:               // the string itself: STRING
    case node_t::NODE_VARIABLE:             // name of variable: VAR <name> [':' type_expr] ['=' expression], ...;
    case node_t::NODE_VAR_ATTRIBUTES:       // name of variable: VAR <name> [':' type_expr] ['=' expression], ...; (transformed to VAR_ATTRIBUTES)
    case node_t::NODE_VIDENTIFIER:          // the identifier string: IDENTIFIER (transformed to VIDENTIFIER)
        break;

    default:
        throw exception_internal_error("set_string() called with a non-string node type");

    }

    f_str = value;
}


/** \brief Get the Boolean value of this node.
 *
 * This function returns true or false depending on the node type:
 * NODE_TRUE or NODE_FALSE.
 *
 * \exception exception_internal_error
 * This exception is raised if the get_boolean() function is called on a
 * type of node which is not NODE_TRUE or NODE_FALSE.
 *
 * \return The Boolean value attached to this node.
 */
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


/** \brief Get the Int64 of this node.
 *
 * This function returns the Int64 of this node.
 *
 * Note that only one type of nodes can be assigned an Int64:
 *
 * NODE_INT64
 *
 * \exception exception_internal_error
 * This exception is raised if the get_int64() function is called on a
 * type of node that does not support a float.
 *
 * \return The integer attached to this node.
 */
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


/** \brief Get the Float64 of this node.
 *
 * This function returns the Float64 of this node.
 *
 * Note that only one type of nodes can be assigned a Float64:
 *
 * NODE_FLOAT64
 *
 * \exception exception_internal_error
 * This exception is raised if the get_float64() function is called on a
 * type of node that does not support a float.
 *
 * \return The float attached to this node.
 */
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


/** \brief Get the string of this node.
 *
 * This function returns the string of this node. The string is a
 * standard String object (full Unicode support.)
 *
 * Note that only a few types of nodes can be assigned a string:
 *
 * NODE_BREAK, NODE_CLASS, NODE_CONTINUE, NODE_ENUM, NODE_FUNCTION,
 * NODE_GOTO, NODE_IDENTIFIER, NODE_IMPORT, NODE_INTERFACE, NODE_LABEL,
 * NODE_NAME, NODE_NAMESPACE, NODE_PACKAGE, NODE_PARAM,
 * NODE_REGULAR_EXPRESSION, NODE_STRING, NODE_VARIABLE, NODE_VAR_ATTRIBUTES,
 * and NODE_VIDENTIFIER
 *
 * \exception exception_internal_error
 * This exception is raised if the get_string() function is called on a
 * type of node that does not support a string.
 *
 * \return The string attached to this node.
 */
String const& Node::get_string() const
{
    // only the corresponding node type accepts a get() call
    switch(f_type)
    {
    case node_t::NODE_BREAK:                // name of label: BREAK [IDENTIFIER | DEFAULT];
    case node_t::NODE_CLASS:                // name of class: CLASS IDENTIFIER
    case node_t::NODE_CONTINUE:             // name of label: CONTINUE [IDENTIFIER | DEFAULT];
    case node_t::NODE_ENUM:                 // name of enumeration: ENUM [IDENTIFIER] ...;
    case node_t::NODE_FUNCTION:             // name of function: FUNCTION [[GET | SET] IDENTIFIER | STRING] ...;
    case node_t::NODE_GOTO:                 // name of label: GOTO IDENTIFIER;
    case node_t::NODE_IDENTIFIER:           // the identifier string: IDENTIFIER
    case node_t::NODE_IMPORT:               // name of package: IMPORT [IDENTIFIER ('.' IDENTIFIER)* | STRING] ...;
    case node_t::NODE_INTERFACE:            // name of interface: INTERFACE IDENTIFIER
    case node_t::NODE_LABEL:                // name of label: IDENTIFIER ':'
    case node_t::NODE_NAME:                 // name of object field: { IDENTIFIER ':' ... }
    case node_t::NODE_NAMESPACE:            // name of namespace: NAMESPACE IDENTIFIER
    case node_t::NODE_PACKAGE:              // name of package: PACKAGE [IDENTIFIER ('.' IDENTIFIER)* | STRING] ...;
    case node_t::NODE_PARAM:                // name of parameter: FUNCTION '(' IDENTIFIER ... ')' ...
    case node_t::NODE_REGULAR_EXPRESSION:   // name of parameter: `...` or /.../...
    case node_t::NODE_STRING:               // the string itself: "..." or '...'
    case node_t::NODE_VARIABLE:             // name of variable: VAR <name> [':' type_expr] ['=' expression], ...;
    case node_t::NODE_VAR_ATTRIBUTES:       // name of variable: VAR <name> [':' type_expr] ['=' expression], ...; (transformed to VAR_ATTRIBUTES)
    case node_t::NODE_VIDENTIFIER:          // the identifier string: IDENTIFIER (transformed to VIDENTIFIER)
        break;

    default:
        throw exception_internal_error(std::string("get_string() called with non-string node type: ") + get_type_name());

    }

    return f_str;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
