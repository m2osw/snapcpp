/* test_as2js_position.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_position.h"
#include    "test_as2js_main.h"

#include    "as2js/position.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>
#include    <sstream>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsPositionUnitTests );





void As2JsPositionUnitTests::test_names()
{
    as2js::Position pos;

    // check the filename
    {
        // by default it is empty
        CPPUNIT_ASSERT(pos.get_filename() == "");

        // some long filename
        pos.set_filename("the/filename/can really/be anything.test");
        CPPUNIT_ASSERT(pos.get_filename() == "the/filename/can really/be anything.test");

        // reset back to empty
        pos.set_filename("");
        CPPUNIT_ASSERT(pos.get_filename() == "");

        // reset back to empty
        pos.set_filename("file.js");
        CPPUNIT_ASSERT(pos.get_filename() == "file.js");
    }

    // check the function name
    {
        // by default it is empty
        CPPUNIT_ASSERT(pos.get_function() == "");

        // some long filename
        pos.set_function("as2js::super::function::name");
        CPPUNIT_ASSERT(pos.get_function() == "as2js::super::function::name");

        // reset back to empty
        pos.set_function("");
        CPPUNIT_ASSERT(pos.get_function() == "");

        // reset back to empty
        pos.set_function("add");
        CPPUNIT_ASSERT(pos.get_function() == "add");
    }
}


void As2JsPositionUnitTests::test_counters()
{
    as2js::Position pos;

    // frist verify the default
    {
        CPPUNIT_ASSERT(pos.get_page() == 1);
        CPPUNIT_ASSERT(pos.get_page_line() == 1);
        CPPUNIT_ASSERT(pos.get_paragraph() == 1);
        CPPUNIT_ASSERT(pos.get_line() == 1);
    }

    int total_line(1);
    for(int page(1); page < 100; ++page)
    {
        int paragraphs(rand() % 10 + 10);
        int page_line(1);
        int paragraph(1);
        for(int line(1); line < 1000; ++line)
        {
            CPPUNIT_ASSERT(pos.get_page() == page);
            CPPUNIT_ASSERT(pos.get_page_line() == page_line);
            CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
            CPPUNIT_ASSERT(pos.get_line() == total_line);

            if(line % paragraphs == 0)
            {
                pos.new_paragraph();
                ++paragraph;
            }
            pos.new_line();
            ++total_line;
            ++page_line;
        }
        pos.new_page();
    }

    // by default, the reset counters resets everything back to 1
    {
        pos.reset_counters();
        CPPUNIT_ASSERT(pos.get_page() == 1);
        CPPUNIT_ASSERT(pos.get_page_line() == 1);
        CPPUNIT_ASSERT(pos.get_paragraph() == 1);
        CPPUNIT_ASSERT(pos.get_line() == 1);
    }

    // we can also define the start line
    int last_line(1);
    for(int idx(1); idx < 250; ++idx)
    {
        int line(rand() % 20000);
        if(idx % 13 == 0)
        {
            // force a negative number to test the throw
            line = -line;
        }
        if(line < 1)
        {
            // this throws because the line # is not valid
            CPPUNIT_ASSERT_THROW(pos.reset_counters(line), as2js::exception_internal_error);

            // the counters are unchanged in that case
            CPPUNIT_ASSERT(pos.get_page() == 1);
            CPPUNIT_ASSERT(pos.get_page_line() == 1);
            CPPUNIT_ASSERT(pos.get_paragraph() == 1);
            CPPUNIT_ASSERT(pos.get_line() == last_line);
        }
        else
        {
            pos.reset_counters(line);
            CPPUNIT_ASSERT(pos.get_page() == 1);
            CPPUNIT_ASSERT(pos.get_page_line() == 1);
            CPPUNIT_ASSERT(pos.get_paragraph() == 1);
            CPPUNIT_ASSERT(pos.get_line() == line);
            last_line = line;
        }
    }
}


void As2JsPositionUnitTests::test_output()
{
    // no filename
    {
        as2js::Position pos;
        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CPPUNIT_ASSERT(pos.get_page() == page);
                CPPUNIT_ASSERT(pos.get_page_line() == page_line);
                CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
                CPPUNIT_ASSERT(pos.get_line() == total_line);

                std::stringstream pos_str;
                pos_str << pos;
                std::stringstream test_str;
                test_str << "line " << total_line << ":";
                CPPUNIT_ASSERT(pos_str.str() == test_str.str());

                if(line % paragraphs == 0)
                {
                    pos.new_paragraph();
                    ++paragraph;
                }
                pos.new_line();
                ++total_line;
                ++page_line;
            }
            pos.new_page();
        }
    }

    {
        as2js::Position pos;
        pos.set_filename("file.js");
        int total_line(1);
        for(int page(1); page < 100; ++page)
        {
            int paragraphs(rand() % 10 + 10);
            int page_line(1);
            int paragraph(1);
            for(int line(1); line < 1000; ++line)
            {
                CPPUNIT_ASSERT(pos.get_page() == page);
                CPPUNIT_ASSERT(pos.get_page_line() == page_line);
                CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
                CPPUNIT_ASSERT(pos.get_line() == total_line);

                std::stringstream pos_str;
                pos_str << pos;
                std::stringstream test_str;
                test_str << "file.js:" << total_line << ":";
                CPPUNIT_ASSERT(pos_str.str() == test_str.str());

                if(line % paragraphs == 0)
                {
                    pos.new_paragraph();
                    ++paragraph;
                }
                pos.new_line();
                ++total_line;
                ++page_line;
            }
            pos.new_page();
        }
    }
}


// vim: ts=4 sw=4 et
