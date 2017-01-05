/* test_as2js_message.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_message.h"
#include    "test_as2js_main.h"

#include    "as2js/message.h"
#include    "as2js/exceptions.h"

#include    <climits>
#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsMessageUnitTests );



namespace
{

class test_callback : public as2js::MessageCallback
{
public:
    test_callback()
    {
        as2js::Message::set_message_callback(this);
        g_warning_count = as2js::Message::warning_count();
        g_error_count = as2js::Message::error_count();
    }

    ~test_callback()
    {
        // make sure the pointer gets reset!
        as2js::Message::set_message_callback(nullptr);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
    {

//std::cerr<< " filename = " << pos.get_filename() << " / " << f_expected_pos.get_filename() << "\n";
//std::cerr<< " msg = [" << message << "] / [" << f_expected_message << "]\n";

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

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CPPUNIT_ASSERT(g_warning_count == as2js::Message::warning_count());
        }

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
        || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::Message::error_count() << "\n";
            CPPUNIT_ASSERT(g_error_count == as2js::Message::error_count());
        }

        f_got_called = true;
    }

    bool                        f_expected_call = true;
    bool                        f_got_called = false;
    as2js::message_level_t      f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
    as2js::err_code_t           f_expected_error_code = as2js::err_code_t::AS_ERR_NONE;
    as2js::Position             f_expected_pos;
    std::string                 f_expected_message; // UTF-8 string

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;

}
// no name namespace



void As2JsMessageUnitTests::test_message()
{
    for(as2js::message_level_t i(as2js::message_level_t::MESSAGE_LEVEL_OFF);
                               i <= as2js::message_level_t::MESSAGE_LEVEL_TRACE;
                               i = static_cast<as2js::message_level_t>(static_cast<int>(i) + 1))
    {
//i = static_cast<as2js::message_level_t>(static_cast<int>(i) + 1);
        std::cerr << "[" << static_cast<int32_t>(i) << "]";

        for(as2js::err_code_t j(as2js::err_code_t::AS_ERR_NONE); j <= as2js::err_code_t::AS_ERR_max; j = static_cast<as2js::err_code_t>(static_cast<int>(j) + 1))
        {
            std::cerr << ".";

            {
                test_callback c;
                c.f_expected_message_level = i;
                c.f_expected_error_code = j;
                c.f_expected_pos.set_filename("unknown-file");
                c.f_expected_pos.set_function("unknown-func");

                for(as2js::message_level_t k(as2js::message_level_t::MESSAGE_LEVEL_OFF); k <= as2js::message_level_t::MESSAGE_LEVEL_TRACE; k = static_cast<as2js::message_level_t>(static_cast<int>(k) + 1))
                {
                    as2js::Message::set_message_level(k);
                    as2js::message_level_t min(k < as2js::message_level_t::MESSAGE_LEVEL_ERROR ? as2js::message_level_t::MESSAGE_LEVEL_ERROR : k);
//std::cerr << "i == " << static_cast<int32_t>(i) << ", k == " << static_cast<int32_t>(k) << ", min == " << static_cast<int32_t>(min) << " expect = " << c.f_expected_call << "\n";
                    {
                        c.f_expected_call = false;
                        c.f_got_called = false;
                        c.f_expected_message = "";
                        as2js::Message msg(i, j);
                    }
                    CPPUNIT_ASSERT(!c.f_got_called); // no message no call
                    {
                        c.f_expected_call = i != as2js::message_level_t::MESSAGE_LEVEL_OFF && i <= min;
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

                        for(as2js::message_level_t k(as2js::message_level_t::MESSAGE_LEVEL_OFF); k <= as2js::message_level_t::MESSAGE_LEVEL_TRACE; k = static_cast<as2js::message_level_t>(static_cast<int>(k) + 1))
                        {
                            as2js::Message::set_message_level(k);
                            as2js::message_level_t min(k < as2js::message_level_t::MESSAGE_LEVEL_ERROR ? as2js::message_level_t::MESSAGE_LEVEL_ERROR : k);
                            {
                                c.f_expected_call = false;
                                c.f_got_called = false;
                                c.f_expected_message = "";
                                as2js::Message msg(i, j, pos);
                            }
                            CPPUNIT_ASSERT(!c.f_got_called);
                            {
                                c.f_expected_call = i != as2js::message_level_t::MESSAGE_LEVEL_OFF && i <= min;
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


void As2JsMessageUnitTests::test_operator()
{
    test_callback c;
    c.f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
    c.f_expected_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
    c.f_expected_pos.set_filename("operator.js");
    c.f_expected_pos.set_function("compute");
    as2js::Message::set_message_level(as2js::message_level_t::MESSAGE_LEVEL_INFO);

    // test the copy constructor and operator
    {
        test_callback try_copy(c);
        test_callback try_assignment;
        try_assignment = c;
    }
    // this is required as the destructors called on the previous '}'
    // will otherwise clear that pointer...
    as2js::Message::set_message_callback(&c);

    as2js::Position pos;
    pos.set_filename("operator.js");
    pos.set_function("compute");
    c.f_expected_pos = pos;

    // test with nothing
    {
        c.f_expected_call = false;
        c.f_got_called = false;
        c.f_expected_message = "";
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
    }
    CPPUNIT_ASSERT(!c.f_got_called); // no message no call

    // test with char *
    {
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = "with a message";
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        msg << "with a message";
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with std::string
    {
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = "with an std::string message";
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        std::string str("with an std::string message");
        msg << str;
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with ASCII wchar_t
    {
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = "Simple wide char string";
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        wchar_t const *str(L"Simple wide char string");
        msg << str;
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with Unicode wchar_t
    {
        wchar_t const *str(L"Some: \x2028 Unicode \xA9");
        as2js::String unicode(str);
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = unicode.to_utf8();
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        msg << str;
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with ASCII std::wstring
    {
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = "with an std::string message";
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        std::wstring str(L"with an std::string message");
        msg << str;
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with Unicode std::wstring
    {
        std::wstring str(L"Some: \x2028 Unicode \xA9");
        as2js::String unicode(str);
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = unicode.to_utf8();
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        msg << str;
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with as2js::String too
    {
        std::wstring str(L"Some: \x2028 Unicode \xA9");
        as2js::String unicode(str);
        c.f_expected_call = true;
        c.f_got_called = false;
        c.f_expected_message = unicode.to_utf8();
        as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
        msg << unicode;
    }
    CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);

    // test with char
    for(int idx(1); idx <= 255; ++idx)
    {
        char ci(static_cast<char>(idx));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with signed char
    for(int idx(-128); idx <= 127; ++idx)
    {
        signed char ci(static_cast<signed char>(idx));
        {
            std::stringstream str;
            str << static_cast<int>(ci);
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with unsigned char
    for(int idx(-128); idx <= 127; ++idx)
    {
        unsigned char ci(static_cast<unsigned char>(idx));
        {
            std::stringstream str;
            str << static_cast<int>(ci);
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with signed short
    for(int idx(0); idx < 256; ++idx)
    {
        signed short ci(static_cast<signed short>(rand()));
        {
            std::stringstream str;
            str << static_cast<int>(ci);
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with unsigned short
    for(int idx(0); idx < 256; ++idx)
    {
        unsigned short ci(static_cast<unsigned short>(rand()));
        {
            std::stringstream str;
            str << static_cast<int>(ci);
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with signed int
    for(int idx(0); idx < 256; ++idx)
    {
        signed int ci((static_cast<unsigned int>(rand()) << 16) ^ static_cast<unsigned int>(rand()));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with unsigned int
    for(int idx(0); idx < 256; ++idx)
    {
        unsigned int ci((static_cast<unsigned int>(rand()) << 16) ^ static_cast<unsigned int>(rand()));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with signed long
    for(int idx(0); idx < 256; ++idx)
    {
        signed long ci(
#if (ULONG_MAX) != 0xfffffffful
                  (static_cast<unsigned long>(rand()) << 48)
                ^ (static_cast<unsigned long>(rand()) << 32)
                ^
#endif
                  (static_cast<unsigned long>(rand()) << 16)
                ^ (static_cast<unsigned long>(rand()) <<  0));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with unsigned long
    for(int idx(0); idx < 256; ++idx)
    {
        unsigned long ci(
#if (ULONG_MAX) != 0xfffffffful
                  (static_cast<unsigned long>(rand()) << 48)
                ^ (static_cast<unsigned long>(rand()) << 32)
                ^
#endif
                  (static_cast<unsigned long>(rand()) << 16)
                ^ (static_cast<unsigned long>(rand())));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // if not 64 bits, then the next 2 tests should probably change a bit
    // to support the additional bits
    CPPUNIT_ASSERT(sizeof(unsigned long long) == 64 / 8);

    // test with signed long long
    for(int idx(0); idx < 256; ++idx)
    {
        signed long long ci(
                  (static_cast<unsigned long long>(rand()) << 48)
                ^ (static_cast<unsigned long long>(rand()) << 32)
                ^ (static_cast<unsigned long long>(rand()) << 16)
                ^ (static_cast<unsigned long long>(rand()) <<  0));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with unsigned long long
    for(int idx(0); idx < 256; ++idx)
    {
        unsigned long long ci(
                  (static_cast<unsigned long long>(rand()) << 48)
                ^ (static_cast<unsigned long long>(rand()) << 32)
                ^ (static_cast<unsigned long long>(rand()) << 16)
                ^ (static_cast<unsigned long long>(rand())));
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with Int64
    for(int idx(0); idx < 256; ++idx)
    {
        as2js::Int64::int64_type ci(
                  (static_cast<as2js::Int64::int64_type>(rand()) << 48)
                ^ (static_cast<as2js::Int64::int64_type>(rand()) << 32)
                ^ (static_cast<as2js::Int64::int64_type>(rand()) << 16)
                ^ (static_cast<as2js::Int64::int64_type>(rand()) <<  0));
        as2js::Int64 value(ci);
        {
            std::stringstream str;
            str << ci;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << value;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with float
    for(int idx(0); idx < 256; ++idx)
    {
        float s1(rand() & 1 ? -1 : 1);
        float n1(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float d1(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float r(n1 / d1 * s1);
        {
            std::stringstream str;
            str << r;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << r;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with double
    for(int idx(0); idx < 256; ++idx)
    {
        double s1(rand() & 1 ? -1 : 1);
        double n1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        double d1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        double r(n1 / d1 * s1);
        {
            std::stringstream str;
            str << r;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << r;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with Float64
    for(int idx(0); idx < 256; ++idx)
    {
        double s1(rand() & 1 ? -1 : 1);
        double n1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        double d1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        double r(n1 / d1 * s1);
        as2js::Float64 f(r);
        {
            std::stringstream str;
            str << r;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << f;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with bool
    for(int idx(0); idx <= 255; ++idx)
    {
        bool ci(static_cast<char>(idx));
        {
            std::stringstream str;
            str << static_cast<int>(ci);
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ci;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }

    // test with pointers
    for(int idx(0); idx <= 255; ++idx)
    {
        int *ptr(new int[5]);
        {
            std::stringstream str;
            str << ptr;
            c.f_expected_call = true;
            c.f_got_called = false;
            c.f_expected_message = str.str();
            as2js::Message msg(as2js::message_level_t::MESSAGE_LEVEL_ERROR, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, pos);
            msg << ptr;
        }
        CPPUNIT_ASSERT(c.f_expected_call == c.f_got_called);
    }
}


// vim: ts=4 sw=4 et
