// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include "libaddr/addr.h"

// C++ lib
//
#include <algorithm>
#include <sstream>
#include <iostream>

// C lib
//
#include <ifaddrs.h>
#include <netdb.h>



namespace addr
{






/** \brief Return true if the range has a 'from' address defined.
 *
 * By default the 'from' and 'to' addresses of an addr_range are legal
 * but considered undefined. After you called the set_from() function
 * once, this function will always return true.
 *
 * \return false until 'set_from()' is called at least once.
 */
bool addr_range::has_from() const
{
    return f_has_from;
}


/** \brief Return true if the range has a 'to' address defined.
 *
 * By default the 'from' and 'to' addresses of an addr_range are legal
 * but considered undefined. After you called the set_to() function
 * once, this function will always return true.
 *
 * \return false until 'set_to()' is called at least once.
 */
bool addr_range::has_to() const
{
    return f_has_to;
}


/** \brief Determine whether an addr_range object is considered a range.
 *
 * This function returns false until both, set_from() and set_to(),
 * were called.
 *
 * Note that the order in which the two functions get called is not
 * important, although we generally expect set_from() to be called
 * first, it does not matter.
 *
 * \return true if both, 'from' and 'to', were set.
 */
bool addr_range::is_range() const
{
    return f_has_from && f_has_to;
}


/** \brief Check whether this range is empty.
 *
 * If you defined the 'from' and 'to' addresses of the range, then you
 * can check whether the range is empty or not.
 *
 * A range is considered empty if 'from' is larger than 'to' because
 * in that case nothing can appear in between (no IP can at the same
 * time be larger than 'from' and smaller than 'to' if 'from > to'
 * is true.)
 *
 * \return true if 'from > to' and is_range() returns true.
 *
 * \sa is_range()
 * \sa has_from()
 * \sa has_to()
 */
bool addr_range::is_empty() const
{
    if(!is_range())
    {
        return false;
    }
    return f_from > f_to;
}


/** \brief Check whether \p rhs is part of this range.
 *
 * If the address specified in rhs is part of this range, then the function
 * returns true. The 'from' and 'to' addresses are considered inclusive,
 * so if rhs is equal to 'from' or 'to', then the function returns true.
 *
 * If 'from' is larger than 'to' then the function already returns false
 * since the range represents an empty range.
 *
 * \exception addr_invalid_state_exception
 * The addr_range object must be a range or this function throws this
 * exception. To test whether you can call this function, first call
 * the is_range() function. If it returns true, then is_in() is available.
 *
 * \param[in] rhs  The address to check for inclusion.
 *
 * \return true if rhs is considered part of this range.
 */
bool addr_range::is_in(addr const & rhs) const
{
    if(!is_range())
    {
        throw addr_invalid_state_exception("addr_range::is_in(): range is not complete (from or to missing.)");
    }

    if(f_from <= f_to)
    {
        //
        return rhs >= f_from && rhs <= f_to;
    }
    //else -- from/to are swapped... this represents an empty range

    return false;
}


/** \brief Set 'from' address.
 *
 * This function saves the 'from' address in this range object.
 *
 * Once this function was called at least once, the has_from() returns true.
 *
 * \param[in] from  The address to save as the 'from' address.
 */
void addr_range::set_from(addr const & from)
{
    f_has_from = true;
    f_from = from;
}


/** \brief Get 'from' address.
 *
 * This function return the 'from' address as set by the set_from()
 * functions.
 *
 * The get_from() function can be called even if the has_from()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'from' address.
 */
addr & addr_range::get_from()
{
    return f_from;
}


/** \brief Get the 'from' address when addr_range is constant.
 *
 * This function return the 'from' address as set by the set_from()
 * functions.
 *
 * The get_from() function can be called even if the has_from()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'from' address.
 */
addr const & addr_range::get_from() const
{
    return f_from;
}


/** \brief Set 'to' address.
 *
 * This function saves the 'to' address in this range object.
 *
 * Once this function was called at least once, the has_to() returns true.
 *
 * \param[in] to  The address to save as the 'to' address.
 */
void addr_range::set_to(addr const & to)
{
    f_has_to = true;
    f_to = to;
}


/** \brief Get the 'to' address.
 *
 * This function return the 'to' address as set by the set_to()
 * function.
 *
 * The get_from() function can be called even if the has_from()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'to' address.
 */
addr & addr_range::get_to()
{
    return f_to;
}


/** \brief Get the 'to' address when addr_range is constant.
 *
 * This function return the 'to' address as set by the set_to()
 * function.
 *
 * The get_to() function can be called even if the has_to()
 * function returns false. It will return a default address
 * (a new 'addr' object.)
 *
 * \return The address saved as the 'to' address.
 */
addr const & addr_range::get_to() const
{
    return f_to;
}


/** \brief Compute a new range with the part that is shared between both inputs.
 *
 * This function computers a range which encompasses all the addresses found
 * in \p this range and \p rhs range.
 *
 * If the two range do not intersect, then the resulting range will be an
 * empty range (see is_empty() for details).
 *
 * The new range receives the largest 'from' address from both inputs and
 * the smallest 'to' address from both inputs.
 *
 * \param[in] rhs  The other range to compute the intersection with.
 *
 * \return The resulting intersection range.
 */
addr_range addr_range::intersection(addr_range const & rhs) const
{
    addr_range result;

    result.set_from(f_from > rhs.f_from ? f_from : rhs.f_from);
    result.set_to  (f_to   < rhs.f_to   ? f_to   : rhs.f_to);

    return result;
}







}
// snap_addr namespace
// vim: ts=4 sw=4 et
