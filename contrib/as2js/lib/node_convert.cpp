/* node_convert.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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
 * \brief Convert a Node object to another type.
 *
 * The conversion functions allow one to convert a certain number of
 * Node objects from their current type to a different type.
 *
 * Most Node cannot be converted to anything else than the UNKNOWN
 * Node type, which is used to <em>delete</em> a Node. The various
 * conversion functions defined below let you know what types are
 * accepted by each function.
 *
 * In most cases the conversion functions will return a Boolean
 * value. If false, then the conversion did not happen. You are
 * responsible for checking the result and act on it appropriately.
 *
 * Although a conversion function, the set_boolean() function is
 * actually defined in the node_value.cpp file. It is done that way
 * because it looks very similar to the set_int64(), set_float64(),
 * and set_string() functions.
 */


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  DATA CONVERSION  ************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Transform any node to NODE_UNKNOWN
 *
 * This function marks the node as unknown. Absolutely any node can be
 * marked as unknown. It is specifically used by the compiler and
 * optimizer to cancel nodes that cannot otherwise be deleted at
 * the time they are working on the tree.
 *
 * All the children of an unknown node are ignored too (considered
 * as NODE_UNKNOWN, although they do not all get converted.)
 *
 * To remove all the unknown nodes once the compiler is finished,
 * one can call the clean_tree() function.
 *
 * \note
 * The Node must not be locked.
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
 * This function transforms a node defined as NODE_CALL into a NODE_AS.
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
 * \note
 * The Node must not be locked.
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
 * represents true or false without actually converting the node.
 *
 * \li NODE_TRUE -- returned as is
 * \li NODE_FALSE -- returned as is
 * \li NODE_NULL -- returns NODE_FALSE
 * \li NODE_UNDEFINED -- returns NODE_FALSE
 * \li NODE_INT64 -- returns NODE_TRUE unless the interger is zero
 *                   in which case NODE_FALSE is returned
 * \li NODE_FLOAT64 -- returns NODE_TRUE unless the floating point is
 *                     exactly zero in which case NODE_FALSE is returned
 * \li NODE_STRING -- returns NODE_TRUE unless the string is empty in
 *                    which case NODE_FALSE is returned
 * \li Any other node type -- returns NODE_UNDEFINED
 *
 * Note that in this case we completely ignore the content of a string.
 * The strings "false", "0.0", and "0" all represent Boolean 'true'.
 *
 * \return NODE_TRUE, NODE_FALSE, or NODE_UNDEFINED depending on 'this' node
 *
 * \sa to_boolean()
 * \sa set_boolean()
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


/** \brief Convert this node to a Boolean node.
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
 * Other input types do not get converted and the function returns false.
 *
 * To just test the Boolean value of a node without converting it, call
 * to_boolean_type_only() instead.
 *
 * \note
 * The Node must not be locked.
 *
 * \return true if the conversion succeeds.
 *
 * \sa to_boolean_type_only()
 * \sa set_boolean()
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


/** \brief Convert a getter or setter to a function call.
 *
 * This function is used to convert a getter ot a setter to
 * a function call.
 *
 * A read from a member variable is a getter if the name of
 * the field was actually defined as a 'get' function.
 *
 * A write to a member variable is a setter if the name of
 * the field was actually defined as a 'set' function.
 *
 * \code
 *     class foo_class
 *     {
 *         function get field() { ... }
 *         function set field() { ... }
 *     };
 *
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
 * \note
 * This function has no way of knowing what's what.
 * It just changes the f_type parameter of this node.
 *
 * \note
 * The Node must not be locked.
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


/** \brief Convert this node to a NODE_IDENTIFIER.
 *
 * This function converts the node to an identifier. This is used to
 * transform some keywords back to an identifier.
 *
 * \li NODE_PRIVATE -- "private"
 * \li NODE_PROTECTED -- "protected"
 * \li NODE_PUBLIC -- "public"
 *
 * At this point this is used to transform these keywords in labels.
 *
 * \note
 * The Node must not be locked.
 *
 * \return true if the conversion succeeded.
 */
bool Node::to_identifier()
{
    modifying();

    switch(f_type)
    {
    case node_t::NODE_IDENTIFIER:
        // already an identifier
        return true;

    case node_t::NODE_PRIVATE:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("private");
        return true;

    case node_t::NODE_PROTECTED:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("protected");
        return true;

    case node_t::NODE_PUBLIC:
        f_type = node_t::NODE_IDENTIFIER;
        set_string("public");
        return true;

    default:
        // failure (cannot convert)
        return false;

    }
    /*NOTREACHED*/
}


/** \brief Convert this node to a NODE_INT64.
 *
 * This function converts the node to an integer number,
 * just like JavaScript would do (outside of the fact that
 * JavaScript only supports floating points...) This means
 * converting the following type of nodes as specified:
 *
 * \li NODE_INT64 -- no conversion
 * \li NODE_FLOAT64 -- convert to integer
 * \li NODE_TRUE -- convert to 1
 * \li NODE_FALSE -- convert to 0
 * \li NODE_NULL -- convert to 0
 * \li NODE_STRING -- convert to integer if valid, zero otherwise (NaN is
 *                    not possible in an integer)
 * \li NODE_UNDEFINED -- convert to 0 (NaN is not possible in an integer)
 *
 * This function converts strings. If the string represents a
 * valid integer, convert to that integer. In this case the full 64 bits
 * are supported. If the string represents a floating point number, then
 * the number is first converted to a floating point, then cast to an
 * integer using the floor() function. If the floating point is too large
 * for the integer, then the maximum or minimum number are used as the
 * result. String that do not represent a number (integer or floating
 * point) are transformed to zero (0). This is a similar behavior to
 * the 'undefined' conversion.
 *
 * \note
 * The Node must not be locked.
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
        if(f_float.is_NaN() || f_float.is_infinity())
        {
            // the C-like cast would use 0x800...000
            // JavaScript expects zero instead
            f_int.set(0);
        }
        else
        {
            f_int.set(f_float.get()); // C-like cast to integer with a floor() (no rounding)
        }
        break;

    case node_t::NODE_TRUE:
        f_int.set(1);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
    case node_t::NODE_UNDEFINED: // should return NaN, not possible with an integer...
        f_int.set(0);
        break;

    case node_t::NODE_STRING:
        if(f_str.is_int64())
        {
            f_int.set(f_str.to_int64());
        }
        else if(f_str.is_float64())
        {
            f_int.set(f_str.to_float64()); // C-like cast to integer with a floor() (no rounding)
        }
        else
        {
            f_int.set(0); // should return NaN, not possible with an integer...
        }
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
 * This function converts the node to a floating point number,
 * just like JavaScript would do. This means converting the following
 * type of nodes:
 *
 * \li NODE_INT64 -- convert to a float
 * \li NODE_FLOAT64 -- no conversion
 * \li NODE_TRUE -- convert to 1.0
 * \li NODE_FALSE -- convert to 0.0
 * \li NODE_NULL -- convert to 0.0
 * \li NODE_STRING -- convert to float if valid, otherwise NaN
 * \li NODE_UNDEFINED -- convert to NaN
 *
 * This function converts strings. If the string represents an integer,
 * it will be converted to the nearest floating point number. If the
 * string does not represent a number (including an empty string),
 * then the float is set to NaN.
 *
 * \note
 * The Node must not be locked.
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

    case node_t::NODE_STRING:
        f_float.set(f_str.to_float64());
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
 * \note
 * The Node must not be locked.
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
 * This function converts the node to a number pretty much
 * like JavaScript would do, except that literals that represent
 * an exact integers are converted to an integer instead of a
 * floating point.
 *
 * If the node already is an integer or a floating point, then
 * no conversion takes place, but it is considered valid and
 * thus the function returns true.
 *
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
 * This function converts strings to a floating point, even if the
 * value represents an integer. It is done that way because JavaScript
 * expects a 'number' and that is expected to be a floating point.
 *
 * \note
 * The Node must not be locked.
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
        // when possible (actually it nearly always is, and strings
        // often become NaN as a result... the '+' and '+=' operators
        // are an exception; also relational operators do not convert
        // strings if both the left hand side and the right hand side
        // are strings.)
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
 * This function transforms a node from what it is to a string. If the
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
 * The conversion of a floating point is not one to one compatible with
 * what a JavaScript implementation would otherwise do. This is due to
 * the fact that Java tends to convert floating points in a slightly
 * different way than C/C++. None the less, the results are generally
 * very close (to the 4th decimal digit.)
 *
 * The NaN floating point is converted to the string "NaN".
 *
 * The floating point +0.0 and -0.0 numbers are converted to exactly "0".
 *
 * The floating point +Infinity is converted to the string "Infinity".
 *
 * The floating point -Infinity is converted to the string "-Infinity".
 *
 * Other numbers are converted as floating points with a decimal point,
 * although floating points that represent an integer may be output as
 * an integer.
 *
 * \note
 * The Node must not be locked.
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
            if(f_str.find('.') != f_str.npos)
            {
                while(f_str.back() == '0')
                {
                    f_str.pop_back();
                }
                if(f_str.back() == '.')
                {
                    f_str.pop_back();
                }
            }
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
 * \endcode
 *
 * In the first case, (a) is transform with the content of variable
 * 'a' and that resulting object is used to access 'field'.
 *
 * In the second case, 'a' itself represents an object and we are accessing
 * that object's 'field' directly.
 *
 * \note
 * Why do we need this distinction? Parenthesis used for grouping are
 * not saved in the resulting tree of nodes. For that reason, at the time
 * we parse that result, we could not distinguish between both
 * expressions. With the NODE_VIDENTIFIER, we can correct that problem.
 *
 * \note
 * The Node must not be locked.
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
 * \note
 * The Node must not be locked.
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
