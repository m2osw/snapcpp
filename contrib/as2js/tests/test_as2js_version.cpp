/* test_as2js_version.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_version.h"
#include    "test_as2js_main.h"

#include    "as2js/options.h"
#include    "as2js/exceptions.h"
#include    "as2js/as2js.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsVersionUnitTests );


bool is_dec(char const *s)
{
    while(*s >= '0' && *s <= '9')
    {
        ++s;
    }
    return *s != '\0';
}



void As2JsVersionUnitTests::test_version()
{
    // verify that the library returns the expected version
    std::string v(as2js::as2js_library_version());
    CPPUNIT_ASSERT(v == AS2JS_VERSION);

    // verify that the version is well formed
    std::stringstream str;
    str << AS2JS_VERSION_MAJOR << "." << AS2JS_VERSION_MINOR << "." << AS2JS_VERSION_PATCH;
    CPPUNIT_ASSERT(v == str.str());

    std::stringstream vers;
    vers << AS2JS_VERSION_MAJOR << AS2JS_VERSION_MINOR << AS2JS_VERSION_PATCH;
    CPPUNIT_ASSERT(is_dec(str.str().c_str()));
}




// vim: ts=4 sw=4 et
