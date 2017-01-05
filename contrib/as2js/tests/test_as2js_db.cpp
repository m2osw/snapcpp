/* test_as2js_db.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_db.h"
#include    "test_as2js_main.h"

#include    "db.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <unistd.h>
#include    <sys/stat.h>

#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsDBUnitTests );

namespace
{


int32_t generate_string(as2js::String& str)
{
    as2js::as_char_t c;
    int32_t used(0);
    int ctrl(rand() % 7);
    int const max_chars(rand() % 25 + 20);
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
            used |= 0x01;
            break;

        case '\f':
            used |= 0x02;
            break;

        case '\n':
            used |= 0x04;
            break;

        case '\r':
            used |= 0x08;
            break;

        case '\t':
            used |= 0x10;
            break;

        case '"':
            used |= 0x20;
            break;

        case '\'':
            used |= 0x40;
            break;

        default:
            if(c < 0x0020)
            {
                // other controls must be escaped using Unicode
                used |= 0x80;
            }
            break;

        }
    }

    return used;
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
//std::cerr << "line = " << pos.get_line() << " / " << f_expected[0].f_pos.get_line() << "\n";
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
            std::cerr << "\n*** STILL " << f_expected.size() << " EXPECTED ***\n";
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


}
// no name namespace




void As2JsDBUnitTests::setUp()
{
    // we do not want a test.db or it would conflict with this test
    struct stat st;
    CPPUNIT_ASSERT(stat("test.db", &st) == -1);
}


void As2JsDBUnitTests::test_match()
{
    for(size_t idx(0); idx < 100; ++idx)
    {
        as2js::String start;
        generate_string(start);
        as2js::String middle;
        generate_string(middle);
        as2js::String end;
        generate_string(end);

        as2js::String name;
        name = start + middle + end;
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, "*"));

        as2js::String p1(start);
        p1 += '*';
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p1));

        as2js::String p2(start);
        p2 += '*';
        p2 += middle;
        p2 += '*';
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p2));

        as2js::String p3(start);
        p3 += '*';
        p3 += end;
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p3));

        as2js::String p4;
        p4 += '*';
        p4 += middle;
        p4 += '*';
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p4));

        as2js::String p5;
        p5 += '*';
        p5 += middle;
        p5 += '*';
        p5 += end;
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p5));

        as2js::String p6(start);
        p6 += '*';
        p6 += middle;
        p6 += '*';
        p6 += end;
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p6));

        as2js::String p7;
        p7 += '*';
        p7 += end;
        CPPUNIT_ASSERT(as2js::Database::match_pattern(name, p7));
    }
}


void As2JsDBUnitTests::test_element()
{
    int32_t used_type(0);
    int32_t used_filename(0);
    for(size_t idx(0); idx < 100 || used_type != 0xFF || used_filename != 0xFF; ++idx)
    {
        as2js::Position pos;

        as2js::String raw_type;
        used_type |= generate_string(raw_type);
        as2js::JSON::JSONValue::pointer_t type(new as2js::JSON::JSONValue(pos, raw_type));

        as2js::String raw_filename;
        used_filename |= generate_string(raw_filename);
        as2js::JSON::JSONValue::pointer_t filename(new as2js::JSON::JSONValue(pos, raw_filename));

        // generate a line number
        int32_t raw_line((rand() & 0xFFFFFF) + 1);
        as2js::Int64 line_int64(raw_line);
        as2js::JSON::JSONValue::pointer_t line(new as2js::JSON::JSONValue(pos, line_int64));

        as2js::JSON::JSONValue::object_t obj;
        obj["filename"] = filename;
        obj["type"] = type;
        obj["line"] = line;
        as2js::JSON::JSONValue::pointer_t element(new as2js::JSON::JSONValue(pos, obj));

        as2js::Database::Element::pointer_t db_element(new as2js::Database::Element("this.is.an.element.name", element));

        CPPUNIT_ASSERT(db_element->get_element_name() == "this.is.an.element.name");
        CPPUNIT_ASSERT(db_element->get_type() == raw_type);
        CPPUNIT_ASSERT(db_element->get_filename() == raw_filename);
        CPPUNIT_ASSERT(db_element->get_line() == raw_line);

        generate_string(raw_type);
        db_element->set_type(raw_type);
        CPPUNIT_ASSERT(db_element->get_type() == raw_type);

        generate_string(raw_filename);
        db_element->set_filename(raw_filename);
        CPPUNIT_ASSERT(db_element->get_filename() == raw_filename);

        raw_line = (rand() & 0xFFFFFF) + 1;
        db_element->set_line(raw_line);
        CPPUNIT_ASSERT(db_element->get_line() == raw_line);
    }

    // now check for erroneous data
    {
        as2js::Position pos;

        as2js::String not_obj;
        generate_string(not_obj);
        as2js::JSON::JSONValue::pointer_t bad_element(new as2js::JSON::JSONValue(pos, not_obj));

        CPPUNIT_ASSERT_THROW(new as2js::Database::Element("expect.a.throw", bad_element), as2js::exception_internal_error);
    }

    {
        as2js::Position pos;

        int32_t bad_raw_type((rand() & 0xFFFFFF) + 1);
        as2js::Int64 bad_type_int64(bad_raw_type);
        as2js::JSON::JSONValue::pointer_t bad_type(new as2js::JSON::JSONValue(pos, bad_type_int64));

        double bad_raw_filename(static_cast<double>((rand() << 16) ^ rand()) / static_cast<double>((rand() << 16) ^ rand()));
        as2js::Float64 bad_filename_float64(bad_raw_filename);
        as2js::JSON::JSONValue::pointer_t bad_filename(new as2js::JSON::JSONValue(pos, bad_filename_float64));

        // generate a line number
        as2js::String bad_raw_line;
        generate_string(bad_raw_line);
        as2js::JSON::JSONValue::pointer_t bad_line(new as2js::JSON::JSONValue(pos, bad_raw_line));

        as2js::JSON::JSONValue::object_t bad_obj;
        bad_obj["filename"] = bad_filename;
        bad_obj["type"] = bad_type;
        bad_obj["line"] = bad_line;
        as2js::JSON::JSONValue::pointer_t element(new as2js::JSON::JSONValue(pos, bad_obj));

        // WARNING: errors should be generated in the order the elements
        //          appear in the map
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "The filename of an element in the database has to be a string.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "The line of an element in the database has to be an integer.";
        tc.f_expected.push_back(expected2);

        test_callback::expected_t expected3;
        expected3.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected3.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected3.f_pos.set_filename("unknown-file");
        expected3.f_pos.set_function("unknown-func");
        expected3.f_message = "The type of an element in the database has to be a string.";
        tc.f_expected.push_back(expected3);

        as2js::Database::Element::pointer_t db_element(new as2js::Database::Element("this.is.a.bad.element.name", element));
        tc.got_called();

        CPPUNIT_ASSERT(db_element->get_element_name() == "this.is.a.bad.element.name");
        CPPUNIT_ASSERT(db_element->get_type() == "");
        CPPUNIT_ASSERT(db_element->get_filename() == "");
        CPPUNIT_ASSERT(db_element->get_line() == 1);
    }
}


void As2JsDBUnitTests::test_package()
{
    for(size_t idx(0); idx < 100; ++idx)
    {
        as2js::Position pos;

        // one package of 10 elements
        as2js::JSON::JSONValue::object_t package_obj;

        struct data_t
        {
            as2js::String   f_element_name;
            as2js::String   f_type;
            as2js::String   f_filename;
            int32_t         f_line;
        };
        std::vector<data_t> elements;

        for(size_t j(0); j < 10; ++j)
        {
            data_t data;

            generate_string(data.f_type);
            as2js::JSON::JSONValue::pointer_t type(new as2js::JSON::JSONValue(pos, data.f_type));

            generate_string(data.f_filename);
            as2js::JSON::JSONValue::pointer_t filename(new as2js::JSON::JSONValue(pos, data.f_filename));

            // generate a line number
            data.f_line = (rand() & 0xFFFFFF) + 1;
            as2js::Int64 line_int64(data.f_line);
            as2js::JSON::JSONValue::pointer_t line(new as2js::JSON::JSONValue(pos, line_int64));

            as2js::JSON::JSONValue::object_t obj;
            obj["type"] = type;
            obj["filename"] = filename;
            obj["line"] = line;
            as2js::JSON::JSONValue::pointer_t element(new as2js::JSON::JSONValue(pos, obj));

            generate_string(data.f_element_name);
            package_obj[data.f_element_name] = element;

            elements.push_back(data);

            // as we're here, make sure we can create such a db element
            as2js::Database::Element::pointer_t db_element(new as2js::Database::Element(data.f_element_name, element));

            CPPUNIT_ASSERT(db_element->get_element_name() == data.f_element_name);
            CPPUNIT_ASSERT(db_element->get_type() == data.f_type);
            CPPUNIT_ASSERT(db_element->get_filename() == data.f_filename);
            CPPUNIT_ASSERT(db_element->get_line() == data.f_line);
        }

        as2js::JSON::JSONValue::pointer_t package(new as2js::JSON::JSONValue(pos, package_obj));
        as2js::String package_name;
        generate_string(package_name);
        as2js::Database::Package::pointer_t db_package(new as2js::Database::Package(package_name, package));

        CPPUNIT_ASSERT(db_package->get_package_name() == package_name);

        for(size_t j(0); j < 10; ++j)
        {
            as2js::Database::Element::pointer_t e(db_package->get_element(elements[j].f_element_name));

            CPPUNIT_ASSERT(e->get_element_name() == elements[j].f_element_name);
            CPPUNIT_ASSERT(e->get_type()         == elements[j].f_type);
            CPPUNIT_ASSERT(e->get_filename()     == elements[j].f_filename);
            CPPUNIT_ASSERT(e->get_line()         == elements[j].f_line);

            // the add_element() does nothing if we add an element with the
            // same name
            as2js::Database::Element::pointer_t n(db_package->add_element(elements[j].f_element_name));
            CPPUNIT_ASSERT(n == e);
        }

        // attempts a find as well
        for(size_t j(0); j < 10; ++j)
        {
            {
                // pattern "starts with"
                int len(rand() % 5 + 1);
                as2js::String pattern(elements[j].f_element_name.substr(0, len));
                pattern += '*';
                as2js::Database::element_vector_t list(db_package->find_elements(pattern));

                // check that the name of the elements found this way are valid
                // matches
                size_t const max_elements(list.size());
                CPPUNIT_ASSERT(max_elements >= 1);
                for(size_t k(0); k < max_elements; ++k)
                {
                    as2js::String name(list[k]->get_element_name());
                    as2js::String match(name.substr(0, len));
                    match += '*';
                    CPPUNIT_ASSERT(pattern == match);
                }

                // now verify that we found them all
                for(size_t q(0); q < 10; ++q)
                {
                    as2js::String name(elements[q].f_element_name);
                    as2js::String start_with(name.substr(0, len));
                    start_with += '*';
                    if(start_with == pattern)
                    {
                        // find that entry in the list
                        bool good(false);
                        for(size_t k(0); k < max_elements; ++k)
                        {
                            if(list[k]->get_element_name() == name)
                            {
                                good = true;
                                break;
                            }
                        }
                        CPPUNIT_ASSERT(good);
                    }
                }
            }

            {
                // pattern "ends with"
                int len(rand() % 5 + 1);
                as2js::String pattern;
                pattern += '*';
                pattern += elements[j].f_element_name.substr(elements[j].f_element_name.length() - len, len);
                as2js::Database::element_vector_t list(db_package->find_elements(pattern));

                // check that the name of the elements found this way are valid
                // matches
                size_t const max_elements(list.size());
                CPPUNIT_ASSERT(max_elements >= 1);
                for(size_t k(0); k < max_elements; ++k)
                {
                    as2js::String name(list[k]->get_element_name());
                    as2js::String match;
                    match += '*';
                    match += name.substr(name.length() - len, len);
                    CPPUNIT_ASSERT(pattern == match);
                }

                // now verify that we found them all
                for(size_t q(0); q < 10; ++q)
                {
                    as2js::String name(elements[q].f_element_name);
                    as2js::String end_with;
                    end_with += '*';
                    end_with += name.substr(name.length() - len, len);
                    if(end_with == pattern)
                    {
                        // find that entry in the list
                        bool good(false);
                        for(size_t k(0); k < max_elements; ++k)
                        {
                            if(list[k]->get_element_name() == name)
                            {
                                good = true;
                                break;
                            }
                        }
                        CPPUNIT_ASSERT(good);
                    }
                }
            }

            {
                // pattern "starts/ends with"
                // names are generated by the generate_string() so they are
                // at least 20 characters long which is enough here
                int slen(rand() % 5 + 1);
                int elen(rand() % 5 + 1);
                as2js::String pattern;
                pattern += elements[j].f_element_name.substr(0, slen);
                pattern += '*';
                pattern += elements[j].f_element_name.substr(elements[j].f_element_name.length() - elen, elen);
                as2js::Database::element_vector_t list(db_package->find_elements(pattern));

                // check that the name of the elements found this way are valid
                // matches
                size_t const max_elements(list.size());
                CPPUNIT_ASSERT(max_elements >= 1);
                for(size_t k(0); k < max_elements; ++k)
                {
                    as2js::String name(list[k]->get_element_name());
                    as2js::String match;
                    match += name.substr(0, slen);
                    match += '*';
                    match += name.substr(name.length() - elen, elen);
                    CPPUNIT_ASSERT(pattern == match);
                }

                // now verify that we found them all
                for(size_t q(0); q < 10; ++q)
                {
                    as2js::String name(elements[q].f_element_name);
                    as2js::String end_with;
                    end_with += name.substr(0, slen);
                    end_with += '*';
                    end_with += name.substr(name.length() - elen, elen);
                    if(end_with == pattern)
                    {
                        // find that entry in the list
                        bool good(false);
                        for(size_t k(0); k < max_elements; ++k)
                        {
                            if(list[k]->get_element_name() == name)
                            {
                                good = true;
                                break;
                            }
                        }
                        CPPUNIT_ASSERT(good);
                    }
                }
            }
        }

        // add a few more elements
        for(size_t j(0); j < 10; ++j)
        {
            // at this point the name of an element is not verified because
            // all the internal code expects valid identifiers for those
            // names so any random name will do in this test
            as2js::String name;
            generate_string(name);
            as2js::Database::Element::pointer_t e(db_package->add_element(name));

            // it creates an empty element in this case
            CPPUNIT_ASSERT(e->get_element_name() == name);
            CPPUNIT_ASSERT(e->get_type() == "");
            CPPUNIT_ASSERT(e->get_filename() == "");
            CPPUNIT_ASSERT(e->get_line() == 1);
        }
    }

    // now check for erroneous data
    {
        as2js::Position pos;

        as2js::String not_obj;
        generate_string(not_obj);
        as2js::JSON::JSONValue::pointer_t bad_package(new as2js::JSON::JSONValue(pos, not_obj));

        CPPUNIT_ASSERT_THROW(new as2js::Database::Package("expect.a.throw", bad_package), as2js::exception_internal_error);
    }

    {
        as2js::Position pos;

        int32_t bad_int((rand() & 0xFFFFFF) + 1);
        as2js::Int64 bad_int64(bad_int);
        as2js::JSON::JSONValue::pointer_t bad_a(new as2js::JSON::JSONValue(pos, bad_int64));

        double bad_float(static_cast<double>((rand() << 16) ^ rand()) / static_cast<double>((rand() << 16) ^ rand()));
        as2js::Float64 bad_float64(bad_float);
        as2js::JSON::JSONValue::pointer_t bad_b(new as2js::JSON::JSONValue(pos, bad_float64));

        as2js::String bad_string;
        generate_string(bad_string);
        as2js::JSON::JSONValue::pointer_t bad_c(new as2js::JSON::JSONValue(pos, bad_string));

        //as2js::JSON::JSONValue::object_t bad_obj;
        //as2js::String n1;
        //generate_string(n1);
        //bad_obj[n1] = bad_a;
        //as2js::String n2;
        //generate_string(n2);
        //bad_obj[n2] = bad_b;
        //as2js::String n3;
        //generate_string(n3);
        //bad_obj[n3] = bad_c;
        //as2js::JSON::JSONValue::pointer_t element(new as2js::JSON::JSONValue(pos, bad_obj));

        as2js::JSON::JSONValue::object_t package_obj;
        as2js::String e1_name;
        generate_string(e1_name);
        package_obj[e1_name] = bad_a;

        as2js::String e2_name;
        generate_string(e2_name);
        package_obj[e2_name] = bad_b;

        as2js::String e3_name;
        generate_string(e3_name);
        package_obj[e3_name] = bad_c;

        // WARNING: errors should be generated in the order the elements
        //          appear in the map
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "A database is expected to be an object of object packages composed of object elements.";
        tc.f_expected.push_back(expected1);

        test_callback::expected_t expected2;
        expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected2.f_pos.set_filename("unknown-file");
        expected2.f_pos.set_function("unknown-func");
        expected2.f_message = "A database is expected to be an object of object packages composed of object elements.";
        tc.f_expected.push_back(expected2);

        test_callback::expected_t expected3;
        expected3.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected3.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected3.f_pos.set_filename("unknown-file");
        expected3.f_pos.set_function("unknown-func");
        expected3.f_message = "A database is expected to be an object of object packages composed of object elements.";
        tc.f_expected.push_back(expected3);

        as2js::JSON::JSONValue::pointer_t package(new as2js::JSON::JSONValue(pos, package_obj));

        as2js::String package_name;
        generate_string(package_name);
        as2js::Database::Package::pointer_t db_package(new as2js::Database::Package(package_name, package));
        tc.got_called();
        CPPUNIT_ASSERT(!!db_package);
    }
}


void As2JsDBUnitTests::test_database()
{
    as2js::Database::pointer_t db(new as2js::Database);

    // saving without a load does nothing
    db->save();

    // whatever the package name, it does not exist...
    CPPUNIT_ASSERT(!db->get_package("name"));

    // adding a package fails with a throw
    CPPUNIT_ASSERT_THROW(db->add_package("name"), as2js::exception_internal_error);

    // the find_packages() function returns nothing
    as2js::Database::package_vector_t v(db->find_packages("name"));
    CPPUNIT_ASSERT(v.empty());

    // now test a load()
    CPPUNIT_ASSERT(db->load("test.db"));

    // a second time returns true also
    CPPUNIT_ASSERT(db->load("test.db"));

    as2js::Database::Package::pointer_t p1(db->add_package("p1"));
    as2js::Database::Element::pointer_t e1(p1->add_element("e1"));
    e1->set_type("type-e1");
    e1->set_filename("e1.as");
    e1->set_line(33);
    as2js::Database::Element::pointer_t e2(p1->add_element("e2"));
    e2->set_type("type-e2");
    e2->set_filename("e2.as");
    e2->set_line(66);
    as2js::Database::Element::pointer_t e3(p1->add_element("e3"));
    e3->set_type("type-e3");
    e3->set_filename("e3.as");
    e3->set_line(99);

    as2js::Database::Package::pointer_t p2(db->add_package("p2"));
    as2js::Database::Element::pointer_t e4(p2->add_element("e4"));
    e4->set_type("type-e4");
    e4->set_filename("e4.as");
    e4->set_line(44);
    as2js::Database::Element::pointer_t e5(p2->add_element("e5"));
    e5->set_type("type-e5");
    e5->set_filename("e5.as");
    e5->set_line(88);
    as2js::Database::Element::pointer_t e6(p2->add_element("e6"));
    e6->set_type("type-e6");
    e6->set_filename("e6.as");
    e6->set_line(11);

    db->save();

    CPPUNIT_ASSERT(db->get_package("p1") == p1);
    CPPUNIT_ASSERT(db->get_package("p2") == p2);

    as2js::Database::Package::pointer_t q(db->get_package("p1"));
    CPPUNIT_ASSERT(q == p1);
    as2js::Database::Package::pointer_t r(db->get_package("p2"));
    CPPUNIT_ASSERT(r == p2);

    as2js::Database::pointer_t qdb(new as2js::Database);
    CPPUNIT_ASSERT(qdb->load("test.db"));

    as2js::Database::Package::pointer_t np1(qdb->get_package("p1"));
    as2js::Database::Element::pointer_t ne1(np1->get_element("e1"));
    CPPUNIT_ASSERT(ne1->get_type() == "type-e1");
    CPPUNIT_ASSERT(ne1->get_filename() == "e1.as");
    CPPUNIT_ASSERT(ne1->get_line() == 33);
    as2js::Database::Element::pointer_t ne2(np1->get_element("e2"));
    CPPUNIT_ASSERT(ne2->get_type() == "type-e2");
    CPPUNIT_ASSERT(ne2->get_filename() == "e2.as");
    CPPUNIT_ASSERT(ne2->get_line() == 66);
    as2js::Database::Element::pointer_t ne3(np1->get_element("e3"));
    CPPUNIT_ASSERT(ne3->get_type() == "type-e3");
    CPPUNIT_ASSERT(ne3->get_filename() == "e3.as");
    CPPUNIT_ASSERT(ne3->get_line() == 99);
    as2js::Database::Package::pointer_t np2(qdb->get_package("p2"));
    as2js::Database::Element::pointer_t ne4(np2->get_element("e4"));
    CPPUNIT_ASSERT(ne4->get_type() == "type-e4");
    CPPUNIT_ASSERT(ne4->get_filename() == "e4.as");
    CPPUNIT_ASSERT(ne4->get_line() == 44);
    as2js::Database::Element::pointer_t ne5(np2->get_element("e5"));
    CPPUNIT_ASSERT(ne5->get_type() == "type-e5");
    CPPUNIT_ASSERT(ne5->get_filename() == "e5.as");
    CPPUNIT_ASSERT(ne5->get_line() == 88);
    as2js::Database::Element::pointer_t ne6(np2->get_element("e6"));
    CPPUNIT_ASSERT(ne6->get_type() == "type-e6");
    CPPUNIT_ASSERT(ne6->get_filename() == "e6.as");
    CPPUNIT_ASSERT(ne6->get_line() == 11);

    as2js::Database::package_vector_t np1a(qdb->find_packages("p1"));
    CPPUNIT_ASSERT(np1a.size() == 1);
    CPPUNIT_ASSERT(np1a[0] == np1);
    as2js::Database::package_vector_t np2a(qdb->find_packages("p2"));
    CPPUNIT_ASSERT(np2a.size() == 1);
    CPPUNIT_ASSERT(np2a[0] == np2);
    as2js::Database::package_vector_t np3a(qdb->find_packages("p*"));
    CPPUNIT_ASSERT(np3a.size() == 2);
    CPPUNIT_ASSERT(np3a[0] == np1);
    CPPUNIT_ASSERT(np3a[1] == np2);

    // done with that one
    unlink("test.db");

    {
        {
            std::ofstream db_file;
            db_file.open("t1.db");
            CPPUNIT_ASSERT(db_file.is_open());
            db_file << "// db file\n"
                    << "an invalid file\n";
        }

        as2js::Database::pointer_t pdb(new as2js::Database);
        CPPUNIT_ASSERT(!pdb->load("t1.db"));
        // make sure we can still create a package (because here f_value
        // is null)
        as2js::Database::Package::pointer_t tp(pdb->add_package("another"));
        CPPUNIT_ASSERT(!!tp);

        unlink("t1.db");
    }

    {
        {
            std::ofstream db_file;
            db_file.open("t2.db");
            CPPUNIT_ASSERT(db_file.is_open());
            db_file << "// db file\n"
                    << "null\n";
        }

        as2js::Database::pointer_t pdb(new as2js::Database);
        CPPUNIT_ASSERT(pdb->load("t2.db"));
        as2js::Database::package_vector_t np(pdb->find_packages("*"));
        CPPUNIT_ASSERT(np.empty());

        unlink("t2.db");
    }

    {
        {
            std::ofstream db_file;
            db_file.open("t3.db");
            CPPUNIT_ASSERT(db_file.is_open());
            db_file << "// db file\n"
                    << "\"unexpected string\"\n";
        }

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("t3.db");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "A database must be defined as a JSON object, or set to 'null'.";
        tc.f_expected.push_back(expected1);

        as2js::Database::pointer_t sdb(new as2js::Database);
        CPPUNIT_ASSERT(!sdb->load("t3.db"));
        tc.got_called();

        as2js::Database::package_vector_t np(sdb->find_packages("*"));
        CPPUNIT_ASSERT(np.empty());

        unlink("t3.db");
    }

    {
        {
            std::ofstream db_file;
            db_file.open("t4.db");
            CPPUNIT_ASSERT(db_file.is_open());
            db_file << "// db file\n"
                    << "{\"invalid\":\"object-here\"}\n";
        }

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_DATABASE;
        expected1.f_pos.set_filename("t4.db");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "A database is expected to be an object of object packages composed of elements.";
        tc.f_expected.push_back(expected1);

        as2js::Database::pointer_t sdb(new as2js::Database);
        CPPUNIT_ASSERT(!sdb->load("t4.db"));
        tc.got_called();

        as2js::Database::package_vector_t np(sdb->find_packages("*"));
        CPPUNIT_ASSERT(np.empty());

        unlink("t4.db");
    }
}



// vim: ts=4 sw=4 et
