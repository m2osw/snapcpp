/* node_compare.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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
 * \brief Compare two nodes against each others.
 *
 * This function is used to compare two nodes against each others. The
 * compare is expected to return a compare_t enumeration value.
 *
 * At this time, the implementation only compares basic literals (i.e.
 * integers, floating points, strings, Booleans, null, undefined.)
 *
 * \todo
 * Implement a compare of object and array literals.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE COMPARE  ***************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Compare two nodes together.
 *
 * This function returns the result of comparing two nodes against each
 * others. The result is one of the compare_t values.
 *
 * At this time, if the function is used to compare nodes that are not
 * literals, then it returns COMPARE_ERROR.
 *
 * The function may return COMPARE_UNORDERED in strict mode or when
 * comparing a value against a NaN.
 *
 * As per the ECMAScript refence, strings are compared as is in binary
 * mode. We do not make use of Unicode or take the locale in account.
 *
 * \note
 * The compare is expected to work as defined in ECMAScript 5 (see 11.8.5,
 * 11.9.3, and 11.9.6).
 *
 * The nearly equal is only used by the smart match operator. This is an
 * addition by as2js which is somewhat like the ~~ operator defined by
 * perl.
 *
 * \param[in] lhs  The left hand side node.
 * \param[in] rhs  The right hand side node.
 * \param[in] mode  Whether the compare is strict, lousy, or smart
 *                  (===, ==, ~~).
 *
 * \return One of the compare_t values representing the comparison result.
 */
compare_t Node::compare(Node::pointer_t const lhs, Node::pointer_t const rhs, compare_mode_t const mode)
{
    if(!lhs->is_literal() || !rhs->is_literal())
    {
        // invalid left or right hand side
        return compare_t::COMPARE_ERROR;
    }

    // since we do not have a NODE_BOOLEAN, but instead have NODE_TRUE
    // and NODE_FALSE, we have to handle that case separately
    if(lhs->f_type == node_t::NODE_FALSE)
    {
        if(rhs->f_type == node_t::NODE_FALSE)
        {
            return compare_t::COMPARE_EQUAL;
        }
        if(rhs->f_type == node_t::NODE_TRUE)
        {
            return compare_t::COMPARE_LESS;
        }
    }
    else if(lhs->f_type == node_t::NODE_TRUE)
    {
        if(rhs->f_type == node_t::NODE_FALSE)
        {
            return compare_t::COMPARE_GREATER;
        }
        if(rhs->f_type == node_t::NODE_TRUE)
        {
            return compare_t::COMPARE_EQUAL;
        }
    }

    // exact same type?
    if(lhs->f_type == rhs->f_type)
    {
        switch(lhs->f_type)
        {
        case node_t::NODE_FLOAT64:
            // NaN is a special case we have to take in account
            if(mode == compare_mode_t::COMPARE_SMART && lhs->get_float64().nearly_equal(rhs->get_float64()))
            {
                return compare_t::COMPARE_EQUAL;
            }
            return lhs->get_float64().compare(rhs->get_float64());

        case node_t::NODE_INT64:
            return lhs->get_int64().compare(rhs->get_int64());

        case node_t::NODE_NULL:
            return compare_t::COMPARE_EQUAL;

        case node_t::NODE_STRING:
            if(lhs->f_str == rhs->f_str)
            {
                return compare_t::COMPARE_EQUAL;
            }
            return lhs->f_str < rhs->f_str ? compare_t::COMPARE_LESS : compare_t::COMPARE_GREATER;

        case node_t::NODE_UNDEFINED:
            return compare_t::COMPARE_EQUAL;

        default:
            throw exception_internal_error("INTERNAL ERROR: comparing two literal nodes with an unknown type."); // LCOV_EXCL_LINE

        }
        /*NOTREACHED*/
    }

    // if strict mode is turned on, we cannot compare objects
    // that are not of the same type (i.e. no conversions allowed)
    if(mode == compare_mode_t::COMPARE_STRICT)
    {
        return compare_t::COMPARE_UNORDERED;
    }

    // we handle one special case here since 'undefined' is otherwise
    // converted to NaN and it would not be equal to 'null' which is
    // represented as being equal to zero
    if((lhs->f_type == node_t::NODE_NULL && rhs->f_type == node_t::NODE_UNDEFINED)
    || (lhs->f_type == node_t::NODE_UNDEFINED && rhs->f_type == node_t::NODE_NULL))
    {
        return compare_t::COMPARE_EQUAL;
    }

    // if we are here, we have go to convert both parameters to floating
    // point numbers and compare the result (remember that we do not handle
    // objects in this functon)
    Float64 lf;
    switch(lhs->f_type)
    {
    case node_t::NODE_INT64:
        lf.set(lhs->f_int.get());
        break;

    case node_t::NODE_FLOAT64:
        lf = lhs->f_float;
        break;

    case node_t::NODE_TRUE:
        lf.set(1.0);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        lf.set(0.0);
        break;

    case node_t::NODE_STRING:
        lf.set(lhs->f_str.to_float64());
        break;

    case node_t::NODE_UNDEFINED:
        lf.set_NaN();
        break;

    default:
        // failure (cannot convert)
        throw exception_internal_error("INTERNAL ERROR: could not convert a literal node to a floating point (lhs)."); // LCOV_EXCL_LINE

    }

    Float64 rf;
    switch(rhs->f_type)
    {
    case node_t::NODE_INT64:
        rf.set(rhs->f_int.get());
        break;

    case node_t::NODE_FLOAT64:
        rf = rhs->f_float;
        break;

    case node_t::NODE_TRUE:
        rf.set(1.0);
        break;

    case node_t::NODE_NULL:
    case node_t::NODE_FALSE:
        rf.set(0.0);
        break;

    case node_t::NODE_STRING:
        rf.set(rhs->f_str.to_float64());
        break;

    case node_t::NODE_UNDEFINED:
        rf.set_NaN();
        break;

    default:
        // failure (cannot convert)
        throw exception_internal_error("INTERNAL ERROR: could not convert a literal node to a floating point (rhs)."); // LCOV_EXCL_LINE

    }

    if(mode == compare_mode_t::COMPARE_SMART && lf.nearly_equal(rf))
    {
        return compare_t::COMPARE_EQUAL;
    }

    return lf.compare(rf);
}


}
// namespace as2js

// vim: ts=4 sw=4 et
