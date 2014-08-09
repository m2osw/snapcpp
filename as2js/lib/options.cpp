/* options.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include "as2js/options.h"


namespace as2js
{



/** \brief Initialize an options object.
 *
 * The default constructor initializes the options array to the maximum
 * number of options. The options are all set to the default, zero (0).
 *
 * To change the option value, use the set_option() function. At this
 * point pretty much all the options accept either 0 or 1 as their
 * value, although any number of than 0 is considered to represent "set".
 */
Options::Options()
    : f_options(static_cast<size_t>(option_t::OPTION_max))
{
}


/** \brief Set an option to the specified value.
 *
 * This function sets the option to the specified value.
 *
 * At this point, all options expect the value to be 0 or 1, although
 * the system does not enforce that at this point. Any value is thus
 * accepted.
 *
 * Options make use of an int64_t so any 64 bit values works.
 *
 * \param[in] option  The option to set.
 * \param[in] value  The new 64 bit value for this option.
 */
void Options::set_option(option_t option, option_value_t value)
{
    f_options[static_cast<size_t>(option)] = value;
}


/** \brief Retrieve the current value of an option.
 *
 * This function is used to retrieve the current value of an option.
 * At this point, all options are expected to be zero (0), the default,
 * or one (1). It is possible to set options to other values, though.
 *
 * \param[in] option  The option to retrieve.
 *
 * \return The current value of this option.
 */
Options::option_value_t Options::get_option(option_t option)
{
    return f_options[static_cast<size_t>(option)];
}


}
// namespace as2js

// vim: ts=4 sw=4 et
