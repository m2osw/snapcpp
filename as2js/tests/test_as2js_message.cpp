/* test_as2js_message.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "test_as2js_message.h"
#include    "test_as2js_main.h"

#include    "as2js/message.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsMessageUnitTests );




class test_callback : public as2js::MessageCallback
{
public:
    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
    {

//std::cerr<< "msg = " << pos.get_filename() << " / " << f_expected_pos.get_filename() << "\n";

        CPPUNIT_ASSERT(f_expected_call);
        CPPUNIT_ASSERT(message_level == f_expected_message_level);
        CPPUNIT_ASSERT(error_code == f_expected_error_code);
        CPPUNIT_ASSERT(pos.get_filename() == f_expected_pos.get_filename());
        CPPUNIT_ASSERT(pos.get_function() == f_expected_pos.get_function());
        CPPUNIT_ASSERT(pos.get_page() == f_expected_pos.get_page());
        CPPUNIT_ASSERT(pos.get_page_line() == f_expected_pos.get_page_line());
        CPPUNIT_ASSERT(pos.get_paragraph() == f_expected_pos.get_paragraph());
        CPPUNIT_ASSERT(pos.get_line() == f_expected_pos.get_line());
        CPPUNIT_ASSERT(message == f_expected_message);

        if(message_level == as2js::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CPPUNIT_ASSERT(g_warning_count == as2js::Message::warning_count());
        }

        if(message_level == as2js::MESSAGE_LEVEL_FATAL
        || message_level == as2js::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::Message::error_count() << "\n";
            CPPUNIT_ASSERT(g_error_count == as2js::Message::error_count());
        }

        f_got_called = true;
    }

    controlled_vars::tbool_t    f_expected_call;
    controlled_vars::fbool_t    f_got_called;
    as2js::message_level_t      f_expected_message_level;
    as2js::err_code_t           f_expected_error_code;
    as2js::Position             f_expected_pos;
    std::string                 f_expected_message; // UTF-8 string
    static controlled_vars::zint32_t   g_warning_count;
    static controlled_vars::zint32_t   g_error_count;
};

controlled_vars::zint32_t   test_callback::g_warning_count;
controlled_vars::zint32_t   test_callback::g_error_count;


void As2JsMessageUnitTests::test_message()
{
    for(as2js::message_level_t i(as2js::MESSAGE_LEVEL_OFF); i <= as2js::MESSAGE_LEVEL_TRACE; i = static_cast<as2js::message_level_t>(static_cast<int>(i) + 1))
    {
//i = static_cast<as2js::message_level_t>(static_cast<int>(i) + 1);
        std::cerr << "[" << static_cast<int32_t>(i) << "]";

        for(as2js::err_code_t j(as2js::AS_ERR_NONE); j <= as2js::AS_ERR_max; j = static_cast<as2js::err_code_t>(static_cast<int>(j) + 1))
        {
            std::cerr << ".";

            {
                test_callback c;
                c.f_expected_message_level = i;
                c.f_expected_error_code = j;
                c.f_expected_pos.set_filename("unknown-file");
                c.f_expected_pos.set_function("unknown-func");
                as2js::Message::set_message_callback(&c);
                for(as2js::message_level_t k(as2js::MESSAGE_LEVEL_OFF); k <= as2js::MESSAGE_LEVEL_TRACE; k = static_cast<as2js::message_level_t>(static_cast<int>(k) + 1))
                {
                    as2js::Message::set_message_level(k);
                    as2js::message_level_t min(k < as2js::MESSAGE_LEVEL_ERROR ? as2js::MESSAGE_LEVEL_ERROR : k);
                    c.f_expected_call = i != as2js::MESSAGE_LEVEL_OFF && i <= min;
//std::cerr << "i == " << static_cast<int32_t>(i) << ", k == " << static_cast<int32_t>(k) << ", min == " << static_cast<int32_t>(min) << " expect = " << c.f_expected_call << "\n";
                    {
                        c.f_got_called = false;
                        c.f_expected_message = "";
                        as2js::Message msg(i, j);
                    }
                    CPPUNIT_ASSERT(!c.f_got_called); // no message no call
                    {
                        c.f_got_called = false;
                        c.f_expected_message = "with a message";
                        as2js::Message msg(i, j);
                        msg << "with a message";
                    }
                    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
                }
            }

            as2js::Position pos;
            pos.set_filename("file.js");
            int total_line(1);
            for(int page(1); page < 10; ++page)
            {
                //std::cerr << "+";

                int paragraphs(rand() % 10 + 10);
                int page_line(1);
                int paragraph(1);
                for(int line(1); line < 100; ++line)
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

                    {
                        test_callback c;
                        c.f_expected_message_level = i;
                        c.f_expected_error_code = j;
                        c.f_expected_pos = pos;
                        c.f_expected_pos.set_filename("file.js");
                        c.f_expected_pos.set_function("unknown-func");
                        as2js::Message::set_message_callback(&c);
                        for(as2js::message_level_t k(as2js::MESSAGE_LEVEL_OFF); k <= as2js::MESSAGE_LEVEL_TRACE; k = static_cast<as2js::message_level_t>(static_cast<int>(k) + 1))
                        {
                            as2js::Message::set_message_level(k);
                            as2js::message_level_t min(k < as2js::MESSAGE_LEVEL_ERROR ? as2js::MESSAGE_LEVEL_ERROR : k);
                            c.f_expected_call = i != as2js::MESSAGE_LEVEL_OFF && i <= min;
                            {
                                c.f_got_called = false;
                                c.f_expected_message = "";
                                as2js::Message msg(i, j, pos);
                            }
                            CPPUNIT_ASSERT(!c.f_got_called);
                            {
                                c.f_got_called = false;
                                c.f_expected_message = "and a small message";
                                as2js::Message msg(i, j, pos);
                                msg << "and a small message";
                            }
                            CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
                        }
                    }

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
}




// vim: ts=4 sw=4 et
