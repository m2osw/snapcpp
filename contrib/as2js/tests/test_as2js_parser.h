#ifndef TEST_AS2JS_PARSER_H
#define TEST_AS2JS_PARSER_H
/* test_as2js_parser.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


class As2JsParserUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsParserUnitTests );
        CPPUNIT_TEST( test_parser_array );
        CPPUNIT_TEST( test_parser_basics );
        CPPUNIT_TEST( test_parser_class );
        CPPUNIT_TEST( test_parser_enum );
        CPPUNIT_TEST( test_parser_for );
        CPPUNIT_TEST( test_parser_function );
        CPPUNIT_TEST( test_parser_if );
        CPPUNIT_TEST( test_parser_pragma );
        CPPUNIT_TEST( test_parser_switch );
        CPPUNIT_TEST( test_parser_synchronized );
        CPPUNIT_TEST( test_parser_trycatch );
        CPPUNIT_TEST( test_parser_type );
        CPPUNIT_TEST( test_parser_variable );
        CPPUNIT_TEST( test_parser_while );
        CPPUNIT_TEST( test_parser_yield );
        CPPUNIT_TEST( test_parser );
    CPPUNIT_TEST_SUITE_END();

public:
    //virtual void setUp();

protected:
    void test_parser_array();
    void test_parser_basics();
    void test_parser_class();
    void test_parser_enum();
    void test_parser_for();
    void test_parser_function();
    void test_parser_if();
    void test_parser_pragma();
    void test_parser_switch();
    void test_parser_synchronized();
    void test_parser_trycatch();
    void test_parser_type();
    void test_parser_variable();
    void test_parser_while();
    void test_parser_yield();
    void test_parser();
};

#endif
// vim: ts=4 sw=4 et
