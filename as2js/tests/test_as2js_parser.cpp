/* test_as2js_parser.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "test_as2js_parser.h"
#include    "test_as2js_main.h"

#include    "as2js/parser.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"
#include    "as2js/json.h"

#include    <controlled_vars/controlled_vars_limited_auto_enum_init.h>

#include    <unistd.h>
#include    <sys/stat.h>

#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsParserUnitTests );

namespace
{


int32_t generate_string(as2js::String& str, bool ascii)
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
            if(ascii)
            {
                c &= 0x7F;
            }
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
        // skip trace messages which happen all the time because of the
        // lexer debug option
        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_TRACE)
        {
            return;
        }

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
        CPPUNIT_ASSERT(message == f_expected[0].f_message.to_utf8());

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
        controlled_vars::tlbool_t   f_call;
        as2js::message_level_t      f_message_level;
        as2js::err_code_t           f_error_code;
        as2js::Position             f_pos;
        as2js::String               f_message; // UTF-8 string
    };

    std::vector<expected_t>     f_expected;

    static controlled_vars::zint32_t   g_warning_count;
    static controlled_vars::zint32_t   g_error_count;
};

controlled_vars::zint32_t   test_callback::g_warning_count;
controlled_vars::zint32_t   test_callback::g_error_count;

controlled_vars::zint32_t   g_empty_home_too_late;


as2js::Options::option_t g_options[] =
{
    as2js::Options::option_t::OPTION_ALLOW_WITH,
    as2js::Options::option_t::OPTION_BINARY,
    as2js::Options::option_t::OPTION_DEBUG,
    //as2js::Options::option_t::OPTION_DEBUG_LEXER, -- we have a separate test to test that one properly
    as2js::Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES,
    as2js::Options::option_t::OPTION_EXTENDED_OPERATORS,
    as2js::Options::option_t::OPTION_EXTENDED_STATEMENTS,
    as2js::Options::option_t::OPTION_JSON,
    as2js::Options::option_t::OPTION_OCTAL,
    as2js::Options::option_t::OPTION_STRICT,
    as2js::Options::option_t::OPTION_TRACE,
    as2js::Options::option_t::OPTION_TRACE_TO_OBJECT
};
size_t const g_options_size(sizeof(g_options) / sizeof(g_options[0]));


struct err_to_string_t
{
    as2js::err_code_t       f_code;
    char const *            f_name;
    int                     f_line;
};

#define    TO_STR_sub(s)            #s
#define    ERROR_NAME(err)     { as2js::err_code_t::AS_ERR_##err, TO_STR_sub(err), __LINE__ }

err_to_string_t const g_error_table[] =
{
    ERROR_NAME(NONE),
    ERROR_NAME(ABSTRACT),
    ERROR_NAME(BAD_NUMERIC_TYPE),
    ERROR_NAME(BAD_PRAGMA),
    ERROR_NAME(CANNOT_COMPILE),
    ERROR_NAME(CANNOT_MATCH),
    ERROR_NAME(CANNOT_OVERLOAD),
    ERROR_NAME(CANNOT_OVERWRITE_CONST),
    ERROR_NAME(CASE_LABEL),
    ERROR_NAME(COLON_EXPECTED),
    ERROR_NAME(COMMA_EXPECTED),
    ERROR_NAME(CURVLY_BRAKETS_EXPECTED),
    ERROR_NAME(DEFAULT_LABEL),
    ERROR_NAME(DIVIDE_BY_ZERO),
    ERROR_NAME(DUPLICATES),
    ERROR_NAME(DYNAMIC),
    ERROR_NAME(EXPRESSION_EXPECTED),
    ERROR_NAME(FINAL),
    ERROR_NAME(IMPROPER_STATEMENT),
    ERROR_NAME(INACCESSIBLE_STATEMENT),
    ERROR_NAME(INCOMPATIBLE),
    ERROR_NAME(INCOMPATIBLE_PRAGMA_ARGUMENT),
    ERROR_NAME(INSTALLATION),
    ERROR_NAME(INSTANCE_EXPECTED),
    ERROR_NAME(INTERNAL_ERROR),
    ERROR_NAME(NATIVE),
    ERROR_NAME(INVALID_ARRAY_FUNCTION),
    ERROR_NAME(INVALID_ATTRIBUTES),
    ERROR_NAME(INVALID_CATCH),
    ERROR_NAME(INVALID_CLASS),
    ERROR_NAME(INVALID_CONDITIONAL),
    ERROR_NAME(INVALID_DEFINITION),
    ERROR_NAME(INVALID_DO),
    ERROR_NAME(INVALID_ENUM),
    ERROR_NAME(INVALID_EXPRESSION),
    ERROR_NAME(INVALID_FIELD),
    ERROR_NAME(INVALID_FIELD_NAME),
    ERROR_NAME(INVALID_FRAME),
    ERROR_NAME(INVALID_FUNCTION),
    ERROR_NAME(INVALID_GOTO),
    ERROR_NAME(INVALID_INPUT_STREAM),
    ERROR_NAME(INVALID_KEYWORD),
    ERROR_NAME(INVALID_LABEL),
    ERROR_NAME(INVALID_NAMESPACE),
    ERROR_NAME(INVALID_NODE),
    ERROR_NAME(INVALID_NUMBER),
    ERROR_NAME(INVALID_OPERATOR),
    ERROR_NAME(INVALID_PACKAGE_NAME),
    ERROR_NAME(INVALID_PARAMETERS),
    ERROR_NAME(INVALID_REST),
    ERROR_NAME(INVALID_RETURN_TYPE),
    ERROR_NAME(INVALID_SCOPE),
    ERROR_NAME(INVALID_TRY),
    ERROR_NAME(INVALID_TYPE),
    ERROR_NAME(INVALID_UNICODE_ESCAPE_SEQUENCE),
    ERROR_NAME(INVALID_VARIABLE),
    ERROR_NAME(IO_ERROR),
    ERROR_NAME(LABEL_NOT_FOUND),
    ERROR_NAME(LOOPING_REFERENCE),
    ERROR_NAME(MISMATCH_FUNC_VAR),
    ERROR_NAME(MISSSING_VARIABLE_NAME),
    ERROR_NAME(NEED_CONST),
    ERROR_NAME(NOT_FOUND),
    ERROR_NAME(NOT_SUPPORTED),
    ERROR_NAME(OBJECT_MEMBER_DEFINED_TWICE),
    ERROR_NAME(PARENTHESIS_EXPECTED),
    ERROR_NAME(PRAGMA_FAILED),
    ERROR_NAME(SEMICOLON_EXPECTED),
    ERROR_NAME(SQUARE_BRAKETS_EXPECTED),
    ERROR_NAME(STRING_EXPECTED),
    ERROR_NAME(STATIC),
    ERROR_NAME(TYPE_NOT_LINKED),
    ERROR_NAME(UNKNOWN_ESCAPE_SEQUENCE),
    ERROR_NAME(UNKNOWN_OPERATOR),
    ERROR_NAME(UNTERMINATED_STRING),
    ERROR_NAME(UNEXPECTED_EOF),
    ERROR_NAME(UNEXPECTED_PUNCTUATION),
    ERROR_NAME(UNEXPECTED_TOKEN),
    ERROR_NAME(UNEXPECTED_DATABASE),
    ERROR_NAME(UNEXPECTED_RC)
};
size_t const g_error_table_size = sizeof(g_error_table) / sizeof(g_error_table[0]);


as2js::err_code_t str_to_error_code(as2js::String const& error_name)
{
    for(size_t idx(0); idx < g_error_table_size; ++idx)
    {
        if(error_name == g_error_table[idx].f_name)
        {
            return g_error_table[idx].f_code;
        }
    }
    CPPUNIT_ASSERT(!"error code not found, test_as2js_parser.cpp bug");
    return as2js::err_code_t::AS_ERR_NONE;
}


void verify_result(as2js::JSON::JSONValue::pointer_t expected, as2js::Node::pointer_t node)
{
    as2js::String node_type_string;
    node_type_string.from_utf8("node_type");
    as2js::String children_string;
    children_string.from_utf8("children");

    CPPUNIT_ASSERT(expected->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
    as2js::JSON::JSONValue::object_t const& child_object(expected->get_object());

    as2js::JSON::JSONValue::pointer_t node_type_value(child_object.find(node_type_string)->second);
    CPPUNIT_ASSERT(node->get_type_name() == node_type_value->get_string());

    as2js::JSON::JSONValue::object_t::const_iterator it(child_object.find(children_string));
    if(it != child_object.end())
    {
        // the children value must be an array
        as2js::JSON::JSONValue::array_t const& array(it->second->get_array());
        size_t const max_children(array.size());
        CPPUNIT_ASSERT(max_children == node->get_children_size());
        for(size_t idx(0); idx < max_children; ++idx)
        {
            as2js::JSON::JSONValue::pointer_t children_value(array[idx]);
            verify_result(children_value, node->get_child(idx));
        }
    }
    else
    {
        // no children defined in the JSON, no children expected in the node
        CPPUNIT_ASSERT(node->get_children_size() == 0);
    }
}


// JSON data used to test the parser, most of the work is in this table
// this is one long JSON string
//
// Note: the top is an array so we can execute each program in the order
//       we define it...
char const g_data[] =
    "[" // start

    // Empty program
    "{"
        "\"name\": \"empty program\","
        "\"program\": \"\","
        "\"result\": {"
            "\"node_type\": \"PROGRAM\""
        "}"
    "},"

    // Empty program with comments
    "{"
        "\"name\": \"empty program with comments\","
        "\"program\": \"// a comment is just ignored\\n/* and the program is still just empty */\","
        "\"result\": {"
            "\"node_type\": \"PROGRAM\""
        "}"
    "},"

    // Empty program with semi-colons
    "{"
        "\"name\": \"empty program with semi-colons\","
        "\"program\": \";;;;;;;;;;\","
        "\"result\": {"
            "\"node_type\": \"PROGRAM\","
            "\"children\": ["
                "{"
                    "\"node_type\": \"DIRECTIVE_LIST\""
                "}"
            "]"
        "}"
    "},"

    // Unexpected ELSE instruction
    "{"
        "\"name\": \"unexpected \\\"else\\\" instruction\","
        "\"program\": \"else\","
        "\"expected messages\": ["
            "{"
                "\"message level\": 2,"
                "\"error code\": \"INVALID_KEYWORD\","
                "\"line #\": 1,"
                "\"message\": \"'else' not expected without an 'if' keyword.\""
            "}"
        "],"
        "\"result\": {"
            "\"node_type\": \"PROGRAM\","
            "\"children\": ["
                "{"
                    "\"node_type\": \"DIRECTIVE_LIST\""
                "}"
            "]"
        "}"
    "},"

    // Unexpected }
    "{"
        "\"name\": \"unexpected \\\"}\\\" character\","
        "\"program\": \"}\","
        "\"expected messages\": ["
            "{"
                "\"message level\": 2,"
                "\"error code\": \"CURVLY_BRAKETS_EXPECTED\","
                "\"line #\": 1,"
                "\"message\": \"'}' not expected without a '{'.\""
            "}"
        "],"
        "\"result\": {"
            "\"node_type\": \"PROGRAM\","
            "\"children\": ["
                "{"
                    "\"node_type\": \"DIRECTIVE_LIST\""
                "}"
            "]"
        "}"
    "},"

    // Try an empty package
    "{"
        "\"name\": \"empty package\","
        "\"program\": \"package name { }\","
        "\"result\": {"
            "\"node_type\": \"PROGRAM\","
            "\"children\": ["
                "{"
                    "\"node_type\": \"DIRECTIVE_LIST\","
                    "\"children\": ["
                        "{"
                            "\"node_type\": \"PACKAGE\","
                            "\"children\": ["
                                "{"
                                    "\"node_type\": \"DIRECTIVE_LIST\""
                                "}"
                            "]"
                        "}"
                    "]"
                "}"
            "]"
        "}"
    "}"

    "]" // end
;



}
// no name namespace




// These will be required in the compiler, not here
// because the parser & below do not use the rc/db
// stuff... only the compiler
//void As2JsParserUnitTests::setUp()
//{
//}
//
//
//void As2JsParserUnitTests::tearDown()
//{
//}


void As2JsParserUnitTests::test_basics()
{
    as2js::String input_data;
    input_data.from_utf8(g_data);

    if(as2js_test::g_save_parser_tests)
    {
        std::ofstream json_file;
        json_file.open("test_parser.json");
        CPPUNIT_ASSERT(json_file.is_open());
        json_file << "// To properly indent this JSON you may use http://json-indent.appspot.com/"
                << std::endl << g_data << std::endl;
    }

    as2js::StringInput::pointer_t in(new as2js::StringInput(input_data));
    as2js::JSON::pointer_t json_data(new as2js::JSON);
    as2js::JSON::JSONValue::pointer_t json(json_data->parse(in));

    // verify that the parser() did not fail
    CPPUNIT_ASSERT(!!json);
    CPPUNIT_ASSERT(json->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_ARRAY);

    as2js::String name_string;
    name_string.from_utf8("name");
    as2js::String program_string;
    program_string.from_utf8("program");
    as2js::String result_string;
    result_string.from_utf8("result");
    as2js::String expected_messages_string;
    expected_messages_string.from_utf8("expected messages");

    std::cout << "\n";

    as2js::JSON::JSONValue::array_t const& array(json->get_array());
    size_t const max_programs(array.size());
    for(size_t idx(0); idx < max_programs; ++idx)
    {
        as2js::JSON::JSONValue::pointer_t prog_obj(array[idx]);
        CPPUNIT_ASSERT(prog_obj->get_type() == as2js::JSON::JSONValue::type_t::JSON_TYPE_OBJECT);
        as2js::JSON::JSONValue::object_t const& prog(prog_obj->get_object());

        // got a program, try to compile it with all the possible options
        as2js::JSON::JSONValue::pointer_t name(prog.find(name_string)->second);
        std::cout << "  -- working on \"" << name->get_string() << "\" ... ";

        for(size_t opt(0); opt <= (1 << g_options_size); ++opt)
        {
            as2js::Options::pointer_t options;
            if(opt != (1 << g_options_size))
            {
                // if not equal to max. then create an actual options
                // object; otherwise we use the default (nullptr)
                options.reset(new as2js::Options);
                for(size_t o(0); o < g_options_size; ++o)
                {
                    if((opt & (1 << o)) != 0)
                    {
                        options->set_option(g_options[o], 1);
                    }
                }
            }

            as2js::JSON::JSONValue::pointer_t program_value(prog.find(program_string)->second);
            as2js::String program_source(program_value->get_string());
            as2js::StringInput::pointer_t prog_text(new as2js::StringInput(program_source));
            as2js::Parser::pointer_t parser(new as2js::Parser(prog_text, options));

            test_callback tc;

            as2js::JSON::JSONValue::object_t::const_iterator expected_msg_it(prog.find(expected_messages_string));
            if(expected_msg_it != prog.end())
            {

                // the expected messages value must be an array
                as2js::JSON::JSONValue::array_t const& msg_array(expected_msg_it->second->get_array());
                size_t const max_msgs(msg_array.size());
                for(size_t j(0); j < max_msgs; ++j)
                {
                    as2js::JSON::JSONValue::pointer_t message_value(msg_array[j]);
                    as2js::JSON::JSONValue::object_t const& message(message_value->get_object());

                    test_callback::expected_t expected;
                    expected.f_message_level = static_cast<as2js::message_level_t>(message.find("message level")->second->get_int64().get());
                    expected.f_error_code = str_to_error_code(message.find("error code")->second->get_string());
                    expected.f_pos.set_filename("unknown-file");
                    as2js::JSON::JSONValue::object_t::const_iterator func_it(message.find("function name"));
                    if(func_it == message.end())
                    {
                        expected.f_pos.set_function("unknown-func");
                    }
                    else
                    {
                        expected.f_pos.set_function(func_it->second->get_string());
                    }
                    expected.f_message = message.find("message")->second->get_string();
                    tc.f_expected.push_back(expected);
                }
            }

            as2js::Node::pointer_t root(parser->parse());

            // the result is object which can have children
            // which are represented by an array of objects
            verify_result(prog.find(result_string)->second, root);
        }

        std::cout << "OK\n";
    }
}





// vim: ts=4 sw=4 et
