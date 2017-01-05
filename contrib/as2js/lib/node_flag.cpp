/* node_flag.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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
 * \brief Handle the node flags.
 *
 * Nodes accept a large set of flags (42 at time of writing).
 *
 * Flags are specific to node types. In an earlier implementation,
 * flags would overlap (i.e. the same bit would be used by different
 * flags, which flag was determine by the type of node being used.)
 * This was revamped to make use of unique flags in order to over
 * potential bugs.
 *
 * Flags being specific to a node type, the various functions below
 * make sure that the flags modified on a node are compatible with
 * that node.
 *
 * \todo
 * The conversion functions do not take flags in account. As far as
 * I know, at this point we cannot convert a node of a type that
 * accept a flag except with the to_unknown() function in which
 * case flags become irrelevant anyway. We should test that the
 * flags remain valid after a conversion.
 *
 * \todo
 * Mutually exclusive flags are not currently verified in this
 * code when it should be.
 */


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
 * the Node type we are dealing with.
 *
 * If the flag was never set, this function returns false.
 *
 * compare_all_flags() can be used to compare all the flags at once
 * without having to load each flag one at a time. This is particularly
 * useful in our unit tests.
 *
 * \param[in] f  The flag to retrieve.
 *
 * \return true if the flag was set to true, false otherwise.
 *
 * \sa set_flag()
 * \sa verify_flag()
 * \sa compare_all_flags()
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
 * the Node type we are dealing with.
 *
 * \param[in] f  The flag to set.
 * \param[in] v  The new value for the flag.
 *
 * \sa get_flag()
 * \sa verify_flag()
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
 * \todo
 * Move some of the external tests (tests done by code in other
 * places like the parser) to here because some flags are
 * mutally exclusive and we should prevent such from being set
 * simultaneously.
 *
 * \exception exception_internal_error
 * This function checks that the flag is allowed in the type of node.
 * If not, this exception is raised because that represents a compiler
 * bug.
 *
 * \param[in] f  The flag to check.
 *
 * \sa set_flag()
 * \sa get_flag()
 */
void Node::verify_flag(flag_t f) const
{
    switch(f)
    {
    case flag_t::NODE_CATCH_FLAG_TYPED:
        if(f_type == node_t::NODE_CATCH)
        {
            return;
        }
        break;

    case flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES:
        if(f_type == node_t::NODE_DIRECTIVE_LIST)
        {
            return;
        }
        break;

    case flag_t::NODE_ENUM_FLAG_CLASS:
    case flag_t::NODE_ENUM_FLAG_INUSE:
        if(f_type == node_t::NODE_ENUM)
        {
            return;
        }
        break;

    case flag_t::NODE_FOR_FLAG_CONST:
    case flag_t::NODE_FOR_FLAG_FOREACH:
    case flag_t::NODE_FOR_FLAG_IN:
        if(f_type == node_t::NODE_FOR)
        {
            return;
        }
        break;

    case flag_t::NODE_FUNCTION_FLAG_GETTER:
    case flag_t::NODE_FUNCTION_FLAG_NEVER:
    case flag_t::NODE_FUNCTION_FLAG_NOPARAMS:
    case flag_t::NODE_FUNCTION_FLAG_OPERATOR:
    case flag_t::NODE_FUNCTION_FLAG_OUT:
    case flag_t::NODE_FUNCTION_FLAG_SETTER:
    case flag_t::NODE_FUNCTION_FLAG_VOID:
        if(f_type == node_t::NODE_FUNCTION)
        {
            return;
        }
        break;

    case flag_t::NODE_IDENTIFIER_FLAG_WITH:
    case flag_t::NODE_IDENTIFIER_FLAG_TYPED:
        if(f_type == node_t::NODE_CLASS
        || f_type == node_t::NODE_IDENTIFIER
        || f_type == node_t::NODE_VIDENTIFIER
        || f_type == node_t::NODE_STRING)
        {
            return;
        }
        break;

    case flag_t::NODE_IMPORT_FLAG_IMPLEMENTS:
        if(f_type == node_t::NODE_IMPORT)
        {
            return;
        }
        break;

    case flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS:
    case flag_t::NODE_PACKAGE_FLAG_REFERENCED:
        if(f_type == node_t::NODE_PACKAGE)
        {
            return;
        }
        break;

    case flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED:
        if(f_type == node_t::NODE_PARAM_MATCH)
        {
            return;
        }
        break;

    case flag_t::NODE_PARAM_FLAG_CATCH:         // a parameter defined in a catch()
    case flag_t::NODE_PARAM_FLAG_CONST:
    case flag_t::NODE_PARAM_FLAG_IN:
    case flag_t::NODE_PARAM_FLAG_OUT:
    case flag_t::NODE_PARAM_FLAG_NAMED:
    case flag_t::NODE_PARAM_FLAG_PARAMREF:      // referenced from another parameter
    case flag_t::NODE_PARAM_FLAG_REFERENCED:    // referenced from a parameter or a variable
    case flag_t::NODE_PARAM_FLAG_REST:
    case flag_t::NODE_PARAM_FLAG_UNCHECKED:
    case flag_t::NODE_PARAM_FLAG_UNPROTOTYPED:
        if(f_type == node_t::NODE_PARAM)
        {
            return;
        }
        break;

    case flag_t::NODE_SWITCH_FLAG_DEFAULT:           // we found a 'default:' label in that switch
        if(f_type == node_t::NODE_SWITCH)
        {
            return;
        }
        break;

    case flag_t::NODE_TYPE_FLAG_MODULO:             // type ... as mod ...;
        if(f_type == node_t::NODE_TYPE)
        {
            return;
        }
        break;

    case flag_t::NODE_VARIABLE_FLAG_CONST:
    case flag_t::NODE_VARIABLE_FLAG_FINAL:
    case flag_t::NODE_VARIABLE_FLAG_LOCAL:
    case flag_t::NODE_VARIABLE_FLAG_MEMBER:
    case flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES:
    case flag_t::NODE_VARIABLE_FLAG_ENUM:                 // there is a NODE_SET and it somehow needs to be copied
    case flag_t::NODE_VARIABLE_FLAG_COMPILED:             // Expression() was called on the NODE_SET
    case flag_t::NODE_VARIABLE_FLAG_INUSE:                // this variable was referenced
    case flag_t::NODE_VARIABLE_FLAG_ATTRS:                // currently being read for attributes (to avoid loops)
    case flag_t::NODE_VARIABLE_FLAG_DEFINED:              // was already parsed
    case flag_t::NODE_VARIABLE_FLAG_DEFINING:             // currently defining, can't read
    case flag_t::NODE_VARIABLE_FLAG_TOADD:                // to be added in the directive list
        if(f_type == node_t::NODE_VARIABLE
        || f_type == node_t::NODE_VAR_ATTRIBUTES)
        {
            return;
        }
        break;

    case flag_t::NODE_FLAG_max:
        break;

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }

    // since we do not use 'default' completely invalid values are not caught
    // in the switch...
    throw exception_internal_error("node_flag.cpp: Node::verify_flag(): flag (" + std::to_string(static_cast<int>(f))
                                                        + ") / type missmatch (" + std::to_string(static_cast<int>(static_cast<node_t>(f_type))) + ")");
}


/** \brief Compare a set of flags with the current flags of this node.
 *
 * This function compares the specified set of flags with the node's
 * flags. If the sets are equal, then the function returns true.
 * Otherwise the function returns false.
 *
 * This function compares all the flags, whether or not they are
 * valid for the current node type.
 *
 * \param[in] s  The set of flags to compare with.
 *
 * \return true if \p s is equal to the node flags.
 *
 * \sa get_flag()
 */
bool Node::compare_all_flags(flag_set_t const& s) const
{
    return f_flags == s;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
