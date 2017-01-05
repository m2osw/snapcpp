#ifndef TEST_AS2JS_STREAM_H
#define TEST_AS2JS_STREAM_H
/* test_as2hs_stream.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


class As2JsStreamUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsStreamUnitTests );
        CPPUNIT_TEST( test_filter_iso88591 );
        CPPUNIT_TEST( test_filter_utf8 );
        CPPUNIT_TEST( test_filter_utf16 );
        CPPUNIT_TEST( test_filter_utf32 );
        CPPUNIT_TEST( test_filter_detect );
        CPPUNIT_TEST( test_string_input );
        CPPUNIT_TEST( test_stdin );
        CPPUNIT_TEST( test_file );
        CPPUNIT_TEST( test_bad_impl );
        CPPUNIT_TEST( test_stdout );
        CPPUNIT_TEST( test_output );
        CPPUNIT_TEST( test_string_output );

        // keep this one last because we lose stdout after that...
        // (we may be able to fix the problem with a fork()...)
        CPPUNIT_TEST( test_stdout_destructive );
    CPPUNIT_TEST_SUITE_END();

public:
    //void setUp();

protected:
    void test_filter_iso88591();
    void test_filter_utf8();
    void test_filter_utf16();
    void test_filter_utf32();
    void test_filter_detect();
    void test_string_input();
    void test_stdin();
    void test_file();
    void test_bad_impl();
    void test_stdout();
    void test_output();
    void test_string_output();

    // keep this one last because we lose stdout after that...
    void test_stdout_destructive();
};

#endif
// vim: ts=4 sw=4 et
