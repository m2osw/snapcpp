#ifndef TEST_AS2JS_NODE_H
#define TEST_AS2JS_NODE_H
/* test_as2hs_node.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


#include    "as2js/node.h"


class As2JsNodeUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( As2JsNodeUnitTests );
        CPPUNIT_TEST( test_type );
        CPPUNIT_TEST( test_compare );
        CPPUNIT_TEST( test_conversions );
        CPPUNIT_TEST( test_tree );
        CPPUNIT_TEST( test_param );
        CPPUNIT_TEST( test_position );
        CPPUNIT_TEST( test_links );
        CPPUNIT_TEST( test_variables );
        CPPUNIT_TEST( test_labels );
        CPPUNIT_TEST( test_attributes );
        CPPUNIT_TEST( test_attribute_tree );

        // the display capability is for debug purposes but
        // we test it to make sure that we display the right
        // information
        CPPUNIT_TEST( test_display_all_types );
        CPPUNIT_TEST( test_display_unicode_string );
        CPPUNIT_TEST( test_display_flags );
        CPPUNIT_TEST( test_display_attributes );
        CPPUNIT_TEST( test_display_tree );
    CPPUNIT_TEST_SUITE_END();

public:
    //void setUp();

protected:
    void test_type();
    void test_compare();
    void test_conversions();
    void test_tree();
    void test_param();
    void test_position();
    void test_links();
    void test_variables();
    void test_labels();
    void test_attributes();
    void test_attribute_tree();
    void test_display_all_types();
    void test_display_unicode_string();
    void test_display_flags();
    void test_display_attributes();
    void test_display_tree();

    // helper functions
    bool in_conflict(size_t j, as2js::Node::attribute_t attr, as2js::Node::attribute_t a) const;
};

#endif
// vim: ts=4 sw=4 et
