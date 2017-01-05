#ifndef TEST_AS2JS_OPTIMIZER_H
#define TEST_AS2JS_OPTIMIZER_H
/* test_as2js_optimizer.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


class As2JsOptimizerUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsOptimizerUnitTests );
        CPPUNIT_TEST( test_optimizer_invalid_nodes );
        CPPUNIT_TEST( test_optimizer_additive );
        CPPUNIT_TEST( test_optimizer_assignments );
        CPPUNIT_TEST( test_optimizer_bitwise );
        CPPUNIT_TEST( test_optimizer_compare );
        CPPUNIT_TEST( test_optimizer_conditional );
        CPPUNIT_TEST( test_optimizer_equality );
        CPPUNIT_TEST( test_optimizer_logical );
        CPPUNIT_TEST( test_optimizer_match );
        CPPUNIT_TEST( test_optimizer_multiplicative );
        CPPUNIT_TEST( test_optimizer_relational );
        CPPUNIT_TEST( test_optimizer_statements );
    CPPUNIT_TEST_SUITE_END();

public:
    //virtual void setUp();

protected:
    void test_optimizer_invalid_nodes();
    void test_optimizer_additive();
    void test_optimizer_assignments();
    void test_optimizer_bitwise();
    void test_optimizer_compare();
    void test_optimizer_conditional();
    void test_optimizer_equality();
    void test_optimizer_logical();
    void test_optimizer_match();
    void test_optimizer_multiplicative();
    void test_optimizer_relational();
    void test_optimizer_statements();
};

#endif
// vim: ts=4 sw=4 et
