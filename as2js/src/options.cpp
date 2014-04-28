/* options.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include "as2js/options.h"


namespace as2js
{



Options::Options()
    : f_options(AS_OPTION_max)
{
    // we're always in strict mode
    set_option(AS_OPTION_STRICT, 1);
}


void Options::set_option(option_t option, option_value_t value)
{
    // TODO: verify that we really want that here, we may prefer
    //       to give the user non-strict access to his code...
    // we're always in strict mode
    if(option == AS_OPTION_STRICT)
    {
        value = 1;
    }

    f_options[option] = value;
}


Options::option_value_t Options::get_option(option_t option)
{
    return f_options[option];
}


}
// namespace as2js

// vim: ts=4 sw=4 et
