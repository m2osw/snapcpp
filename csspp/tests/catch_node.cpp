// CSS Preprocessor -- Test Suite
// Copyright (C) 2015  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Test the node.cpp file.
 *
 * This test runs a battery of tests agains the node.cpp
 * implementation to ensure full coverage.
 */

#include "catch_tests.h"

#include "csspp/exceptions.h"
#include "csspp/lexer.h"
#include "csspp/unicode_range.h"

#include <sstream>

#include <string.h>

namespace
{


} // no name namespace


TEST_CASE("Node types", "[node] [type]")
{
    // we expect the test suite to be compiled with the exact same version
    csspp::node_type_t w(csspp::node_type_t::UNKNOWN);
    while(w <= csspp::node_type_t::max_type)
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(w, pos));

        // verify the type
        REQUIRE(n->get_type() == w);

        n->set_flag("important", true);
        REQUIRE(n->get_flag("important"));
        n->set_flag("important", false);
        REQUIRE_FALSE(n->get_flag("important"));

        // boolean
        switch(w)
        {
        case csspp::node_type_t::BOOLEAN:
        case csspp::node_type_t::OPEN_CURLYBRACKET:
            {
                bool b(rand() % 1 == 0);
                n->set_boolean(b);
                REQUIRE(n->get_boolean() == b);
                if(w == csspp::node_type_t::OPEN_CURLYBRACKET)
                {
                    REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_INVALID);
                }
                else
                {
                    REQUIRE(n->to_boolean() == (b ? csspp::boolean_t::BOOLEAN_TRUE : csspp::boolean_t::BOOLEAN_FALSE));
                }
            }
            break;

        case csspp::node_type_t::DECIMAL_NUMBER:
        case csspp::node_type_t::INTEGER:
        case csspp::node_type_t::PERCENT:
            {
                bool b(rand() % 1 == 0);
                n->set_boolean(b);
                REQUIRE(n->get_boolean() == b);
                // the to_boolean() converts the value not the f_boolean field
                // this test MUST happen before the next or we would not know
                // whether it  true or false
                REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);
            }
            break;

        default:
            REQUIRE_THROWS_AS(n->set_boolean(true), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_boolean(), csspp::csspp_exception_logic);
            break;

        }

        // integer
        switch(w)
        {
        case csspp::node_type_t::AN_PLUS_B:
        case csspp::node_type_t::ARG:
        case csspp::node_type_t::AT_KEYWORD:
        case csspp::node_type_t::COMMENT:
        case csspp::node_type_t::INTEGER:
        case csspp::node_type_t::UNICODE_RANGE:
            {
                if(w == csspp::node_type_t::INTEGER)
                {
                    REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);
                }
                csspp::integer_t i(static_cast<csspp::integer_t>(rand()) + (static_cast<csspp::integer_t>(rand()) << 32));
                n->set_integer(i);
                REQUIRE(n->get_integer() == i);
                if(w == csspp::node_type_t::INTEGER)
                {
                    REQUIRE(n->to_boolean() == (i != 0 ? csspp::boolean_t::BOOLEAN_TRUE : csspp::boolean_t::BOOLEAN_FALSE));
                }
            }
            break;

        default:
            REQUIRE_THROWS_AS(n->set_integer(123), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_integer(), csspp::csspp_exception_logic);
            break;

        }

        // decimal number
        switch(w)
        {
        case csspp::node_type_t::DECIMAL_NUMBER:
        case csspp::node_type_t::PERCENT:
            REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);
            n->set_decimal_number(123.456);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            REQUIRE(n->get_decimal_number() == 123.456);
#pragma GCC diagnostic pop
            REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_TRUE);
            break;

        default:
            REQUIRE_THROWS_AS(n->set_decimal_number(3.14159), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_decimal_number(), csspp::csspp_exception_logic);
            break;

        }

        // string
        switch(w)
        {
        case csspp::node_type_t::AT_KEYWORD:
        case csspp::node_type_t::COMMENT:
        case csspp::node_type_t::DECIMAL_NUMBER:
        case csspp::node_type_t::DECLARATION:
        case csspp::node_type_t::EXCLAMATION:
        case csspp::node_type_t::FUNCTION:
        case csspp::node_type_t::HASH:
        case csspp::node_type_t::IDENTIFIER:
        case csspp::node_type_t::INTEGER:
        case csspp::node_type_t::PLACEHOLDER:
        case csspp::node_type_t::STRING:
        case csspp::node_type_t::URL:
        case csspp::node_type_t::VARIABLE:
        case csspp::node_type_t::VARIABLE_FUNCTION:
            if(w == csspp::node_type_t::STRING)
            {
                REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);
            }
            n->set_string("test-string");
            REQUIRE(n->get_string() == "test-string");
            if(w == csspp::node_type_t::STRING)
            {
                REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_TRUE);
            }
            break;

        default:
            REQUIRE_THROWS_AS(n->set_string("add"), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_string(), csspp::csspp_exception_logic);
            break;

        }

        {
            csspp::color c;
            switch(w)
            {
            case csspp::node_type_t::COLOR:
                {
                    c.set_color(rand() % 255, rand() % 255, rand() % 255, rand() % 255);
                    n->set_color(c);
                    csspp::color d(n->get_color());
                    REQUIRE(c.get_color() == d.get_color());
                }
                break;

            default:
                REQUIRE_THROWS_AS(n->set_color(c), csspp::csspp_exception_logic);
                REQUIRE_THROWS_AS(n->get_color(), csspp::csspp_exception_logic);
                break;

            }
        }

        // font metrics
        switch(w)
        {
        case csspp::node_type_t::FONT_METRICS:
            n->set_font_size(12.5);
            n->set_dim1("px");
            n->set_line_height(24.3);
            n->set_dim2("%");
            //REQUIRE(n->get_string() == "px/%"); -- we do not allow get_string()
            //REQUIRE(n->get_integer() == 12.5 as a double); -- we do not allow get_integer()
            //REQUIRE(n->get_decimal_number() == 24.3); -- we do not allow get_decimal_number()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            REQUIRE(n->get_font_size() == 12.5);
            REQUIRE(n->get_dim1() == "px");
            REQUIRE(n->get_line_height() == 24.3);
            REQUIRE(n->get_dim2() == "%");
#pragma GCC diagnostic pop

            // dimensions require a few more tests
            n->set_dim2("deg");  // chane dim2
            REQUIRE(n->get_dim1() == "px");
            REQUIRE(n->get_dim2() == "deg");
            n->set_dim2("");  // remove dim2
            REQUIRE(n->get_dim1() == "px");
            REQUIRE(n->get_dim2() == "");
            n->set_dim1("");  // remove dim1
            REQUIRE(n->get_dim1() == "");
            REQUIRE(n->get_dim2() == "");
            n->set_dim2("em"); // set dim2 without a dim1
            REQUIRE(n->get_dim1() == "");
            REQUIRE(n->get_dim2() == "em");
            n->set_dim1("px"); // set a dim1 with a dim2
            REQUIRE(n->get_dim1() == "px");
            REQUIRE(n->get_dim2() == "em");
            n->set_dim1("");  // remove dim1 with a dim2
            REQUIRE(n->get_dim1() == "");
            REQUIRE(n->get_dim2() == "em");
            n->set_dim2("");  // remove dim2 without a dim1
            REQUIRE(n->get_dim1() == "");
            REQUIRE(n->get_dim2() == "");
            break;

        default:
            REQUIRE_THROWS_AS(n->set_font_size(12.5), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_font_size(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->set_line_height(24.3), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_line_height(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->set_dim1("px"), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_dim1(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->set_dim2("%"), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_dim2(), csspp::csspp_exception_logic);
            break;

        }

        // children
        switch(w)
        {
        case csspp::node_type_t::ARG:
        case csspp::node_type_t::ARRAY:
        case csspp::node_type_t::AT_KEYWORD:
        case csspp::node_type_t::COMPONENT_VALUE:
        case csspp::node_type_t::DECLARATION:
        case csspp::node_type_t::FUNCTION:
        case csspp::node_type_t::LIST:
        case csspp::node_type_t::MAP:
        case csspp::node_type_t::OPEN_CURLYBRACKET:
        case csspp::node_type_t::OPEN_PARENTHESIS:
        case csspp::node_type_t::OPEN_SQUAREBRACKET:
        case csspp::node_type_t::VARIABLE_FUNCTION:
            {
                // try adding one child
                if(w == csspp::node_type_t::LIST)
                {
                    REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);
                }
                REQUIRE(n->empty());
                REQUIRE(n->size() == 0);
                REQUIRE_THROWS_AS(n->get_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->get_last_child(), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);

                csspp::node::pointer_t child1(new csspp::node(csspp::node_type_t::COMMA, n->get_position()));
                csspp::node::pointer_t child2(new csspp::node(csspp::node_type_t::EXCLAMATION, n->get_position()));
                csspp::node::pointer_t eof_token(new csspp::node(csspp::node_type_t::EOF_TOKEN, n->get_position()));
                csspp::node::pointer_t whitespace1(new csspp::node(csspp::node_type_t::WHITESPACE, n->get_position()));
                csspp::node::pointer_t whitespace2(new csspp::node(csspp::node_type_t::WHITESPACE, n->get_position()));

                n->add_child(child1);
                if(w == csspp::node_type_t::LIST)
                {
                    REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_TRUE);
                }
                REQUIRE(n->size() == 1);
                REQUIRE_FALSE(n->empty());
                REQUIRE(n->get_last_child() == child1);
                REQUIRE(n->get_child(0) == child1);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);

                n->add_child(child2);
                if(w == csspp::node_type_t::LIST)
                {
                    REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_TRUE);
                }
                REQUIRE(n->size() == 2);
                REQUIRE_FALSE(n->empty());
                REQUIRE(n->get_last_child() == child2);
                REQUIRE(n->get_child(0) == child1);
                REQUIRE(n->get_child(1) == child2);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);

                if(rand() & 1)
                {
                    n->remove_child(0);
                    REQUIRE(n->size() == 1);

                    n->remove_child(child2);
                }
                else
                {
                    n->remove_child(child2);
                    REQUIRE(n->size() == 1);

                    n->remove_child(0);
                }

                // fully empty again, all fails like follow
                if(w == csspp::node_type_t::LIST)
                {
                    REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);
                }
                REQUIRE(n->empty());
                REQUIRE(n->size() == 0);
                REQUIRE_THROWS_AS(n->get_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->get_last_child(), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);

                // test a few more things
                n->add_child(child1);
                REQUIRE(n->size() == 1);
                n->insert_child(1, whitespace1);
                REQUIRE(n->size() == 2);
                n->add_child(whitespace2); // ignore two whitespaces in a row
                REQUIRE(n->size() == 2);
                n->insert_child(0, child2);
                REQUIRE(n->size() == 3);
                n->add_child(eof_token);  // never add EOF_TOKEN
                REQUIRE(n->size() == 3);
                REQUIRE(n->get_child(0) == child2);
                REQUIRE(n->get_child(1) == child1);
                REQUIRE(n->get_child(2) == whitespace1);
                REQUIRE(n->child_position(child1) == 1);
                REQUIRE(n->child_position(child2) == 0);
                REQUIRE(n->child_position(whitespace1) == 2);
                REQUIRE(n->child_position(whitespace2) == csspp::node::npos);
                REQUIRE(n->child_position(eof_token) == csspp::node::npos);
                n->clear();
                REQUIRE(n->size() == 0);
                REQUIRE(n->child_position(child1) == csspp::node::npos);
                REQUIRE(n->child_position(child2) == csspp::node::npos);
                REQUIRE(n->child_position(whitespace1) == csspp::node::npos);
                REQUIRE(n->child_position(whitespace2) == csspp::node::npos);
                REQUIRE(n->child_position(eof_token) == csspp::node::npos);

                // test the replace
                n->add_child(child1);
                REQUIRE(n->size() == 1);
                REQUIRE(n->get_child(0) == child1);
                n->replace_child(child1, child2);
                REQUIRE(n->size() == 1);
                REQUIRE(n->get_child(0) == child2);

                csspp::node::pointer_t list(new csspp::node(csspp::node_type_t::LIST, n->get_position()));
                n->add_child(child1);
                list->take_over_children_of(n);
                REQUIRE(n->size() == 0);
                REQUIRE(list->size() == 2);
                REQUIRE(list->get_child(0) == child2);
                REQUIRE(list->get_child(1) == child1);
            }
            break;

        default:
            REQUIRE_THROWS_AS(n->empty(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->size(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->clear(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->add_child(n), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->remove_child(0), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_child(0), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_last_child(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->take_over_children_of(0), csspp::csspp_exception_logic);
            break;

        }

        // all invalid node types for to_boolean()
        switch(w)
        {
        case csspp::node_type_t::ARRAY:
        case csspp::node_type_t::BOOLEAN:
        case csspp::node_type_t::DECIMAL_NUMBER:
        case csspp::node_type_t::INTEGER:
        case csspp::node_type_t::LIST:
        case csspp::node_type_t::MAP:
        case csspp::node_type_t::PERCENT:
        case csspp::node_type_t::STRING:
            break;

        default:
            REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_INVALID);
            break;

        }

        // move to the next type
        w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid tree handling", "[node] [invalid]")
{
    // replace with an invalid child
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::LIST, pos));

        csspp::node::pointer_t child1(new csspp::node(csspp::node_type_t::INTEGER, pos));
        child1->set_integer(33);
        csspp::node::pointer_t child2(new csspp::node(csspp::node_type_t::STRING, pos));
        child2->set_string("hello!");

        n->add_child(child1);

        // child2, child1 are inverted
        REQUIRE_THROWS_AS(n->replace_child(child2, child1), csspp::csspp_exception_logic);
    }

    // insert with invalid index
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::LIST, pos));

        csspp::node::pointer_t child1(new csspp::node(csspp::node_type_t::INTEGER, pos));
        child1->set_integer(345);
        csspp::node::pointer_t child2(new csspp::node(csspp::node_type_t::STRING, pos));
        child2->set_string("world.");

        n->insert_child(0, child1);

        // insert index can be 0 or 1, anything else and it is an overflow
        for(int i(-100); i < 0; ++i)
        {
            REQUIRE_THROWS_AS(n->insert_child(i, child2), csspp::csspp_exception_overflow);
        }
        for(int i(2); i <= 100; ++i)
        {
            REQUIRE_THROWS_AS(n->insert_child(i, child2), csspp::csspp_exception_overflow);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("True and false", "[node] [type] [output]")
{
    // test boolean values
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::IDENTIFIER, pos));

        REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_INVALID);

        n->set_string("true");
        REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_TRUE);

        n->set_string("false");
        REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);

        n->set_string("null");
        REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_FALSE);

        n->set_string("other");
        REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_INVALID);

        // fortuitious...
        n->set_string("invalid");
        REQUIRE(n->to_boolean() == csspp::boolean_t::BOOLEAN_INVALID);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Node variables", "[node] [variable]")
{
    // set/get variables
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::LIST, pos));

        csspp::node::pointer_t t[21];
        for(int i(-10); i <= 10; ++i)
        {
            std::string nb(std::to_string(i));
            t[i + 10].reset(new csspp::node(csspp::node_type_t::IDENTIFIER, pos));
            t[i + 10]->set_string("test" + nb);

            n->set_variable("t" + nb, t[i + 10]);
            REQUIRE(n->get_variable("t" + nb) == t[i + 10]);
        }

        // check contents again
        for(int i(-10); i <= 10; ++i)
        {
            std::string nb(std::to_string(i));

            csspp::node::pointer_t p(n->get_variable("t" + nb));
            REQUIRE(p == t[i + 10]);
            REQUIRE(p->get_string() == "test" + nb);
        }

        n->clear_variables();

        // all are gone now!
        for(int i(-10); i <= 10; ++i)
        {
            std::string nb(std::to_string(i));

            csspp::node::pointer_t p(n->get_variable("t" + nb));
            REQUIRE(!p);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Node flags", "[node] [flag]")
{
    // read/write flags
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::LIST, pos));

        for(int i(-10); i <= 10; ++i)
        {
            std::string nb("t" + std::to_string(i));

            n->set_flag(nb, true);
            REQUIRE(n->get_flag(nb));
            n->set_flag(nb, false);
            REQUIRE_FALSE(n->get_flag(nb));
            n->set_flag(nb, true);
            REQUIRE(n->get_flag(nb));
        }

        // check contents again
        for(int i(-10); i <= 10; ++i)
        {
            std::string nb("t" + std::to_string(i));

            REQUIRE(n->get_flag(nb));
        }

        n->clear_flags();

        // all are gone now!
        for(int i(-10); i <= 10; ++i)
        {
            std::string nb("t" + std::to_string(i));

            REQUIRE_FALSE(n->get_flag(nb));
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Type names", "[node] [type] [output]")
{
    // we expect the test suite to be compiled with the exact same version
    csspp::node_type_t w(csspp::node_type_t::UNKNOWN);
    while(w <= csspp::node_type_t::max_type)
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(w, pos));

        std::stringstream ss;
        ss << n->get_type();
        std::string const name(ss.str());

        switch(w)
        {
        case csspp::node_type_t::UNKNOWN:
            REQUIRE(name == "UNKNOWN");
            break;

        case csspp::node_type_t::ADD:
            REQUIRE(name == "ADD");
            break;

        case csspp::node_type_t::AND:
            REQUIRE(name == "AND");
            break;

        case csspp::node_type_t::ASSIGNMENT:
            REQUIRE(name == "ASSIGNMENT");
            break;

        case csspp::node_type_t::AT_KEYWORD:
            REQUIRE(name == "AT_KEYWORD");
            break;

        case csspp::node_type_t::BOOLEAN:
            REQUIRE(name == "BOOLEAN");
            break;

        case csspp::node_type_t::CDC:
            REQUIRE(name == "CDC");
            break;

        case csspp::node_type_t::CDO:
            REQUIRE(name == "CDO");
            break;

        case csspp::node_type_t::CLOSE_CURLYBRACKET:
            REQUIRE(name == "CLOSE_CURLYBRACKET");
            break;

        case csspp::node_type_t::CLOSE_PARENTHESIS:
            REQUIRE(name == "CLOSE_PARENTHESIS");
            break;

        case csspp::node_type_t::CLOSE_SQUAREBRACKET:
            REQUIRE(name == "CLOSE_SQUAREBRACKET");
            break;

        case csspp::node_type_t::COLON:
            REQUIRE(name == "COLON");
            break;

        case csspp::node_type_t::COLOR:
            REQUIRE(name == "COLOR");
            break;

        case csspp::node_type_t::COLUMN:
            REQUIRE(name == "COLUMN");
            break;

        case csspp::node_type_t::COMMA:
            REQUIRE(name == "COMMA");
            break;

        case csspp::node_type_t::COMMENT:
            REQUIRE(name == "COMMENT");
            break;

        case csspp::node_type_t::CONDITIONAL:
            REQUIRE(name == "CONDITIONAL");
            break;

        case csspp::node_type_t::DASH_MATCH:
            REQUIRE(name == "DASH_MATCH");
            break;

        case csspp::node_type_t::DECIMAL_NUMBER:
            REQUIRE(name == "DECIMAL_NUMBER");
            break;

        case csspp::node_type_t::DIVIDE:
            REQUIRE(name == "DIVIDE");
            break;

        case csspp::node_type_t::DOLLAR:
            REQUIRE(name == "DOLLAR");
            break;

        case csspp::node_type_t::EOF_TOKEN:
            REQUIRE(name == "EOF_TOKEN");
            break;

        case csspp::node_type_t::EQUAL:
            REQUIRE(name == "EQUAL");
            break;

        case csspp::node_type_t::EXCLAMATION:
            REQUIRE(name == "EXCLAMATION");
            break;

        case csspp::node_type_t::FONT_METRICS:
            REQUIRE(name == "FONT_METRICS");
            break;

        case csspp::node_type_t::FUNCTION:
            REQUIRE(name == "FUNCTION");
            break;

        case csspp::node_type_t::GREATER_EQUAL:
            REQUIRE(name == "GREATER_EQUAL");
            break;

        case csspp::node_type_t::GREATER_THAN:
            REQUIRE(name == "GREATER_THAN");
            break;

        case csspp::node_type_t::HASH:
            REQUIRE(name == "HASH");
            break;

        case csspp::node_type_t::IDENTIFIER:
            REQUIRE(name == "IDENTIFIER");
            break;

        case csspp::node_type_t::INCLUDE_MATCH:
            REQUIRE(name == "INCLUDE_MATCH");
            break;

        case csspp::node_type_t::INTEGER:
            REQUIRE(name == "INTEGER");
            break;

        case csspp::node_type_t::LESS_EQUAL:
            REQUIRE(name == "LESS_EQUAL");
            break;

        case csspp::node_type_t::LESS_THAN:
            REQUIRE(name == "LESS_THAN");
            break;

        case csspp::node_type_t::MODULO:
            REQUIRE(name == "MODULO");
            break;

        case csspp::node_type_t::MULTIPLY:
            REQUIRE(name == "MULTIPLY");
            break;

        case csspp::node_type_t::NOT_EQUAL:
            REQUIRE(name == "NOT_EQUAL");
            break;

        case csspp::node_type_t::NULL_TOKEN:
            REQUIRE(name == "NULL_TOKEN");
            break;

        case csspp::node_type_t::OPEN_CURLYBRACKET:
            REQUIRE(name == "OPEN_CURLYBRACKET");
            break;

        case csspp::node_type_t::OPEN_PARENTHESIS:
            REQUIRE(name == "OPEN_PARENTHESIS");
            break;

        case csspp::node_type_t::OPEN_SQUAREBRACKET:
            REQUIRE(name == "OPEN_SQUAREBRACKET");
            break;

        case csspp::node_type_t::PERCENT:
            REQUIRE(name == "PERCENT");
            break;

        case csspp::node_type_t::PERIOD:
            REQUIRE(name == "PERIOD");
            break;

        case csspp::node_type_t::PLACEHOLDER:
            REQUIRE(name == "PLACEHOLDER");
            break;

        case csspp::node_type_t::POWER:
            REQUIRE(name == "POWER");
            break;

        case csspp::node_type_t::PRECEDED:
            REQUIRE(name == "PRECEDED");
            break;

        case csspp::node_type_t::PREFIX_MATCH:
            REQUIRE(name == "PREFIX_MATCH");
            break;

        case csspp::node_type_t::REFERENCE:
            REQUIRE(name == "REFERENCE");
            break;

        case csspp::node_type_t::SCOPE:
            REQUIRE(name == "SCOPE");
            break;

        case csspp::node_type_t::SEMICOLON:
            REQUIRE(name == "SEMICOLON");
            break;

        case csspp::node_type_t::STRING:
            REQUIRE(name == "STRING");
            break;

        case csspp::node_type_t::SUBSTRING_MATCH:
            REQUIRE(name == "SUBSTRING_MATCH");
            break;

        case csspp::node_type_t::SUBTRACT:
            REQUIRE(name == "SUBTRACT");
            break;

        case csspp::node_type_t::SUFFIX_MATCH:
            REQUIRE(name == "SUFFIX_MATCH");
            break;

        case csspp::node_type_t::UNICODE_RANGE:
            REQUIRE(name == "UNICODE_RANGE");
            break;

        case csspp::node_type_t::URL:
            REQUIRE(name == "URL");
            break;

        case csspp::node_type_t::VARIABLE:
            REQUIRE(name == "VARIABLE");
            break;

        case csspp::node_type_t::VARIABLE_FUNCTION:
            REQUIRE(name == "VARIABLE_FUNCTION");
            break;

        case csspp::node_type_t::WHITESPACE:
            REQUIRE(name == "WHITESPACE");
            break;

        // second part
        case csspp::node_type_t::AN_PLUS_B:
            REQUIRE(name == "AN_PLUS_B");
            break;

        case csspp::node_type_t::ARG:
            REQUIRE(name == "ARG");
            break;

        case csspp::node_type_t::ARRAY:
            REQUIRE(name == "ARRAY");
            break;

        case csspp::node_type_t::COMPONENT_VALUE:
            REQUIRE(name == "COMPONENT_VALUE");
            break;

        case csspp::node_type_t::DECLARATION:
            REQUIRE(name == "DECLARATION");
            break;

        case csspp::node_type_t::LIST:
            REQUIRE(name == "LIST");
            break;

        case csspp::node_type_t::MAP:
            REQUIRE(name == "MAP");
            break;

        case csspp::node_type_t::max_type:
            REQUIRE(name == "max_type");
            break;

        }

        // move to the next type
        w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Node output", "[node] [output]")
{
    // we expect the test suite to be compiled with the exact same version
    csspp::node_type_t w(csspp::node_type_t::UNKNOWN);
    while(w <= csspp::node_type_t::max_type)
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(w, pos));

        std::stringstream ss;
        ss << n->get_type();
        std::string const name(ss.str());

        switch(w)
        {
        case csspp::node_type_t::UNKNOWN:
            REQUIRE(name == "UNKNOWN");
            break;

        case csspp::node_type_t::ADD:
            REQUIRE(name == "ADD");
            break;

        case csspp::node_type_t::AND:
            REQUIRE(name == "AND");
            break;

        case csspp::node_type_t::ASSIGNMENT:
            REQUIRE(name == "ASSIGNMENT");
            break;

        case csspp::node_type_t::AT_KEYWORD:
            REQUIRE(name == "AT_KEYWORD");
            break;

        case csspp::node_type_t::BOOLEAN:
            REQUIRE(name == "BOOLEAN");
            break;

        case csspp::node_type_t::CDC:
            REQUIRE(name == "CDC");
            break;

        case csspp::node_type_t::CDO:
            REQUIRE(name == "CDO");
            break;

        case csspp::node_type_t::CLOSE_CURLYBRACKET:
            REQUIRE(name == "CLOSE_CURLYBRACKET");
            break;

        case csspp::node_type_t::CLOSE_PARENTHESIS:
            REQUIRE(name == "CLOSE_PARENTHESIS");
            break;

        case csspp::node_type_t::CLOSE_SQUAREBRACKET:
            REQUIRE(name == "CLOSE_SQUAREBRACKET");
            break;

        case csspp::node_type_t::COLON:
            REQUIRE(name == "COLON");
            break;

        case csspp::node_type_t::COLOR:
            REQUIRE(name == "COLOR");
            break;

        case csspp::node_type_t::COLUMN:
            REQUIRE(name == "COLUMN");
            break;

        case csspp::node_type_t::COMMA:
            REQUIRE(name == "COMMA");
            break;

        case csspp::node_type_t::COMMENT:
            REQUIRE(name == "COMMENT");
            break;

        case csspp::node_type_t::CONDITIONAL:
            REQUIRE(name == "CONDITIONAL");
            break;

        case csspp::node_type_t::DASH_MATCH:
            REQUIRE(name == "DASH_MATCH");
            break;

        case csspp::node_type_t::DECIMAL_NUMBER:
            REQUIRE(name == "DECIMAL_NUMBER");
            break;

        case csspp::node_type_t::DIVIDE:
            REQUIRE(name == "DIVIDE");
            break;

        case csspp::node_type_t::DOLLAR:
            REQUIRE(name == "DOLLAR");
            break;

        case csspp::node_type_t::EOF_TOKEN:
            REQUIRE(name == "EOF_TOKEN");
            break;

        case csspp::node_type_t::EQUAL:
            REQUIRE(name == "EQUAL");
            break;

        case csspp::node_type_t::EXCLAMATION:
            REQUIRE(name == "EXCLAMATION");
            break;

        case csspp::node_type_t::FONT_METRICS:
            REQUIRE(name == "FONT_METRICS");
            break;

        case csspp::node_type_t::FUNCTION:
            REQUIRE(name == "FUNCTION");
            break;

        case csspp::node_type_t::GREATER_EQUAL:
            REQUIRE(name == "GREATER_EQUAL");
            break;

        case csspp::node_type_t::GREATER_THAN:
            REQUIRE(name == "GREATER_THAN");
            break;

        case csspp::node_type_t::HASH:
            REQUIRE(name == "HASH");
            break;

        case csspp::node_type_t::IDENTIFIER:
            REQUIRE(name == "IDENTIFIER");
            break;

        case csspp::node_type_t::INCLUDE_MATCH:
            REQUIRE(name == "INCLUDE_MATCH");
            break;

        case csspp::node_type_t::INTEGER:
            REQUIRE(name == "INTEGER");
            break;

        case csspp::node_type_t::LESS_EQUAL:
            REQUIRE(name == "LESS_EQUAL");
            break;

        case csspp::node_type_t::LESS_THAN:
            REQUIRE(name == "LESS_THAN");
            break;

        case csspp::node_type_t::MODULO:
            REQUIRE(name == "MODULO");
            break;

        case csspp::node_type_t::MULTIPLY:
            REQUIRE(name == "MULTIPLY");
            break;

        case csspp::node_type_t::NOT_EQUAL:
            REQUIRE(name == "NOT_EQUAL");
            break;

        case csspp::node_type_t::NULL_TOKEN:
            REQUIRE(name == "NULL_TOKEN");
            break;

        case csspp::node_type_t::OPEN_CURLYBRACKET:
            REQUIRE(name == "OPEN_CURLYBRACKET");
            break;

        case csspp::node_type_t::OPEN_PARENTHESIS:
            REQUIRE(name == "OPEN_PARENTHESIS");
            break;

        case csspp::node_type_t::OPEN_SQUAREBRACKET:
            REQUIRE(name == "OPEN_SQUAREBRACKET");
            break;

        case csspp::node_type_t::PERCENT:
            REQUIRE(name == "PERCENT");
            break;

        case csspp::node_type_t::PERIOD:
            REQUIRE(name == "PERIOD");
            break;

        case csspp::node_type_t::PLACEHOLDER:
            REQUIRE(name == "PLACEHOLDER");
            break;

        case csspp::node_type_t::POWER:
            REQUIRE(name == "POWER");
            break;

        case csspp::node_type_t::PRECEDED:
            REQUIRE(name == "PRECEDED");
            break;

        case csspp::node_type_t::PREFIX_MATCH:
            REQUIRE(name == "PREFIX_MATCH");
            break;

        case csspp::node_type_t::REFERENCE:
            REQUIRE(name == "REFERENCE");
            break;

        case csspp::node_type_t::SCOPE:
            REQUIRE(name == "SCOPE");
            break;

        case csspp::node_type_t::SEMICOLON:
            REQUIRE(name == "SEMICOLON");
            break;

        case csspp::node_type_t::STRING:
            REQUIRE(name == "STRING");
            break;

        case csspp::node_type_t::SUBSTRING_MATCH:
            REQUIRE(name == "SUBSTRING_MATCH");
            break;

        case csspp::node_type_t::SUBTRACT:
            REQUIRE(name == "SUBTRACT");
            break;

        case csspp::node_type_t::SUFFIX_MATCH:
            REQUIRE(name == "SUFFIX_MATCH");
            break;

        case csspp::node_type_t::UNICODE_RANGE:
            REQUIRE(name == "UNICODE_RANGE");
            break;

        case csspp::node_type_t::URL:
            REQUIRE(name == "URL");
            break;

        case csspp::node_type_t::VARIABLE:
            REQUIRE(name == "VARIABLE");
            break;

        case csspp::node_type_t::VARIABLE_FUNCTION:
            REQUIRE(name == "VARIABLE_FUNCTION");
            break;

        case csspp::node_type_t::WHITESPACE:
            REQUIRE(name == "WHITESPACE");
            break;

        // second part
        case csspp::node_type_t::AN_PLUS_B:
            REQUIRE(name == "AN_PLUS_B");
            break;

        case csspp::node_type_t::ARG:
            REQUIRE(name == "ARG");
            break;

        case csspp::node_type_t::ARRAY:
            REQUIRE(name == "ARRAY");
            break;

        case csspp::node_type_t::COMPONENT_VALUE:
            REQUIRE(name == "COMPONENT_VALUE");
            break;

        case csspp::node_type_t::DECLARATION:
            REQUIRE(name == "DECLARATION");
            break;

        case csspp::node_type_t::LIST:
            REQUIRE(name == "LIST");
            break;

        case csspp::node_type_t::MAP:
            REQUIRE(name == "MAP");
            break;

        case csspp::node_type_t::max_type:
            REQUIRE(name == "max_type");
            break;

        }

        // move to the next type
        w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Node to string", "[node] [type] [output]")
{
    // we expect the test suite to be compiled with the exact same version
    for(int flags(0); flags < 4; ++flags)
    {
        csspp::node_type_t w(csspp::node_type_t::UNKNOWN);
        while(w <= csspp::node_type_t::max_type)
        {
            csspp::position pos("test.css");
            csspp::node::pointer_t n(new csspp::node(w, pos));

            switch(w)
            {
            case csspp::node_type_t::ADD:
                REQUIRE(n->to_string(flags) == "+");
                break;

            case csspp::node_type_t::AND:
                REQUIRE(n->to_string(flags) == "&&");
                break;

            case csspp::node_type_t::ASSIGNMENT:
                REQUIRE(n->to_string(flags) == ":=");
                break;

            case csspp::node_type_t::AT_KEYWORD:
                REQUIRE(n->to_string(flags) == "@");
                n->set_string("test");
                REQUIRE(n->to_string(flags) == "@test");
                break;

            case csspp::node_type_t::BOOLEAN:
                REQUIRE(n->to_string(flags) == "false");
                n->set_boolean(true);
                REQUIRE(n->to_string(flags) == "true");
                break;

            case csspp::node_type_t::COLON:
                REQUIRE(n->to_string(flags) == ":");
                break;

            case csspp::node_type_t::COLOR:
                {
                    REQUIRE(n->to_string(flags) == "transparent");
                    csspp::color c;
                    c.set_color(0x58, 0x32, 0xff, 0xff);
                    n->set_color(c);
                    REQUIRE(n->to_string(flags) == "#5832ff");
                    c.set_color(0x58, 0x32, 0xff, 0x7f);
                    n->set_color(c);
                    REQUIRE(n->to_string(flags) == "rgba(88,50,255,0.5)");
                }
                break;

            case csspp::node_type_t::COLUMN:
                REQUIRE(n->to_string(flags) == "||");
                break;

            case csspp::node_type_t::COMMA:
                REQUIRE(n->to_string(flags) == ",");
                break;

            case csspp::node_type_t::COMMENT:
                // once we remove the @preserve, this could happen
                REQUIRE(n->to_string(flags) == "");
                n->set_string("the comment");
                REQUIRE(n->to_string(flags) == "// the comment\n");
                n->set_string("the comment\non two lines");
                REQUIRE(n->to_string(flags) == "// the comment\n// on two lines\n");
                n->set_integer(1);
                n->set_string("the C-like comment\non two lines");
                REQUIRE(n->to_string(flags) == "/* the C-like comment\non two lines */");
                break;

            case csspp::node_type_t::CONDITIONAL:
                REQUIRE(n->to_string(flags) == "?");
                break;

            case csspp::node_type_t::DASH_MATCH:
                REQUIRE(n->to_string(flags) == "|=");
                break;

            case csspp::node_type_t::DECIMAL_NUMBER:
                REQUIRE(n->to_string(flags) == "0");
                n->set_string("em");
                REQUIRE(n->to_string(flags) == "0em");
                n->set_decimal_number(1.25);
                REQUIRE(n->to_string(flags) == "1.25em");
                break;

            case csspp::node_type_t::DIVIDE:
                if((flags & csspp::node::g_to_string_flag_add_spaces) != 0)
                {
                    REQUIRE(n->to_string(flags) == " / ");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "/");
                }
                break;

            case csspp::node_type_t::DOLLAR:
                REQUIRE(n->to_string(flags) == "$");
                break;

            case csspp::node_type_t::EQUAL:
                REQUIRE(n->to_string(flags) == "=");
                break;

            case csspp::node_type_t::EXCLAMATION:
                REQUIRE(n->to_string(flags) == "!");
                break;

            case csspp::node_type_t::FONT_METRICS:
                REQUIRE(n->to_string(flags) ==
                          csspp::decimal_number_to_string(n->get_font_size(), false)
                        + n->get_dim1()
                        + "/"
                        + csspp::decimal_number_to_string(n->get_line_height(), false)
                        + n->get_dim2());
                break;

            case csspp::node_type_t::FUNCTION:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "()");

                    // test with an actual function
                    n->set_string("rgba");
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, n->get_position()));
                    p->set_decimal_number(1.0);
                    n->add_child(p);
                    REQUIRE(n->to_string(flags) == "rgba(0,0,0,1)");
                }
                break;

            case csspp::node_type_t::GREATER_EQUAL:
                REQUIRE(n->to_string(flags) == ">=");
                break;

            case csspp::node_type_t::GREATER_THAN:
                REQUIRE(n->to_string(flags) == ">");
                break;

            case csspp::node_type_t::HASH:
                REQUIRE(n->to_string(flags) == "#");
                n->set_string("random");
                REQUIRE(n->to_string(flags) == "#random");
                break;

            case csspp::node_type_t::IDENTIFIER:
                REQUIRE(n->to_string(flags) == "");
                n->set_string("an identifier in CSS can be absolutely anything!");
                REQUIRE(n->to_string(flags) == "an identifier in CSS can be absolutely anything!");
                break;

            case csspp::node_type_t::INCLUDE_MATCH:
                REQUIRE(n->to_string(flags) == "~=");
                break;

            case csspp::node_type_t::INTEGER:
                {
                    REQUIRE(n->to_string(flags) == "0");
                    csspp::integer_t i(static_cast<csspp::integer_t>(rand()) + (static_cast<csspp::integer_t>(rand()) << 32));
                    n->set_integer(i);
                    REQUIRE(n->to_string(flags) == std::to_string(i));
                    n->set_string("px");
                    REQUIRE(n->to_string(flags) == std::to_string(i) + "px");
                }
                break;

            case csspp::node_type_t::LESS_EQUAL:
                REQUIRE(n->to_string(flags) == "<=");
                break;

            case csspp::node_type_t::LESS_THAN:
                REQUIRE(n->to_string(flags) == "<");
                break;

            case csspp::node_type_t::MODULO:
                if((flags & csspp::node::g_to_string_flag_add_spaces) != 0)
                {
                    REQUIRE(n->to_string(flags) == " % ");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "%");
                }
                break;

            case csspp::node_type_t::MULTIPLY:
                REQUIRE(n->to_string(flags) == "*");
                break;

            case csspp::node_type_t::NOT_EQUAL:
                REQUIRE(n->to_string(flags) == "!=");
                break;

            case csspp::node_type_t::NULL_TOKEN:
                REQUIRE(n->to_string(flags) == "");
                break;

            case csspp::node_type_t::OPEN_CURLYBRACKET:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "{}");

                    // test with an actual function
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(7);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::MODULO, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(52);
                    n->add_child(p);
                    if((flags & csspp::node::g_to_string_flag_add_spaces) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "{7 % 52}");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "{7%52}");
                    }
                }
                break;

            case csspp::node_type_t::OPEN_PARENTHESIS:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "()");

                    // test with an actual function
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(17);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::MULTIPLY, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(502);
                    n->add_child(p);
                    REQUIRE(n->to_string(flags) == "(17*502)");
                }
                break;

            case csspp::node_type_t::OPEN_SQUAREBRACKET:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "[]");

                    // test with an actual function
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(5);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DIVIDE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(152);
                    n->add_child(p);
                    if((flags & csspp::node::g_to_string_flag_add_spaces) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "[5 / 152]");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "[5/152]");
                    }
                }
                break;

            case csspp::node_type_t::PERCENT:
                REQUIRE(n->to_string(flags) == "0%");
                n->set_decimal_number(1.25);
                REQUIRE(n->to_string(flags) == "1.25%");
                break;

            case csspp::node_type_t::PERIOD:
                REQUIRE(n->to_string(flags) == ".");
                break;

            case csspp::node_type_t::PLACEHOLDER:
                REQUIRE(n->to_string(flags) == "%");
                n->set_string("the-name-of-the-placeholder");
                REQUIRE(n->to_string(flags) == "%the-name-of-the-placeholder");
                break;

            case csspp::node_type_t::POWER:
                REQUIRE(n->to_string(flags) == "**");
                break;

            case csspp::node_type_t::PRECEDED:
                REQUIRE(n->to_string(flags) == "~");
                break;

            case csspp::node_type_t::PREFIX_MATCH:
                REQUIRE(n->to_string(flags) == "^=");
                break;

            case csspp::node_type_t::REFERENCE:
                REQUIRE(n->to_string(flags) == "&");
                break;

            case csspp::node_type_t::SCOPE:
                REQUIRE(n->to_string(flags) == "|");
                break;

            case csspp::node_type_t::SEMICOLON:
                REQUIRE(n->to_string(flags) == ";");
                break;

            case csspp::node_type_t::STRING:
                // with an empty string
                if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                {
                    REQUIRE(n->to_string(flags) == "\"\"");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "");
                }

                // with a ' in the string
                n->set_string("whatever string content we'd want really...");
                if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                {
                    REQUIRE(n->to_string(flags) == "\"whatever string content we'd want really...\"");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "whatever string content we'd want really...");
                }

                // with a " in the string
                n->set_string("yet if we have one quote like this: \" then the other is used to quote the string");
                if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                {
                    REQUIRE(n->to_string(flags) == "'yet if we have one quote like this: \" then the other is used to quote the string'");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "yet if we have one quote like this: \" then the other is used to quote the string");
                }

                // with both ' and ", more '
                n->set_string("counter: ''''' > \"\"\"");
                if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                {
                    REQUIRE(n->to_string(flags) == "\"counter: ''''' > \\\"\\\"\\\"\"");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "counter: ''''' > \"\"\"");
                }

                // with both ' and ", more "
                n->set_string("counter: ''' < \"\"\"\"\"");
                if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                {
                    REQUIRE(n->to_string(flags) == "'counter: \\'\\'\\' < \"\"\"\"\"'");
                }
                else
                {
                    REQUIRE(n->to_string(flags) == "counter: ''' < \"\"\"\"\"");
                }
                break;

            case csspp::node_type_t::SUBSTRING_MATCH:
                REQUIRE(n->to_string(flags) == "*=");
                break;

            case csspp::node_type_t::SUBTRACT:
                REQUIRE(n->to_string(flags) == "-");
                break;

            case csspp::node_type_t::SUFFIX_MATCH:
                REQUIRE(n->to_string(flags) == "$=");
                break;

            case csspp::node_type_t::UNICODE_RANGE:
                REQUIRE(n->to_string(flags) == "U+0");
                n->set_integer(0x00004FFF00004000);
                REQUIRE(n->to_string(flags) == "U+4???");
                break;

            case csspp::node_type_t::URL:
                REQUIRE(n->to_string(flags) == "url()");
                n->set_string("http://this.should.be/a/valid/url");
                REQUIRE(n->to_string(flags) == "url(http://this.should.be/a/valid/url)");
                break;

            case csspp::node_type_t::VARIABLE:
                REQUIRE(n->to_string(flags) == "$");
                n->set_string("varname");
                REQUIRE(n->to_string(flags) == "$varname");
                break;

            case csspp::node_type_t::VARIABLE_FUNCTION:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "$()");

                    // test with an actual function
                    n->set_string("my_function");
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("colorful");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(33);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, n->get_position()));
                    p->set_decimal_number(1.0);
                    n->add_child(p);
                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "$my_function(0,\"colorful\",33,1)");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "$my_function(0,colorful,33,1)");
                    }
                }
                break;

            case csspp::node_type_t::WHITESPACE:
                REQUIRE(n->to_string(flags) == " ");
                break;

            // second part
            case csspp::node_type_t::AN_PLUS_B:
                REQUIRE(n->to_string(flags) == "0");
                n->set_integer(0x0000000500000003);
                REQUIRE(n->to_string(flags) == "3n+5");
                break;

            case csspp::node_type_t::ARG:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "");
                    REQUIRE(n->get_integer() == 0);

                    // test with an actual function
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::ADD, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(33);
                    n->add_child(p);
                    REQUIRE(n->to_string(flags) == "0+33");
                }
                break;

            case csspp::node_type_t::ARRAY:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "()");

                    // each item is comma separated
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::IDENTIFIER, n->get_position()));
                    p->set_string("number");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, n->get_position()));
                    p->set_decimal_number(3.22);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::POWER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("hello world!");
                    n->add_child(p);

                    REQUIRE(n->to_string(flags) == "(number, 3.22, **, \"hello world!\")");
                }
                break;

            case csspp::node_type_t::COMPONENT_VALUE:
                // test with the default (undefined) separator
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "");

                    // test with an actual function
                    csspp::node::pointer_t a(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    n->add_child(a);
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    a->add_child(p);

                    a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    n->add_child(a);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("orange");
                    a->add_child(p);

                    a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    n->add_child(a);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(33);
                    a->add_child(p);

                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "0,\"orange\",33");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "0,orange,33");
                    }

                    // test with an actual function but not argified
                    n->clear();
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(111);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::COMMA, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("purple");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::COMMA, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(301);
                    n->add_child(p);

                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "111,\"purple\",301");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "111,purple,301");
                    }
                }

                // test again with "," as the ARG separator
                {
                    n->clear();

                    // test with an actual function
                    csspp::node::pointer_t a(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    a->set_integer(static_cast<int>(csspp::node_type_t::COMMA));
                    n->add_child(a);
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    a->add_child(p);

                    a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    a->set_integer(static_cast<int>(csspp::node_type_t::COMMA));
                    n->add_child(a);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("orange");
                    a->add_child(p);

                    a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    a->set_integer(static_cast<int>(csspp::node_type_t::COMMA));
                    n->add_child(a);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(33);
                    a->add_child(p);

                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "0,\"orange\",33");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "0,orange,33");
                    }

                    // test with an actual function but not argified
                    n->clear();
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(111);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::COMMA, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("purple");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::COMMA, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(301);
                    n->add_child(p);

                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "111,\"purple\",301");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "111,purple,301");
                    }
                }

                // test again with "/" as the ARG separator
                {
                    n->clear();

                    // test with an actual function
                    csspp::node::pointer_t a(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    a->set_integer(static_cast<int>(csspp::node_type_t::DIVIDE));
                    n->add_child(a);
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    a->add_child(p);

                    a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    a->set_integer(static_cast<int>(csspp::node_type_t::DIVIDE));
                    n->add_child(a);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("orange");
                    a->add_child(p);

                    a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
                    a->set_integer(static_cast<int>(csspp::node_type_t::DIVIDE));
                    n->add_child(a);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(33);
                    a->add_child(p);

                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        REQUIRE(n->to_string(flags) == "0/\"orange\"/33");
                    }
                    else
                    {
                        REQUIRE(n->to_string(flags) == "0/orange/33");
                    }

                    // test with an actual function but not argified
                    n->clear();
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(111);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DIVIDE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("purple");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DIVIDE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(301);
                    n->add_child(p);

                    if((flags & csspp::node::g_to_string_flag_show_quotes) != 0)
                    {
                        if((flags & csspp::node::g_to_string_flag_add_spaces) != 0)
                        {
                            REQUIRE(n->to_string(flags) == "111 / \"purple\" / 301");
                        }
                        else
                        {
                            REQUIRE(n->to_string(flags) == "111/\"purple\"/301");
                        }
                    }
                    else
                    {
                        if((flags & csspp::node::g_to_string_flag_add_spaces) != 0)
                        {
                            REQUIRE(n->to_string(flags) == "111 / purple / 301");
                        }
                        else
                        {
                            REQUIRE(n->to_string(flags) == "111/purple/301");
                        }
                    }
                }
                break;

            case csspp::node_type_t::DECLARATION:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "");

                    // test with an actual function
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::WHITESPACE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::SUBTRACT, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::WHITESPACE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
                    p->set_integer(33);
                    n->add_child(p);
                    REQUIRE(n->to_string(flags) == "0 - 33");
                }
                break;

            case csspp::node_type_t::LIST:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "");

                    // test with an actual function
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, n->get_position()));
                    p->set_decimal_number(3.22);
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::WHITESPACE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::POWER, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::WHITESPACE, n->get_position()));
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, n->get_position()));
                    p->set_decimal_number(5.3);
                    n->add_child(p);
                    REQUIRE(n->to_string(flags) == "3.22 ** 5.3");
                }
                break;

            case csspp::node_type_t::MAP:
                {
                    // the defaults are empty...
                    REQUIRE(n->to_string(flags) == "()");

                    // number: 3.22
                    csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::IDENTIFIER, n->get_position()));
                    p->set_string("number");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, n->get_position()));
                    p->set_decimal_number(3.22);
                    n->add_child(p);

                    // string: "hello world!"
                    p.reset(new csspp::node(csspp::node_type_t::IDENTIFIER, n->get_position()));
                    p->set_string("string");
                    n->add_child(p);
                    p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
                    p->set_string("hello world!");
                    n->add_child(p);

                    REQUIRE(n->to_string(flags) == "(number: 3.22, string: \"hello world!\")");
                }
                break;

            // anything else is an error
            default:
                // other node types generate a throw
                REQUIRE_THROWS_AS(n->to_string(flags), csspp::csspp_exception_logic);
                break;

            }

            // move to the next type
            w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Node to string ARG with wrong type", "[node] [output] [invalid]")
{
    // we expect the test suite to be compiled with the exact same version
    for(int flags(0); flags < 4; ++flags)
    {
        for(csspp::node_type_t w(csspp::node_type_t::UNKNOWN);
            w <= csspp::node_type_t::max_type;
            w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1))
        {
            switch(w)
            {
            case csspp::node_type_t::UNKNOWN:
            case csspp::node_type_t::COMMA:
            case csspp::node_type_t::DIVIDE:
                continue;

            default:
                break;

            }
            csspp::position pos("test.css");
            csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::COMPONENT_VALUE, pos));

            // test with an actual function
            csspp::node::pointer_t a(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
            a->set_integer(static_cast<csspp::integer_t>(w));
            n->add_child(a);
            csspp::node::pointer_t p(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
            a->add_child(p);

            a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
            a->set_integer(static_cast<csspp::integer_t>(w));
            n->add_child(a);
            p.reset(new csspp::node(csspp::node_type_t::STRING, n->get_position()));
            p->set_string("orange");
            a->add_child(p);

            a.reset(new csspp::node(csspp::node_type_t::ARG, n->get_position()));
            a->set_integer(static_cast<csspp::integer_t>(w));
            n->add_child(a);
            p.reset(new csspp::node(csspp::node_type_t::INTEGER, n->get_position()));
            p->set_integer(33);
            a->add_child(p);

            // w is not valid as an ARG separator so it throws
            REQUIRE_THROWS_AS(n->to_string(flags), csspp::csspp_exception_logic);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Error with node names", "[node] [type] [output]")
{
    // we expect the test suite to be compiled with the exact same version
    csspp::node_type_t w(csspp::node_type_t::UNKNOWN);
    while(w <= csspp::node_type_t::max_type)
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(w, pos));

        csspp::error::instance() << pos
                                 << "node name \""
                                 << n->get_type()
                                 << "\"."
                                 << csspp::error_mode_t::ERROR_ERROR;

        std::string name("good");
        switch(w)
        {
        case csspp::node_type_t::UNKNOWN:
            REQUIRE_ERRORS("test.css(1): error: node name \"UNKNOWN\".\n");
            break;

        case csspp::node_type_t::ADD:
            REQUIRE_ERRORS("test.css(1): error: node name \"ADD\".\n");
            break;

        case csspp::node_type_t::AND:
            REQUIRE_ERRORS("test.css(1): error: node name \"AND\".\n");
            break;

        case csspp::node_type_t::ASSIGNMENT:
            REQUIRE_ERRORS("test.css(1): error: node name \"ASSIGNMENT\".\n");
            break;

        case csspp::node_type_t::AT_KEYWORD:
            REQUIRE_ERRORS("test.css(1): error: node name \"AT_KEYWORD\".\n");
            break;

        case csspp::node_type_t::BOOLEAN:
            REQUIRE_ERRORS("test.css(1): error: node name \"BOOLEAN\".\n");
            break;

        case csspp::node_type_t::CDC:
            REQUIRE_ERRORS("test.css(1): error: node name \"CDC\".\n");
            break;

        case csspp::node_type_t::CDO:
            REQUIRE_ERRORS("test.css(1): error: node name \"CDO\".\n");
            break;

        case csspp::node_type_t::CLOSE_CURLYBRACKET:
            REQUIRE_ERRORS("test.css(1): error: node name \"CLOSE_CURLYBRACKET\".\n");
            break;

        case csspp::node_type_t::CLOSE_PARENTHESIS:
            REQUIRE_ERRORS("test.css(1): error: node name \"CLOSE_PARENTHESIS\".\n");
            break;

        case csspp::node_type_t::CLOSE_SQUAREBRACKET:
            REQUIRE_ERRORS("test.css(1): error: node name \"CLOSE_SQUAREBRACKET\".\n");
            break;

        case csspp::node_type_t::COLON:
            REQUIRE_ERRORS("test.css(1): error: node name \"COLON\".\n");
            break;

        case csspp::node_type_t::COLOR:
            REQUIRE_ERRORS("test.css(1): error: node name \"COLOR\".\n");
            break;

        case csspp::node_type_t::COLUMN:
            REQUIRE_ERRORS("test.css(1): error: node name \"COLUMN\".\n");
            break;

        case csspp::node_type_t::COMMA:
            REQUIRE_ERRORS("test.css(1): error: node name \"COMMA\".\n");
            break;

        case csspp::node_type_t::COMMENT:
            REQUIRE_ERRORS("test.css(1): error: node name \"COMMENT\".\n");
            break;

        case csspp::node_type_t::CONDITIONAL:
            REQUIRE_ERRORS("test.css(1): error: node name \"CONDITIONAL\".\n");
            break;

        case csspp::node_type_t::DASH_MATCH:
            REQUIRE_ERRORS("test.css(1): error: node name \"DASH_MATCH\".\n");
            break;

        case csspp::node_type_t::DECIMAL_NUMBER:
            REQUIRE_ERRORS("test.css(1): error: node name \"DECIMAL_NUMBER\".\n");
            break;

        case csspp::node_type_t::DIVIDE:
            REQUIRE_ERRORS("test.css(1): error: node name \"DIVIDE\".\n");
            break;

        case csspp::node_type_t::DOLLAR:
            REQUIRE_ERRORS("test.css(1): error: node name \"DOLLAR\".\n");
            break;

        case csspp::node_type_t::EOF_TOKEN:
            REQUIRE_ERRORS("test.css(1): error: node name \"EOF_TOKEN\".\n");
            break;

        case csspp::node_type_t::EQUAL:
            REQUIRE_ERRORS("test.css(1): error: node name \"EQUAL\".\n");
            break;

        case csspp::node_type_t::EXCLAMATION:
            REQUIRE_ERRORS("test.css(1): error: node name \"EXCLAMATION\".\n");
            break;

        case csspp::node_type_t::FONT_METRICS:
            REQUIRE_ERRORS("test.css(1): error: node name \"FONT_METRICS\".\n");
            break;

        case csspp::node_type_t::FUNCTION:
            REQUIRE_ERRORS("test.css(1): error: node name \"FUNCTION\".\n");
            break;

        case csspp::node_type_t::GREATER_EQUAL:
            REQUIRE_ERRORS("test.css(1): error: node name \"GREATER_EQUAL\".\n");
            break;

        case csspp::node_type_t::GREATER_THAN:
            REQUIRE_ERRORS("test.css(1): error: node name \"GREATER_THAN\".\n");
            break;

        case csspp::node_type_t::HASH:
            REQUIRE_ERRORS("test.css(1): error: node name \"HASH\".\n");
            break;

        case csspp::node_type_t::IDENTIFIER:
            REQUIRE_ERRORS("test.css(1): error: node name \"IDENTIFIER\".\n");
            break;

        case csspp::node_type_t::INCLUDE_MATCH:
            REQUIRE_ERRORS("test.css(1): error: node name \"INCLUDE_MATCH\".\n");
            break;

        case csspp::node_type_t::INTEGER:
            REQUIRE_ERRORS("test.css(1): error: node name \"INTEGER\".\n");
            break;

        case csspp::node_type_t::LESS_EQUAL:
            REQUIRE_ERRORS("test.css(1): error: node name \"LESS_EQUAL\".\n");
            break;

        case csspp::node_type_t::LESS_THAN:
            REQUIRE_ERRORS("test.css(1): error: node name \"LESS_THAN\".\n");
            break;

        case csspp::node_type_t::MODULO:
            REQUIRE_ERRORS("test.css(1): error: node name \"MODULO\".\n");
            break;

        case csspp::node_type_t::MULTIPLY:
            REQUIRE_ERRORS("test.css(1): error: node name \"MULTIPLY\".\n");
            break;

        case csspp::node_type_t::NOT_EQUAL:
            REQUIRE_ERRORS("test.css(1): error: node name \"NOT_EQUAL\".\n");
            break;

        case csspp::node_type_t::NULL_TOKEN:
            REQUIRE_ERRORS("test.css(1): error: node name \"NULL_TOKEN\".\n");
            break;

        case csspp::node_type_t::OPEN_CURLYBRACKET:
            REQUIRE_ERRORS("test.css(1): error: node name \"OPEN_CURLYBRACKET\".\n");
            break;

        case csspp::node_type_t::OPEN_PARENTHESIS:
            REQUIRE_ERRORS("test.css(1): error: node name \"OPEN_PARENTHESIS\".\n");
            break;

        case csspp::node_type_t::OPEN_SQUAREBRACKET:
            REQUIRE_ERRORS("test.css(1): error: node name \"OPEN_SQUAREBRACKET\".\n");
            break;

        case csspp::node_type_t::PERCENT:
            REQUIRE_ERRORS("test.css(1): error: node name \"PERCENT\".\n");
            break;

        case csspp::node_type_t::PERIOD:
            REQUIRE_ERRORS("test.css(1): error: node name \"PERIOD\".\n");
            break;

        case csspp::node_type_t::PLACEHOLDER:
            REQUIRE_ERRORS("test.css(1): error: node name \"PLACEHOLDER\".\n");
            break;

        case csspp::node_type_t::POWER:
            REQUIRE_ERRORS("test.css(1): error: node name \"POWER\".\n");
            break;

        case csspp::node_type_t::PRECEDED:
            REQUIRE_ERRORS("test.css(1): error: node name \"PRECEDED\".\n");
            break;

        case csspp::node_type_t::PREFIX_MATCH:
            REQUIRE_ERRORS("test.css(1): error: node name \"PREFIX_MATCH\".\n");
            break;

        case csspp::node_type_t::REFERENCE:
            REQUIRE_ERRORS("test.css(1): error: node name \"REFERENCE\".\n");
            break;

        case csspp::node_type_t::SCOPE:
            REQUIRE_ERRORS("test.css(1): error: node name \"SCOPE\".\n");
            break;

        case csspp::node_type_t::SEMICOLON:
            REQUIRE_ERRORS("test.css(1): error: node name \"SEMICOLON\".\n");
            break;

        case csspp::node_type_t::STRING:
            REQUIRE_ERRORS("test.css(1): error: node name \"STRING\".\n");
            break;

        case csspp::node_type_t::SUBSTRING_MATCH:
            REQUIRE_ERRORS("test.css(1): error: node name \"SUBSTRING_MATCH\".\n");
            break;

        case csspp::node_type_t::SUBTRACT:
            REQUIRE_ERRORS("test.css(1): error: node name \"SUBTRACT\".\n");
            break;

        case csspp::node_type_t::SUFFIX_MATCH:
            REQUIRE_ERRORS("test.css(1): error: node name \"SUFFIX_MATCH\".\n");
            break;

        case csspp::node_type_t::UNICODE_RANGE:
            REQUIRE_ERRORS("test.css(1): error: node name \"UNICODE_RANGE\".\n");
            break;

        case csspp::node_type_t::URL:
            REQUIRE_ERRORS("test.css(1): error: node name \"URL\".\n");
            break;

        case csspp::node_type_t::VARIABLE:
            REQUIRE_ERRORS("test.css(1): error: node name \"VARIABLE\".\n");
            break;

        case csspp::node_type_t::VARIABLE_FUNCTION:
            REQUIRE_ERRORS("test.css(1): error: node name \"VARIABLE_FUNCTION\".\n");
            break;

        case csspp::node_type_t::WHITESPACE:
            REQUIRE_ERRORS("test.css(1): error: node name \"WHITESPACE\".\n");
            break;

        // second part
        case csspp::node_type_t::AN_PLUS_B:
            REQUIRE_ERRORS("test.css(1): error: node name \"AN_PLUS_B\".\n");
            break;

        case csspp::node_type_t::ARG:
            REQUIRE_ERRORS("test.css(1): error: node name \"ARG\".\n");
            break;

        case csspp::node_type_t::ARRAY:
            REQUIRE_ERRORS("test.css(1): error: node name \"ARRAY\".\n");
            break;

        case csspp::node_type_t::COMPONENT_VALUE:
            REQUIRE_ERRORS("test.css(1): error: node name \"COMPONENT_VALUE\".\n");
            break;

        case csspp::node_type_t::DECLARATION:
            REQUIRE_ERRORS("test.css(1): error: node name \"DECLARATION\".\n");
            break;

        case csspp::node_type_t::LIST:
            REQUIRE_ERRORS("test.css(1): error: node name \"LIST\".\n");
            break;

        case csspp::node_type_t::MAP:
            REQUIRE_ERRORS("test.css(1): error: node name \"MAP\".\n");
            break;

        case csspp::node_type_t::max_type:
            REQUIRE_ERRORS("test.css(1): error: node name \"max_type\".\n");
            break;

        }

        // move to the next type
        w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Print nodes", "[node] [output]")
{
    // create a tree of nodes, then print it to exercise all the possible
    // cases of the display() function
    csspp::position pos("print.css");
    csspp::node::pointer_t root(new csspp::node(csspp::node_type_t::LIST, pos));

        csspp::node::pointer_t integer(new csspp::node(csspp::node_type_t::INTEGER, pos));
        integer->set_integer(123);
        root->add_child(integer);

        csspp::node::pointer_t string(new csspp::node(csspp::node_type_t::STRING, pos));
        string->set_string("bear");
        root->add_child(string);

        csspp::node::pointer_t decimal_number(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, pos));
        decimal_number->set_decimal_number(100.0);
        root->add_child(decimal_number);

        csspp::node::pointer_t child(new csspp::node(csspp::node_type_t::AT_KEYWORD, pos));
        child->set_string("@-char");
        child->set_flag("important", true);
        csspp::node::pointer_t var(new csspp::node(csspp::node_type_t::IDENTIFIER, pos));
        var->set_string("colorous");
        child->set_variable("test", var);
        root->add_child(child);

            csspp::node::pointer_t boolean(new csspp::node(csspp::node_type_t::BOOLEAN, pos));
            boolean->set_boolean(true);
            child->add_child(boolean);

            csspp::node::pointer_t integer2(new csspp::node(csspp::node_type_t::INTEGER, pos));
            integer2->set_integer(409);
            child->add_child(integer2);

            csspp::node::pointer_t string2(new csspp::node(csspp::node_type_t::STRING, pos));
            string2->set_string("rabbit");
            child->add_child(string2);

            csspp::node::pointer_t bracket(new csspp::node(csspp::node_type_t::OPEN_CURLYBRACKET, pos));
            bracket->set_boolean(true);
            child->add_child(bracket);

                csspp::node::pointer_t decimal_number2(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, pos));
                decimal_number2->set_decimal_number(208.0);
                bracket->add_child(decimal_number2);

                csspp::node::pointer_t an_plus_b(new csspp::node(csspp::node_type_t::AN_PLUS_B, pos));
                an_plus_b->set_integer(0x0000000700000003);
                bracket->add_child(an_plus_b);

                csspp::node::pointer_t color(new csspp::node(csspp::node_type_t::COLOR, pos));
                csspp::color c;
                c.set_color(0x11, 0x34, 0x78, 0xFF);
                color->set_color(c);
                bracket->add_child(color);

                csspp::node::pointer_t font_metrics(new csspp::node(csspp::node_type_t::FONT_METRICS, pos));
                font_metrics->set_font_size(3.45);
                font_metrics->set_dim1("cm");
                font_metrics->set_line_height(1.5);
                font_metrics->set_dim2("%");
                bracket->add_child(font_metrics);

    std::stringstream ss;
    ss << *root;

    REQUIRE_TREES(ss.str(),

"LIST\n"
"  INTEGER \"\" I:123\n"
"  STRING \"bear\"\n"
"  DECIMAL_NUMBER \"\" D:100\n"
"  AT_KEYWORD \"@-char\" I:0 F:important\n"
"      V:test\n"
"        IDENTIFIER \"colorous\"\n"
"    BOOLEAN B:true\n"
"    INTEGER \"\" I:409\n"
"    STRING \"rabbit\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECIMAL_NUMBER \"\" D:208\n"
"      AN_PLUS_B S:3n+7\n"
"      COLOR H:ff783411\n"
"      FONT_METRICS FM:3.45cm/150%\n"

        );

    // also test a clone() and see that the clone is a 1:1 an equivalent
    csspp::node::pointer_t p(root->clone());

    std::stringstream ss2;
    ss2 << *p;

    REQUIRE_TREES(ss2.str(),

"LIST\n"
"  INTEGER \"\" I:123\n"
"  STRING \"bear\"\n"
"  DECIMAL_NUMBER \"\" D:100\n"
"  AT_KEYWORD \"@-char\" I:0 F:important\n"
"      V:test\n"
"        IDENTIFIER \"colorous\"\n"
"    BOOLEAN B:true\n"
"    INTEGER \"\" I:409\n"
"    STRING \"rabbit\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECIMAL_NUMBER \"\" D:208\n"
"      AN_PLUS_B S:3n+7\n"
"      COLOR H:ff783411\n"
"      FONT_METRICS FM:3.45cm/150%\n"

        );

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Font metrics", "[node] [output]")
{
    // create a tree of nodes, then print it to exercise all the possible
    // cases of the display() function
    csspp::position pos("font-metrics.css");
    csspp::node::pointer_t font_metrics(new csspp::node(csspp::node_type_t::FONT_METRICS, pos));

    for(int i(0); i < 100; ++i)
    {
        csspp::decimal_number_t const size(static_cast<csspp::decimal_number_t>(rand() % 1000) / 10.0);
        font_metrics->set_font_size(size);
        REQUIRE((font_metrics->get_font_size() - size) < 0.0001);

        csspp::decimal_number_t const height(static_cast<csspp::decimal_number_t>(rand() % 1000) / 10.0);
        font_metrics->set_line_height(height);
        REQUIRE((font_metrics->get_line_height() - height) < 0.0001);

        // line height has no effect on the font size
        REQUIRE((font_metrics->get_font_size() - size) < 0.0001);

        // to see that font size has no effect on line height...
        csspp::decimal_number_t const size2(static_cast<csspp::decimal_number_t>(rand() % 1000) / 10.0);
        font_metrics->set_font_size(size2);
        REQUIRE((font_metrics->get_font_size() - size2) < 0.0001);

        REQUIRE((font_metrics->get_line_height() - height) < 0.0001);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
