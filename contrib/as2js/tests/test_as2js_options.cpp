/* test_as2js_options.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_options.h"
#include    "test_as2js_main.h"

#include    "as2js/options.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsOptionsUnitTests );





void As2JsOptionsUnitTests::test_options()
{
    as2js::Options::pointer_t opt(new as2js::Options);

    // verify that all options are set to zero by default
    for(as2js::Options::option_t o(as2js::Options::option_t::OPTION_UNKNOWN); o < as2js::Options::option_t::OPTION_max; o = static_cast<as2js::Options::option_t>(static_cast<int>(o) + 1))
    {
        CPPUNIT_ASSERT(opt->get_option(o) == 0);
    }

    for(as2js::Options::option_t o(as2js::Options::option_t::OPTION_UNKNOWN); o < as2js::Options::option_t::OPTION_max; o = static_cast<as2js::Options::option_t>(static_cast<int>(o) + 1))
    {
        for(int i(0); i < 100; ++i)
        {
            int64_t value((static_cast<int64_t>(rand()) << 48)
                        ^ (static_cast<int64_t>(rand()) << 32)
                        ^ (static_cast<int64_t>(rand()) << 16)
                        ^ (static_cast<int64_t>(rand()) <<  0));
            opt->set_option(o, value);
            CPPUNIT_ASSERT(opt->get_option(o) == value);
        }
    }
}




// vim: ts=4 sw=4 et
