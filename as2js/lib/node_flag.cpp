/* node_flag.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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
/***  NODE FLAG  ******************************************************/
/**********************************************************************/
/**********************************************************************/


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


}
// namespace as2js

// vim: ts=4 sw=4 et
