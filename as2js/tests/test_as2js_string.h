#ifndef TEST_AS2JS_STRING_H
#define TEST_AS2JS_STRING_H
/* stream.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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


#include <cppunit/extensions/HelperMacros.h>


class As2JsStringUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsStringUnitTests );
        CPPUNIT_TEST( test_iso88591 );
        CPPUNIT_TEST( test_utf8 );
        CPPUNIT_TEST( test_utf16 );
        CPPUNIT_TEST( test_utf32 );
        CPPUNIT_TEST( test_number );
        CPPUNIT_TEST( test_concatenation );
    CPPUNIT_TEST_SUITE_END();

public:
    //void setUp();

protected:
    void test_iso88591();
    void test_utf8();
    void test_utf16();
    void test_utf32();
    void test_number();
    void test_concatenation();
};

#endif
// vim: ts=4 sw=4 et
