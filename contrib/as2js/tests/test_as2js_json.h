#ifndef TEST_AS2JS_JSON_H
#define TEST_AS2JS_JSON_H
/* test_as2js_json.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


#include <cppunit/extensions/HelperMacros.h>


class As2JsJSONUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsJSONUnitTests );
        CPPUNIT_TEST( test_basic_values );
        CPPUNIT_TEST( test_array_value );
        CPPUNIT_TEST( test_object_value );
        CPPUNIT_TEST( test_json );
        CPPUNIT_TEST( test_json_with_positive_numbers );
        CPPUNIT_TEST( test_error );
    CPPUNIT_TEST_SUITE_END();

public:
    //void setUp();

protected:
    void test_basic_values();
    void test_array_value();
    void test_object_value();
    void test_json();
    void test_json_with_positive_numbers();
    void test_error();
};

#endif
// vim: ts=4 sw=4 et
