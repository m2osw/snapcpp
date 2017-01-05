/* test_as2js_json.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_json.h"
#include    "test_as2js_main.h"

#include    "as2js/json.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"

// See http://icu-project.org/apiref/icu4c/index.html
#include <unicode/uchar.h>
//#include <unicode/cuchar> // once available in Linux...

#include    <unistd.h>

#include    <limits>
#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsJSONUnitTests );

namespace
{


int32_t generate_string(as2js::String& str, as2js::String& stringified)
{
    stringified += '"';
    as2js::as_char_t c;
    int32_t used(0);
    int ctrl(rand() % 7);
    int const max_chars(rand() % 25 + 5);
    for(int j(0); j < max_chars; ++j)
    {
        do
        {
            c = rand() & 0x1FFFFF;
            if(ctrl == 0)
            {
                ctrl = rand() % 7;
                if((ctrl & 3) == 1)
                {
                    c = c & 1 ? '"' : '\'';
                }
                else
                {
                    c &= 0x1F;
                }
            }
            else
            {
                --ctrl;
            }
        }
        while(c >= 0x110000
           || (c >= 0xD800 && c <= 0xDFFF)
           || ((c & 0xFFFE) == 0xFFFE)
           || c == '\0');
        str += c;
        switch(c)
        {
        case '\b':
            stringified += '\\';
            stringified += 'b';
            used |= 0x01;
            break;

        case '\f':
            stringified += '\\';
            stringified += 'f';
            used |= 0x02;
            break;

        case '\n':
            stringified += '\\';
            stringified += 'n';
            used |= 0x04;
            break;

        case '\r':
            stringified += '\\';
            stringified += 'r';
            used |= 0x08;
            break;

        case '\t':
            stringified += '\\';
            stringified += 't';
            used |= 0x10;
            break;

        case '"':
            stringified += '\\';
            stringified += '"';
            used |= 0x20;
            break;

        case '\'':
            // JSON does not expect the apostrophe (') to be escaped
            //stringified += '\\';
            stringified += '\'';
            used |= 0x40;
            break;

        default:
            if(c < 0x0020)
            {
                // other controls must be escaped using Unicode
                std::stringstream ss;
                ss << std::hex << "\\u" << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                stringified += ss.str().c_str();
                used |= 0x80;
            }
            else
            {
                stringified += c;
            }
            break;

        }
    }
    stringified += '"';

    return used;
}


void stringify_string(as2js::String const& str, as2js::String& stringified)
{
    stringified += '"';
    size_t const max_chars(str.length());
    for(size_t j(0); j < max_chars; ++j)
    {
        as2js::as_char_t c(str[j]);
        switch(c)
        {
        case '\b':
            stringified += '\\';
            stringified += 'b';
            break;

        case '\f':
            stringified += '\\';
            stringified += 'f';
            break;

        case '\n':
            stringified += '\\';
            stringified += 'n';
            break;

        case '\r':
            stringified += '\\';
            stringified += 'r';
            break;

        case '\t':
            stringified += '\\';
            stringified += 't';
            break;

        case '"':
            stringified += '\\';
            stringified += '"';
            break;

        case '\'':
            // JSON does not escape apostrophes (')
            //stringified += '\\';
            stringified += '\'';
            break;

        default:
            if(c < 0x0020)
            {
                // other controls must be escaped using Unicode
                std::stringstream ss;
                ss << std::hex << "\\u" << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                stringified += ss.str().c_str();
            }
            else
            {
                stringified += c;
            }
            break;

        }
    }
    stringified += '"';
}


struct test_data_t
{
    as2js::Position                     f_pos;
    as2js::JSON::JSONValue::pointer_t   f_value;
    uint32_t                            f_count = 0;
};


int const TYPE_NULL         = 0x00000001;
int const TYPE_INT64        = 0x00000002;
int const TYPE_FLOAT64      = 0x00000004;
int const TYPE_NAN          = 0x00000008;
int const TYPE_PINFINITY    = 0x00000010;
int const TYPE_MINFINITY    = 0x00000020;
int const TYPE_TRUE         = 0x00000040;
int const TYPE_FALSE        = 0x00000080;
int const TYPE_STRING       = 0x00000100;
int const TYPE_ARRAY        = 0x00000200;
int const TYPE_OBJECT       = 0x00000400;

int const TYPE_ALL          = 0x000007FF;

int g_type_used;


void create_item(test_data_t& data, as2js::JSON::JSONValue::pointer_t parent, int depth)
{
    size_t const max_items(rand() % 8 + 2);
    for(size_t j(0); j < max_items; ++j)
    {
        ++data.f_count;
        as2js::JSON::JSONValue::pointer_t item;
        int const select(rand() % 8);
        switch(select)
        {
        case 0: // NULL
            g_type_used |= TYPE_NULL;
            item.reset(new as2js::JSON::JSONValue(data.f_pos));
            break;

        case 1: // INT64
            g_type_used |= TYPE_INT64;
            {
                as2js::Int64::int64_type int_value((rand() << 13) ^ rand());
                as2js::Int64 integer(int_value);
                item.reset(new as2js::JSON::JSONValue(data.f_pos, integer));
            }
            break;

        case 2: // FLOAT64
            switch(rand() % 10)
            {
            case 0:
                g_type_used |= TYPE_NAN;
                {
                    as2js::Float64 flt;
                    flt.set_NaN();
                    item.reset(new as2js::JSON::JSONValue(data.f_pos, flt));
                }
                break;

            case 1:
                g_type_used |= TYPE_PINFINITY;
                {
                    as2js::Float64 flt;
                    flt.set_infinity();
                    item.reset(new as2js::JSON::JSONValue(data.f_pos, flt));
                }
                break;

            case 2:
                g_type_used |= TYPE_MINFINITY;
                {
                    as2js::Float64::float64_type flt_value(-std::numeric_limits<as2js::Float64::float64_type>::infinity());
                    as2js::Float64 flt(flt_value);
                    item.reset(new as2js::JSON::JSONValue(data.f_pos, flt));
                }
                break;

            default:
                g_type_used |= TYPE_FLOAT64;
                {
                    as2js::Float64::float64_type flt_value(static_cast<double>((rand() << 16) | rand()) / static_cast<double>((rand() << 16) | rand()));
                    as2js::Float64 flt(flt_value);
                    item.reset(new as2js::JSON::JSONValue(data.f_pos, flt));
                }
                break;

            }
            break;

        case 3: // TRUE
            g_type_used |= TYPE_TRUE;
            item.reset(new as2js::JSON::JSONValue(data.f_pos, true));
            break;

        case 4: // FALSE
            g_type_used |= TYPE_FALSE;
            item.reset(new as2js::JSON::JSONValue(data.f_pos, false));
            break;

        case 5: // STRING
            g_type_used |= TYPE_STRING;
            {
                as2js::String str;
                as2js::String stringified;
                generate_string(str, stringified);
                item.reset(new as2js::JSON::JSONValue(data.f_pos, str));
            }
            break;

        case 6: // empty ARRAY
            g_type_used |= TYPE_ARRAY;
            {
                as2js::JSON::JSONValue::array_t empty_array;
                item.reset(new as2js::JSON::JSONValue(data.f_pos, empty_array));
                if(depth < 5 && (rand() & 1) != 0)
                {
                    create_item(data, item, depth + 1);
                }
            }
            break;

        case 7: // empty OBJECT
            g_type_used |= TYPE_OBJECT;
            {
                as2js::JSON::JSONValue::object_t empty_object;
                item.reset(new as2js::JSON::JSONValue(data.f_pos, empty_object));
                if(depth < 5 && (rand() & 1) != 0)
                {
                    create_item(data, item, depth + 1);
                }
            }
            break;

        // more?
        default:
            throw std::logic_error("test generated an invalid # to generate an object item");

        }
        if(parent->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY)
        {
            parent->set_item(parent->get_array().size(), item);
        }
        else
        {
            as2js::String field_name;
            as2js::String stringified_value;
            generate_string(field_name, stringified_value);
            parent->set_member(field_name, item);
        }
    }
}


void create_array(test_data_t& data)
{
    as2js::JSON::JSONValue::array_t array;
    data.f_value.reset(new as2js::JSON::JSONValue(data.f_pos, array));
    create_item(data, data.f_value, 0);
}


void create_object(test_data_t& data)
{
    as2js::JSON::JSONValue::object_t object;
    data.f_value.reset(new as2js::JSON::JSONValue(data.f_pos, object));
    create_item(data, data.f_value, 0);
}


void data_to_string(as2js::JSON::JSONValue::pointer_t value, as2js::String& expected)
{
    switch(value->get_type())
    {
    case as2js::JSON::JSONValue::type_t::JSON_TYPE_NULL:
        expected += "null";
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_TRUE:
        expected += "true";
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_FALSE:
        expected += "false";
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_INT64:
        expected += std::to_string(value->get_int64().get());
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64:
        if(value->get_float64().is_NaN())
        {
            expected += "NaN";
        }
        else if(value->get_float64().is_positive_infinity())
        {
            expected += "Infinity";
        }
        else if(value->get_float64().is_negative_infinity())
        {
            expected += "-Infinity";
        }
        else
        {
            expected += std::to_string(value->get_float64().get());
        }
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING:
        stringify_string(value->get_string(), expected);
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY:
        expected += '[';
        {
            bool first(true);
            for(auto it : value->get_array())
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    expected += ',';
                }
                data_to_string(it, expected); // recursive
            }
        }
        expected += ']';
        break;

    case as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT:
        expected += '{';
        {
            bool first(true);
            for(auto it : value->get_object())
            {
                if(first)
                {
                    first = false;
                }
                else
                {
                    expected += ',';
                }
                stringify_string(it.first, expected);
                expected += ':';
                data_to_string(it.second, expected); // recursive
            }
        }
        expected += '}';
        break;

    // more?
    default:
        throw std::logic_error("test found an invalid JSONValue::type_t to stringify a value item");

    }
}


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
        CPPUNIT_ASSERT(!f_expected.empty());

//std::cerr << "filename = " << pos.get_filename() << " / " << f_expected[0].f_pos.get_filename() << "\n";
//std::cerr << "msg = " << message << " / " << f_expected[0].f_message << "\n";
//std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
//std::cerr << "error_code = " << static_cast<int>(error_code) << " / " << static_cast<int>(f_expected[0].f_error_code) << "\n";

        CPPUNIT_ASSERT(f_expected[0].f_call);
        CPPUNIT_ASSERT(message_level == f_expected[0].f_message_level);
        CPPUNIT_ASSERT(error_code == f_expected[0].f_error_code);
        CPPUNIT_ASSERT(pos.get_filename() == f_expected[0].f_pos.get_filename());
        CPPUNIT_ASSERT(pos.get_function() == f_expected[0].f_pos.get_function());
        CPPUNIT_ASSERT(pos.get_page() == f_expected[0].f_pos.get_page());
        CPPUNIT_ASSERT(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
        CPPUNIT_ASSERT(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
        CPPUNIT_ASSERT(pos.get_line() == f_expected[0].f_pos.get_line());
        CPPUNIT_ASSERT(message == f_expected[0].f_message);

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

        f_expected.erase(f_expected.begin());
    }

    void got_called()
    {
        if(!f_expected.empty())
        {
            std::cerr << "\n*** STILL EXPECTED: ***\n";
            std::cerr << "filename = " << f_expected[0].f_pos.get_filename() << "\n";
            std::cerr << "msg = " << f_expected[0].f_message << "\n";
            std::cerr << "page = " << f_expected[0].f_pos.get_page() << "\n";
            std::cerr << "error_code = " << static_cast<int>(f_expected[0].f_error_code) << "\n";
        }
        CPPUNIT_ASSERT(f_expected.empty());
    }

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::Position             f_pos;
        std::string                 f_message; // UTF-8 string
    };

    std::vector<expected_t>     f_expected;

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;


bool is_identifier_char(int32_t const c)
{
    // special cases in JavaScript identifiers
    if(c == 0x200C    // ZWNJ
    || c == 0x200D)   // ZWJ
    {
        return true;
    }

    switch(u_charType(static_cast<UChar32>(c)))
    {
    case U_UPPERCASE_LETTER:
    case U_LOWERCASE_LETTER:
    case U_TITLECASE_LETTER:
    case U_MODIFIER_LETTER:
    case U_OTHER_LETTER:
    case U_LETTER_NUMBER:
    case U_NON_SPACING_MARK:
    case U_COMBINING_SPACING_MARK:
    case U_DECIMAL_DIGIT_NUMBER:
    case U_CONNECTOR_PUNCTUATION:
        return true;

    default:
        return false;

    }
}


}
// no name namespace




void As2JsJSONUnitTests::test_basic_values()
{
    // a null pointer value...
    as2js::JSON::JSONValue::pointer_t const nullptr_value;

    // NULL value
    {
        as2js::Position pos;
        pos.reset_counters(33);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_NULL);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 33);
        CPPUNIT_ASSERT(value->to_string() == "null");
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_NULL);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 33);
        CPPUNIT_ASSERT(copy.to_string() == "null");
    }

    // TRUE value
    {
        as2js::Position pos;
        pos.reset_counters(35);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, true));
        // modify out pos object to make sure that the one in value is not a reference
        pos.set_filename("verify.json");
        pos.set_function("bad_objects");
        pos.new_line();
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_TRUE);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == "data.json");
        CPPUNIT_ASSERT(p.get_function() == "save_objects");
        CPPUNIT_ASSERT(p.get_line() == 35);
        CPPUNIT_ASSERT(value->to_string() == "true");
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_TRUE);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == "data.json");
        CPPUNIT_ASSERT(q.get_function() == "save_objects");
        CPPUNIT_ASSERT(q.get_line() == 35);
        CPPUNIT_ASSERT(copy.to_string() == "true");
    }

    // FALSE value
    {
        as2js::Position pos;
        pos.reset_counters(53);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, false));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FALSE);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 53);
        CPPUNIT_ASSERT(value->to_string() == "false");
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FALSE);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 53);
        CPPUNIT_ASSERT(copy.to_string() == "false");
    }

    // INT64 value
    for(int idx(0); idx < 100; ++idx)
    {
        as2js::Position pos;
        pos.reset_counters(103);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::Int64::int64_type int_value((rand() << 14) ^ rand());
        as2js::Int64 integer(int_value);
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, integer));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_INT64);
        CPPUNIT_ASSERT(value->get_int64().get() == int_value);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 103);
        std::stringstream ss;
        ss << integer.get();
        as2js::String cmp;
        cmp.from_utf8(ss.str().c_str());
        CPPUNIT_ASSERT(value->to_string() == cmp);
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_INT64);
        CPPUNIT_ASSERT(copy.get_int64().get() == int_value);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 103);
        CPPUNIT_ASSERT(copy.to_string() == cmp);
    }

    // FLOAT64 value
    {
        as2js::Position pos;
        pos.reset_counters(144);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::Float64::float64_type flt_value(std::numeric_limits<as2js::Float64::float64_type>::quiet_NaN());
        as2js::Float64 flt(flt_value);
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, flt));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // NaN's do not compare equal
        CPPUNIT_ASSERT(value->get_float64().get() != flt_value);
#pragma GCC diagnostic pop
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 144);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CPPUNIT_ASSERT(value->to_string() == "NaN");
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // NaN's do not compare equal
        CPPUNIT_ASSERT(copy.get_float64().get() != flt_value);
#pragma GCC diagnostic pop
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 144);
        CPPUNIT_ASSERT(copy.to_string() == "NaN");
    }

    for(int idx(0); idx < 100; ++idx)
    {
        as2js::Position pos;
        pos.reset_counters(44);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::Float64::float64_type flt_value(static_cast<double>(rand()) / static_cast<double>(rand()));
        as2js::Float64 flt(flt_value);
        as2js::String cmp;
        std::string ss(std::to_string(flt_value));
        cmp.from_utf8(ss.c_str());
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, flt));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(value->get_float64().get() == flt_value);
#pragma GCC diagnostic pop
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 44);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CPPUNIT_ASSERT(value->to_string() == cmp);
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(copy.get_float64().get() == flt_value);
#pragma GCC diagnostic pop
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 44);
        CPPUNIT_ASSERT(copy.to_string() == cmp);
    }

    // STRING value
    for(size_t idx(0), used(0); idx < 100 || used != 0xFF; ++idx)
    {
        as2js::Position pos;
        pos.reset_counters(89);
        pos.set_filename("data.json");
        pos.set_function("save_objects");
        as2js::String str;
        as2js::String stringified;
        used |= generate_string(str, stringified);
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, str));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT(value->get_string() == str);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 89);
#if 0
as2js::String r(value->to_string());
std::cerr << std::hex << " lengths " << r.length() << " / " << stringified.length() << "\n";
size_t max_chrs(std::min(r.length(), stringified.length()));
for(size_t g(0); g < max_chrs; ++g)
{
    if(static_cast<int>(r[g]) != static_cast<int>(stringified[g]))
    {
        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(stringified[g]) << "\n";
    }
    else
    {
        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(stringified[g]) << "\n";
    }
}
if(r.length() > stringified.length())
{
    for(size_t g(stringified.length()); g < r.length(); ++g)
    {
        std::cerr << " *** " << static_cast<int>(r[g]) << "\n";
    }
}
else
{
    for(size_t g(r.length()); g < stringified.length(); ++g)
    {
        std::cerr << " +++ " << static_cast<int>(stringified[g]) << "\n";
    }
}
std::cerr << std::dec;
#endif
        CPPUNIT_ASSERT(value->to_string() == stringified);
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_STRING);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT(copy.get_string() == str);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(rand(), nullptr_value), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 89);
        CPPUNIT_ASSERT(copy.to_string() == stringified);
    }
}


void As2JsJSONUnitTests::test_array_value()
{
    // a null pointer value...
    as2js::JSON::JSONValue::pointer_t const nullptr_value;

    // test with an empty array
    {
        as2js::Position pos;
        pos.reset_counters(109);
        pos.set_filename("array.json");
        pos.set_function("save_array");
        as2js::JSON::JSONValue::array_t initial;
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, initial));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        as2js::JSON::JSONValue::array_t const& array(value->get_array());
        CPPUNIT_ASSERT(array.empty());
        for(int idx(-10); idx <= 10; ++idx)
        {
            if(idx == 0)
            {
                // nullptr is not valid for data
                CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::exception_invalid_data);
            }
            else
            {
                // index is invalid
                CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::exception_index_out_of_range);
            }
        }
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 109);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CPPUNIT_ASSERT(value->to_string() == "[]");
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        as2js::JSON::JSONValue::array_t const& array_copy(value->get_array());
        CPPUNIT_ASSERT(array_copy.empty());
        for(int idx(-10); idx <= 10; ++idx)
        {
            if(idx == 0)
            {
                // nullptr is not valid for data
                CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::exception_invalid_data);
            }
            else
            {
                // index is invalid
                CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::exception_index_out_of_range);
            }
        }
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 109);
        CPPUNIT_ASSERT(copy.to_string() == "[]");
    }

    // test with a few random arrays
    for(int idx(0); idx < 10; ++idx)
    {
        as2js::Position pos;
        pos.reset_counters(109);
        pos.set_filename("array.json");
        pos.set_function("save_array");
        as2js::JSON::JSONValue::array_t initial;

        as2js::String result("[");
        size_t const max_items(rand() % 100 + 20);
        for(size_t j(0); j < max_items; ++j)
        {
            if(j != 0)
            {
                result += ",";
            }
            as2js::JSON::JSONValue::pointer_t item;
            int const select(rand() % 8);
            switch(select)
            {
            case 0: // NULL
                item.reset(new as2js::JSON::JSONValue(pos));
                result += "null";
                break;

            case 1: // INT64
                {
                    as2js::Int64::int64_type int_value((rand() << 13) ^ rand());
                    as2js::Int64 integer(int_value);
                    item.reset(new as2js::JSON::JSONValue(pos, integer));
                    result += std::to_string(int_value);
                }
                break;

            case 2: // FLOAT64
                {
                    as2js::Float64::float64_type flt_value(static_cast<double>((rand() << 16) | rand()) / static_cast<double>((rand() << 16) | rand()));
                    as2js::Float64 flt(flt_value);
                    item.reset(new as2js::JSON::JSONValue(pos, flt));
                    result += std::to_string(flt_value);
                }
                break;

            case 3: // TRUE
                item.reset(new as2js::JSON::JSONValue(pos, true));
                result += "true";
                break;

            case 4: // FALSE
                item.reset(new as2js::JSON::JSONValue(pos, false));
                result += "false";
                break;

            case 5: // STRING
                {
                    as2js::String str;
                    as2js::String stringified;
                    generate_string(str, stringified);
                    item.reset(new as2js::JSON::JSONValue(pos, str));
                    result += stringified;
                }
                break;

            case 6: // empty ARRAY
                {
                    as2js::JSON::JSONValue::array_t empty_array;
                    item.reset(new as2js::JSON::JSONValue(pos, empty_array));
                    result += "[]";
                }
                break;

            case 7: // empty OBJECT
                {
                    as2js::JSON::JSONValue::object_t empty_object;
                    item.reset(new as2js::JSON::JSONValue(pos, empty_object));
                    result += "{}";
                }
                break;

            // more?
            default:
                throw std::logic_error("test generated an invalid # to generate an array item");

            }
            initial.push_back(item);
        }
        result += "]";

        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, initial));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        as2js::JSON::JSONValue::array_t const& array(value->get_array());
        CPPUNIT_ASSERT(array.size() == max_items);
        //for(int idx(-10); idx <= 10; ++idx)
        //{
        //    if(idx == 0)
        //    {
        //        // nullptr is not valid for data
        //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::exception_invalid_data);
        //    }
        //    else
        //    {
        //        // index is invalid
        //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::exception_index_out_of_range);
        //    }
        //}
        CPPUNIT_ASSERT_THROW(value->get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 109);
//as2js::String r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
        CPPUNIT_ASSERT(value->to_string() == result);
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        as2js::JSON::JSONValue::array_t const& array_copy(value->get_array());
        CPPUNIT_ASSERT(array_copy.size() == max_items);
        //for(int idx(-10); idx <= 10; ++idx)
        //{
        //    if(idx == 0)
        //    {
        //        // nullptr is not valid for data
        //        CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::exception_invalid_data);
        //    }
        //    else
        //    {
        //        // index is invalid
        //        CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::exception_index_out_of_range);
        //    }
        //}
        CPPUNIT_ASSERT_THROW(copy.get_object(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_member("name", nullptr_value), as2js::exception_internal_error);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 109);
        CPPUNIT_ASSERT(copy.to_string() == result);
        // the cyclic flag should have been reset, make sure of that:
        CPPUNIT_ASSERT(copy.to_string() == result);

        // test that we catch a direct 'array[x] = array;'
        value->set_item(max_items, value);
        // copy is not affected...
        CPPUNIT_ASSERT(copy.to_string() == result);
        // value to string fails because it is cyclic
        CPPUNIT_ASSERT_THROW(value->to_string() == result, as2js::exception_cyclical_structure);
        as2js::JSON::JSONValue::array_t const& cyclic_array(value->get_array());
        CPPUNIT_ASSERT(cyclic_array.size() == max_items + 1);

        {
            as2js::String str;
            as2js::String stringified;
            generate_string(str, stringified);
            as2js::JSON::JSONValue::pointer_t item;
            item.reset(new as2js::JSON::JSONValue(pos, str));
            // remove the existing ']' first
            result.erase(result.end() - 1);
            result += ',';
            result += stringified;
            result += ']';
            value->set_item(max_items, item);
//as2js::String r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
            CPPUNIT_ASSERT(value->to_string() == result);
        }
    }
}


void As2JsJSONUnitTests::test_object_value()
{
    // a null pointer value...
    as2js::JSON::JSONValue::pointer_t const nullptr_value;

    // test with an empty array
    {
        as2js::Position pos;
        pos.reset_counters(109);
        pos.set_filename("object.json");
        pos.set_function("save_object");
        as2js::JSON::JSONValue::object_t initial;
        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, initial));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(-1, nullptr_value), as2js::exception_internal_error);
        as2js::JSON::JSONValue::object_t const& object(value->get_object());
        CPPUNIT_ASSERT(object.empty());
        // name is invalid
        CPPUNIT_ASSERT_THROW(value->set_member("", nullptr_value), as2js::exception_invalid_index);
        // nullptr is not valid for data
        CPPUNIT_ASSERT_THROW(value->set_member("ignore", nullptr_value), as2js::exception_invalid_data);
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 109);
//std::cerr << "compare " << value->to_string() << " with " << cmp << "\n";
        CPPUNIT_ASSERT(value->to_string() == "{}");
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(0, nullptr_value), as2js::exception_internal_error);
        as2js::JSON::JSONValue::object_t const& object_copy(value->get_object());
        CPPUNIT_ASSERT(object_copy.empty());
        // name is invalid
        CPPUNIT_ASSERT_THROW(copy.set_member("", nullptr_value), as2js::exception_invalid_index);
        // nullptr is not valid for data
        CPPUNIT_ASSERT_THROW(copy.set_member("ignore", nullptr_value), as2js::exception_invalid_data);
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 109);
        CPPUNIT_ASSERT(copy.to_string() == "{}");
    }

    // test with a few random objects
    typedef std::map<as2js::String, as2js::String>  sort_t;
    for(int idx(0); idx < 10; ++idx)
    {
        as2js::Position pos;
        pos.reset_counters(199);
        pos.set_filename("object.json");
        pos.set_function("save_object");
        as2js::JSON::JSONValue::object_t initial;
        sort_t sorted;

        size_t const max_items(rand() % 100 + 20);
        for(size_t j(0); j < max_items; ++j)
        {
            as2js::String field_name;
            as2js::String stringified_value;
            generate_string(field_name, stringified_value);
            stringified_value += ':';
            as2js::JSON::JSONValue::pointer_t item;
            int const select(rand() % 8);
            switch(select)
            {
            case 0: // NULL
                item.reset(new as2js::JSON::JSONValue(pos));
                stringified_value += "null";
                break;

            case 1: // INT64
                {
                    as2js::Int64::int64_type int_value((rand() << 13) ^ rand());
                    as2js::Int64 integer(int_value);
                    item.reset(new as2js::JSON::JSONValue(pos, integer));
                    stringified_value += std::to_string(int_value);
                }
                break;

            case 2: // FLOAT64
                {
                    as2js::Float64::float64_type flt_value(static_cast<double>((rand() << 16) | rand()) / static_cast<double>((rand() << 16) | rand()));
                    as2js::Float64 flt(flt_value);
                    item.reset(new as2js::JSON::JSONValue(pos, flt));
                    stringified_value += std::to_string(flt_value);
                }
                break;

            case 3: // TRUE
                item.reset(new as2js::JSON::JSONValue(pos, true));
                stringified_value += "true";
                break;

            case 4: // FALSE
                item.reset(new as2js::JSON::JSONValue(pos, false));
                stringified_value += "false";
                break;

            case 5: // STRING
                {
                    as2js::String str;
                    as2js::String stringified;
                    generate_string(str, stringified);
                    item.reset(new as2js::JSON::JSONValue(pos, str));
                    stringified_value += stringified;
                }
                break;

            case 6: // empty ARRAY
                {
                    as2js::JSON::JSONValue::array_t empty_array;
                    item.reset(new as2js::JSON::JSONValue(pos, empty_array));
                    stringified_value += "[]";
                }
                break;

            case 7: // empty OBJECT
                {
                    as2js::JSON::JSONValue::object_t empty_object;
                    item.reset(new as2js::JSON::JSONValue(pos, empty_object));
                    stringified_value += "{}";
                }
                break;

            // more?
            default:
                throw std::logic_error("test generated an invalid # to generate an object item");

            }
            initial[field_name] = item;
            sorted[field_name] = stringified_value;
        }
        as2js::String result("{");
        bool first(true);
        for(auto it : sorted)
        {
            if(!first)
            {
                result += ',';
            }
            else
            {
                first = false;
            }
            result += it.second;
        }
        result += "}";

        as2js::JSON::JSONValue::pointer_t value(new as2js::JSON::JSONValue(pos, initial));
        CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
        CPPUNIT_ASSERT_THROW(value->get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(value->set_item(0, nullptr_value), as2js::exception_internal_error);
        as2js::JSON::JSONValue::object_t const& object(value->get_object());
        CPPUNIT_ASSERT(object.size() == max_items);
        //for(int idx(-10); idx <= 10; ++idx)
        //{
        //    if(idx == 0)
        //    {
        //        // nullptr is not valid for data
        //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::exception_invalid_data);
        //    }
        //    else
        //    {
        //        // index is invalid
        //        CPPUNIT_ASSERT_THROW(value->set_item(idx, nullptr_value), as2js::exception_index_out_of_range);
        //    }
        //}
        as2js::Position const& p(value->get_position());
        CPPUNIT_ASSERT(p.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(p.get_function() == pos.get_function());
        CPPUNIT_ASSERT(p.get_line() == 199);
//as2js::String r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//    for(size_t g(result.length()); g < r.length(); ++g)
//    {
//        std::cerr << " *** " << static_cast<int>(r[g]) << "\n";
//    }
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
        CPPUNIT_ASSERT(value->to_string() == result);
        // copy operator
        as2js::JSON::JSONValue copy(*value);
        CPPUNIT_ASSERT(copy.get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
        CPPUNIT_ASSERT_THROW(copy.get_int64().get() == 0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_float64().get() >= 0.0, as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_string() == "ignore", as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.get_array(), as2js::exception_internal_error);
        CPPUNIT_ASSERT_THROW(copy.set_item(0, nullptr_value), as2js::exception_internal_error);
        as2js::JSON::JSONValue::object_t const& object_copy(value->get_object());
        CPPUNIT_ASSERT(object_copy.size() == max_items);
        //for(int idx(-10); idx <= 10; ++idx)
        //{
        //    if(idx == 0)
        //    {
        //        // nullptr is not valid for data
        //        CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::exception_invalid_data);
        //    }
        //    else
        //    {
        //        // index is invalid
        //        CPPUNIT_ASSERT_THROW(copy.set_item(idx, nullptr_value), as2js::exception_index_out_of_range);
        //    }
        //}
        as2js::Position const& q(copy.get_position());
        CPPUNIT_ASSERT(q.get_filename() == pos.get_filename());
        CPPUNIT_ASSERT(q.get_function() == pos.get_function());
        CPPUNIT_ASSERT(q.get_line() == 199);
        CPPUNIT_ASSERT(copy.to_string() == result);
        // the cyclic flag should have been reset, make sure of that:
        CPPUNIT_ASSERT(copy.to_string() == result);

        // test that we catch a direct 'object[x] = object;'
        value->set_member("random", value);
        // copy is not affected...
        CPPUNIT_ASSERT(copy.to_string() == result);
        // value to string fails because it is cyclic
        CPPUNIT_ASSERT_THROW(value->to_string() == result, as2js::exception_cyclical_structure);
        as2js::JSON::JSONValue::object_t const& cyclic_object(value->get_object());
        CPPUNIT_ASSERT(cyclic_object.size() == max_items + 1);

        {
            as2js::String str;
            as2js::String stringified("\"random\":");
            generate_string(str, stringified);
            as2js::JSON::JSONValue::pointer_t item;
            item.reset(new as2js::JSON::JSONValue(pos, str));
            sorted["random"] = stringified;
            // with objects the entire result needs to be rebuilt
            result = "{";
            first = true;
            for(auto it : sorted)
            {
                if(!first)
                {
                    result += ',';
                }
                else
                {
                    first = false;
                }
                result += it.second;
            }
            result += "}";
            value->set_member("random", item);
//as2js::String r(value->to_string());
//std::cerr << std::hex << " lengths " << r.length() << " / " << result.length() << "\n";
//size_t max_chrs(std::min(r.length(), result.length()));
//for(size_t g(0); g < max_chrs; ++g)
//{
//    if(static_cast<int>(r[g]) != static_cast<int>(result[g]))
//    {
//        std::cerr << " --- " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//    else
//    {
//        std::cerr << " " << static_cast<int>(r[g]) << " / " << static_cast<int>(result[g]) << "\n";
//    }
//}
//if(r.length() > result.length())
//{
//    for(size_t g(result.length()); g < r.length(); ++g)
//    {
//        std::cerr << " *** " << static_cast<int>(r[g]) << "\n";
//    }
//}
//else
//{
//    for(size_t g(r.length()); g < result.length(); ++g)
//    {
//        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
//    }
//}
//std::cerr << std::dec;
            CPPUNIT_ASSERT(value->to_string() == result);
        }
    }
}


void As2JsJSONUnitTests::test_json()
{
    // test with a few random objects
    g_type_used = 0;
    typedef std::map<as2js::String, as2js::String>  sort_t;
    for(int idx(0); idx < 10 || g_type_used != TYPE_ALL; ++idx)
    {
        as2js::String const header(rand() & 1 ? "// we can have a C++ comment\n/* or even a C like comment in the header\n(not the rest because we do not have access...) */\n" : "");

        test_data_t data;
        data.f_pos.reset_counters(199);
        data.f_pos.set_filename("full.json");
        data.f_pos.set_function("save_full");

        if(rand() & 1)
        {
            create_object(data);
        }
        else
        {
            create_array(data);
        }
        as2js::String expected;
        expected += 0xFEFF; // BOM
        expected += header;
        if(!header.empty())
        {
            expected += '\n';
        }
        data_to_string(data.f_value, expected);
//std::cerr << "created " << data.f_count << " items.\n";

        as2js::JSON::pointer_t json(new as2js::JSON);
        json->set_value(data.f_value);

        as2js::StringOutput::pointer_t out(new as2js::StringOutput);
        json->output(out, header);
        as2js::String const& result(out->get_string());
#if 0
{
std::cerr << std::hex << " lengths " << expected.length() << " / " << result.length() << "\n";
size_t max_chrs(std::min(expected.length(), result.length()));
for(size_t g(0); g < max_chrs; ++g)
{
    if(static_cast<int>(expected[g]) != static_cast<int>(result[g]))
    {
        std::cerr << " --- " << static_cast<int>(expected[g]) << " / " << static_cast<int>(result[g]) << "\n";
    }
    else
    {
        std::cerr << " " << static_cast<int>(expected[g]) << " / " << static_cast<int>(result[g]) << "\n";
    }
}
if(expected.length() > result.length())
{
    for(size_t g(result.length()); g < expected.length(); ++g)
    {
        std::cerr << " *** " << static_cast<int>(expected[g]) << "\n";
    }
}
else
{
    for(size_t g(expected.length()); g < result.length(); ++g)
    {
        std::cerr << " +++ " << static_cast<int>(result[g]) << "\n";
    }
}
std::cerr << std::dec;
}
#endif
        CPPUNIT_ASSERT(result == expected);

        CPPUNIT_ASSERT(json->get_value() == data.f_value);
        // make sure the tree is also correct:
        as2js::String expected_tree;
        expected_tree += 0xFEFF; // BOM
        expected_tree += header;
        if(!header.empty())
        {
            expected_tree += '\n';
        }
        data_to_string(json->get_value(), expected_tree);
        CPPUNIT_ASSERT(expected_tree == expected);

        // copy operator
        as2js::JSON copy(*json);

        // the copy gets the exact same value pointer...
        CPPUNIT_ASSERT(copy.get_value() == data.f_value);
        // make sure the tree is also correct:
        as2js::String expected_copy;
        expected_copy += 0xFEFF; // BOM
        expected_copy += header;
        if(!header.empty())
        {
            expected_copy += '\n';
        }
        data_to_string(copy.get_value(), expected_copy);
        CPPUNIT_ASSERT(expected_copy == expected);

        // create an unsafe temporary file and save that JSON in there...
        int number(rand() % 1000000);
        std::stringstream ss;
        ss << "/tmp/as2js_test" << std::setfill('0') << std::setw(6) << number << ".js";
//std::cerr << "filename [" << ss.str() << "]\n";
        std::string filename(ss.str());
        json->save(filename, header);

        as2js::JSON::pointer_t load_json(new as2js::JSON);
        as2js::JSON::JSONValue::pointer_t loaded_value(load_json->load(filename));
        CPPUNIT_ASSERT(loaded_value == load_json->get_value());

        as2js::StringOutput::pointer_t lout(new as2js::StringOutput);
        load_json->output(lout, header);
        as2js::String const& lresult(lout->get_string());
        CPPUNIT_ASSERT(lresult == expected);

        unlink(filename.c_str());
    }
}


void As2JsJSONUnitTests::test_json_with_positive_numbers()
{
    as2js::String const content(
            "// we can have a C++ comment\n"
            "/* or even a C like comment in the header\n"
            "(not the rest because we do not have access...) */\n"
            "[\n"
            "\t+111,\n"
            "\t+1.113,\n"
            "\t+Infinity,\n"
            "\t+NaN\n"
            "]\n"
        );

    test_data_t data;
    data.f_pos.reset_counters(201);
    data.f_pos.set_filename("full.json");
    data.f_pos.set_function("save_full");

    as2js::StringInput::pointer_t in(new as2js::StringInput(content));

    as2js::JSON::pointer_t load_json(new as2js::JSON);
    as2js::JSON::JSONValue::pointer_t loaded_value(load_json->parse(in));
    CPPUNIT_ASSERT(loaded_value == load_json->get_value());

    as2js::JSON::JSONValue::pointer_t value(load_json->get_value());
    CPPUNIT_ASSERT(value->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);
    as2js::JSON::JSONValue::array_t array(value->get_array());
    CPPUNIT_ASSERT(array.size() == 4);

    CPPUNIT_ASSERT(array[0]->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_INT64);
    as2js::Int64 integer(array[0]->get_int64());
    CPPUNIT_ASSERT(integer.get() == 111);

    CPPUNIT_ASSERT(array[1]->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
    as2js::Float64 floating_point(array[1]->get_float64());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    CPPUNIT_ASSERT(floating_point.get() == 1.113);
#pragma GCC diagnostic pop

    CPPUNIT_ASSERT(array[2]->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
    floating_point = array[2]->get_float64();
    CPPUNIT_ASSERT(floating_point.is_positive_infinity());

    CPPUNIT_ASSERT(array[3]->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_FLOAT64);
    floating_point = array[3]->get_float64();
    CPPUNIT_ASSERT(floating_point.is_NaN());
}


void As2JsJSONUnitTests::test_error()
{
    {
        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected.f_error_code = as2js::err_code_t::AS_ERR_NOT_FOUND;
        expected.f_pos.set_filename("/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "cannot open JSON file \"/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately\".";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::JSON::pointer_t load_json(new as2js::JSON);
        CPPUNIT_ASSERT(load_json->load("/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately") == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        test_callback::expected_t expected;
        expected.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected.f_pos.set_filename("unknown-file");
        expected.f_pos.set_function("unknown-func");
        expected.f_message = "could not open output file \"/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately\".";

        test_callback tc;
        tc.f_expected.push_back(expected);

        as2js::JSON::pointer_t save_json(new as2js::JSON);
        CPPUNIT_ASSERT(save_json->save("/this/file/definitively/does/not/exist/so/we'll/get/an/error/immediately", "// unused\n") == false);
        tc.got_called();
    }

    {
        as2js::JSON::pointer_t json(new as2js::JSON);
        as2js::StringOutput::pointer_t lout(new as2js::StringOutput);
        as2js::String const header("// unused\n");
        CPPUNIT_ASSERT_THROW(json->output(lout, header), as2js::exception_invalid_data);
    }

    {
        // use an unsafe temporary file...
        int number(rand() % 1000000);
        std::stringstream ss;
        ss << "/tmp/as2js_test" << std::setfill('0') << std::setw(6) << number << ".js";
        std::string filename(ss.str());
        // create an empty file
        FILE *f(fopen(filename.c_str(), "w"));
        CPPUNIT_ASSERT(f != NULL);
        fclose(f);

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_EOF;
        expected1.f_pos.set_filename(filename);
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "the end of the file was reached while reading JSON data.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename(filename);
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"" + filename + "\".";
        tc.f_expected.push_back(expected2);

//std::cerr << "filename [" << ss.str() << "]\n";
        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->load(filename) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,,'valid too':123}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,invalid:123}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,123:'invalid'}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,['invalid']}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,{'invalid':123}}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_STRING_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a string as the JSON object member name.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,'colon missing'123}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COLON_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a colon (:) as the JSON object member name and member value separator.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            // we use 'valid' twice but one is in a sub-object to test
            // that does not generate a problem
            "{'valid':123,'sub-member':{'valid':123,'sub-sub-member':{'sub-sub-invalid'123},'ignore':'this'}}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COLON_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a colon (:) as the JSON object member name and member value separator.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123,'re-valid':{'sub-valid':123,'sub-sub-member':{'sub-sub-valid':123},'more-valid':'this'},'valid':'again'}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_OBJECT_MEMBER_DEFINED_TWICE;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "the same object member \"valid\" was defined twice, which is not allowed in JSON.";
        tc.f_expected.push_back(expected1);

        as2js::JSON::pointer_t json(new as2js::JSON);
        // defined twice does not mean we get a null pointer...
        // (we should enhance this test to verify the result which is
        // that we keep the first entry with a given name.)
        CPPUNIT_ASSERT(json->parse(in) != as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "{'valid':123 'next-member':456}"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COMMA_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a comma (,) to separate two JSON object members.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "['valid',-123,,'next-item',456]"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (COMMA) found in a JSON input stream.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "['valid',-555,'bad-neg',-'123']"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (STRING) found after a '-' sign, a number was expected.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "['valid',+555,'bad-pos',+'123']"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (STRING) found after a '+' sign, a number was expected.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "['valid',123 'next-item',456]"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COMMA_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a comma (,) to separate two JSON array items.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    {
        as2js::String str(
            "['valid',[123 'next-item'],456]"
        );
        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_COMMA_EXPECTED;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "expected a comma (,) to separate two JSON array items.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }

    // skip controls to avoid problems with the lexer itself...
    for(as2js::as_char_t c(0x20); c < 0x110000; ++c)
    {
        switch(c)
        {
        //case '\n':
        //case '\r':
        //case '\t':
        case ' ':
        case '{':
        case '[':
        case '\'':
        case '"':
        case '#':
        case '-':
        case '@':
        case '\\':
        case '`':
        case 0x7F:
        //case ',': -- that would generate errors because it would be in the wrong place
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            // that looks like valid entries as is... so ignore
            continue;

        default:
            if(c >= 0xD800 && c <= 0xDFFF)
            {
                // skip surrogate, no need to test those
                continue;
            }
            if(!is_identifier_char(c))
            {
                // skip "punctuation" for now...
                continue;
            }
            break;

        }
        as2js::String str;
        str += c;

        as2js::Node::pointer_t node;
        {
            as2js::Options::pointer_t options(new as2js::Options);
            options->set_option(as2js::Options::option_t::OPTION_JSON, 1);
            as2js::Input::pointer_t input(new as2js::StringInput(str));
            as2js::Lexer::pointer_t lexer(new as2js::Lexer(input, options));
            CPPUNIT_ASSERT(lexer->get_input() == input);
            node = lexer->get_next_token();
            CPPUNIT_ASSERT(!!node);
        }

        as2js::StringInput::pointer_t in(new as2js::StringInput(str));

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_TOKEN;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "unexpected token (";
        expected1.f_message += node->get_type_name();
        expected1.f_message += ") found in a JSON input stream.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_CANNOT_COMPILE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "could not interpret this JSON input \"\".";
        tc.f_expected.push_back(expected2);

        as2js::JSON::pointer_t json(new as2js::JSON);
        CPPUNIT_ASSERT(json->parse(in) == as2js::JSON::JSONValue::pointer_t());
        tc.got_called();
    }
}



// vim: ts=4 sw=4 et
