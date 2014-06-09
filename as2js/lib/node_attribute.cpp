/* node_attribute.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE ATTRIBUTE  *************************************************/
/**********************************************************************/
/**********************************************************************/

namespace
{

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
// no name namesoace


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
 * \param[in] a  The flag or attribute to check.
 */
void Node::verify_attribute(attribute_t a) const
{
    switch(a)
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
        if(f_type != node_t::NODE_PROGRAM)
        {
            return;
        }
        break;

    // attributes were defined
    case attribute_t::NODE_ATTR_DEFINED:
        // all nodes can receive this flag
        return;

    case attribute_t::NODE_ATTR_max:
        break;

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }

    throw exception_internal_error("attribute / type missmatch in Node::verify_attribute()");
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

    case attribute_t::NODE_ATTR_max:
        // this should already have been caught in the verify_attribute() function
        throw exception_internal_error("invalid attribute / flag in Node::verify_attribute()"); // LCOV_EXCL_LINE

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    // note: verify_attribute(() is called before this function
    //       and it catches completely invalid attribute numbers
    //       (i.e. negative, larger than NODE_ATTR_max)
    }

    if(conflict)
    {
        // this can be a user error so we emit an error instead of throwing
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_position);
        msg << "Attributes " << names << " are mutually exclusive. Only one of them can be used.";
    }
}


}
// namespace as2js

// vim: ts=4 sw=4 et
