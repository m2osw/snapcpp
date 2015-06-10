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

        // integer
        switch(w)
        {
        case csspp::node_type_t::COMMENT:
        case csspp::node_type_t::INTEGER:
        case csspp::node_type_t::UNICODE_RANGE:
            n->set_integer(123);
            REQUIRE(n->get_integer() == 123);
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
            n->set_decimal_number(123.456);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            REQUIRE(n->get_decimal_number() == 123.456);
#pragma GCC diagnostic pop
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
        case csspp::node_type_t::STRING:
        case csspp::node_type_t::URL:
        case csspp::node_type_t::VARIABLE:
            n->set_string("test-string");
            REQUIRE(n->get_string() == "test-string");
            break;

        default:
            REQUIRE_THROWS_AS(n->set_string("add"), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_string(), csspp::csspp_exception_logic);
            break;

        }

        // children
        switch(w)
        {
        case csspp::node_type_t::AT_KEYWORD:
        case csspp::node_type_t::COMPONENT_VALUE:
        case csspp::node_type_t::DECLARATION:
        case csspp::node_type_t::FUNCTION:
        case csspp::node_type_t::LIST:
        case csspp::node_type_t::OPEN_CURLYBRACKET:
        case csspp::node_type_t::OPEN_PARENTHESIS:
        case csspp::node_type_t::OPEN_SQUAREBRACKET:
            {
                // try adding one child
                REQUIRE(n->empty());
                REQUIRE(n->size() == 0);
                REQUIRE_THROWS_AS(n->get_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->get_last_child(), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);

                csspp::node::pointer_t child1(new csspp::node(csspp::node_type_t::COMMA, n->get_position()));
                csspp::node::pointer_t child2(new csspp::node(csspp::node_type_t::EXCLAMATION, n->get_position()));

                n->add_child(child1);
                REQUIRE(n->size() == 1);
                REQUIRE_FALSE(n->empty());
                REQUIRE(n->get_last_child() == child1);
                REQUIRE(n->get_child(0) == child1);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);

                n->add_child(child2);
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
                REQUIRE(n->empty());
                REQUIRE(n->size() == 0);
                REQUIRE_THROWS_AS(n->get_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->get_last_child(), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(0), csspp::csspp_exception_overflow);
                REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);
            }
            break;

        default:
            REQUIRE_THROWS_AS(n->empty(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->size(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->add_child(n), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->remove_child(n), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->remove_child(0), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_child(0), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_last_child(), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->take_over_children_of(0), csspp::csspp_exception_logic);
            break;

        }

        // move to the next type
        w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
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

        case csspp::node_type_t::AT_KEYWORD:
            REQUIRE(name == "AT_KEYWORD");
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

        case csspp::node_type_t::COLUMN:
            REQUIRE(name == "COLUMN");
            break;

        case csspp::node_type_t::COMMA:
            REQUIRE(name == "COMMA");
            break;

        case csspp::node_type_t::COMMENT:
            REQUIRE(name == "COMMENT");
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

        case csspp::node_type_t::FUNCTION:
            REQUIRE(name == "FUNCTION");
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

        case csspp::node_type_t::MULTIPLY:
            REQUIRE(name == "MULTIPLY");
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

        case csspp::node_type_t::WHITESPACE:
            REQUIRE(name == "WHITESPACE");
            break;

        case csspp::node_type_t::CHARSET:
            REQUIRE(name == "CHARSET");
            break;

        case csspp::node_type_t::COMPONENT_VALUE:
            REQUIRE(name == "COMPONENT_VALUE");
            break;

        case csspp::node_type_t::DECLARATION:
            REQUIRE(name == "DECLARATION");
            break;

        case csspp::node_type_t::FONTFACE:
            REQUIRE(name == "FONTFACE");
            break;

        case csspp::node_type_t::KEYFRAME:
            REQUIRE(name == "KEYFRAME");
            break;

        case csspp::node_type_t::KEYFRAMES:
            REQUIRE(name == "KEYFRAMES");
            break;

        case csspp::node_type_t::LIST:
            REQUIRE(name == "LIST");
            break;

        case csspp::node_type_t::MEDIA:
            REQUIRE(name == "MEDIA");
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

        case csspp::node_type_t::AT_KEYWORD:
            REQUIRE(name == "AT_KEYWORD");
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

        case csspp::node_type_t::COLUMN:
            REQUIRE(name == "COLUMN");
            break;

        case csspp::node_type_t::COMMA:
            REQUIRE(name == "COMMA");
            break;

        case csspp::node_type_t::COMMENT:
            REQUIRE(name == "COMMENT");
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

        case csspp::node_type_t::FUNCTION:
            REQUIRE(name == "FUNCTION");
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

        case csspp::node_type_t::MULTIPLY:
            REQUIRE(name == "MULTIPLY");
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

        case csspp::node_type_t::WHITESPACE:
            REQUIRE(name == "WHITESPACE");
            break;

        case csspp::node_type_t::CHARSET:
            REQUIRE(name == "CHARSET");
            break;

        case csspp::node_type_t::COMPONENT_VALUE:
            REQUIRE(name == "COMPONENT_VALUE");
            break;

        case csspp::node_type_t::DECLARATION:
            REQUIRE(name == "DECLARATION");
            break;

        case csspp::node_type_t::FONTFACE:
            REQUIRE(name == "FONTFACE");
            break;

        case csspp::node_type_t::KEYFRAME:
            REQUIRE(name == "KEYFRAME");
            break;

        case csspp::node_type_t::KEYFRAMES:
            REQUIRE(name == "KEYFRAMES");
            break;

        case csspp::node_type_t::LIST:
            REQUIRE(name == "LIST");
            break;

        case csspp::node_type_t::MEDIA:
            REQUIRE(name == "MEDIA");
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

        case csspp::node_type_t::AT_KEYWORD:
            REQUIRE_ERRORS("test.css(1): error: node name \"AT_KEYWORD\".\n");
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

        case csspp::node_type_t::COLUMN:
            REQUIRE_ERRORS("test.css(1): error: node name \"COLUMN\".\n");
            break;

        case csspp::node_type_t::COMMA:
            REQUIRE_ERRORS("test.css(1): error: node name \"COMMA\".\n");
            break;

        case csspp::node_type_t::COMMENT:
            REQUIRE_ERRORS("test.css(1): error: node name \"COMMENT\".\n");
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

        case csspp::node_type_t::FUNCTION:
            REQUIRE_ERRORS("test.css(1): error: node name \"FUNCTION\".\n");
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

        case csspp::node_type_t::MULTIPLY:
            REQUIRE_ERRORS("test.css(1): error: node name \"MULTIPLY\".\n");
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

        case csspp::node_type_t::WHITESPACE:
            REQUIRE_ERRORS("test.css(1): error: node name \"WHITESPACE\".\n");
            break;

        case csspp::node_type_t::CHARSET:
            REQUIRE_ERRORS("test.css(1): error: node name \"CHARSET\".\n");
            break;

        case csspp::node_type_t::COMPONENT_VALUE:
            REQUIRE_ERRORS("test.css(1): error: node name \"COMPONENT_VALUE\".\n");
            break;

        case csspp::node_type_t::DECLARATION:
            REQUIRE_ERRORS("test.css(1): error: node name \"DECLARATION\".\n");
            break;

        case csspp::node_type_t::FONTFACE:
            REQUIRE_ERRORS("test.css(1): error: node name \"FONTFACE\".\n");
            break;

        case csspp::node_type_t::KEYFRAME:
            REQUIRE_ERRORS("test.css(1): error: node name \"KEYFRAME\".\n");
            break;

        case csspp::node_type_t::KEYFRAMES:
            REQUIRE_ERRORS("test.css(1): error: node name \"KEYFRAMES\".\n");
            break;

        case csspp::node_type_t::LIST:
            REQUIRE_ERRORS("test.css(1): error: node name \"LIST\".\n");
            break;

        case csspp::node_type_t::MEDIA:
            REQUIRE_ERRORS("test.css(1): error: node name \"MEDIA\".\n");
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
        root->add_child(child);

            csspp::node::pointer_t integer2(new csspp::node(csspp::node_type_t::INTEGER, pos));
            integer2->set_integer(409);
            child->add_child(integer2);

            csspp::node::pointer_t string2(new csspp::node(csspp::node_type_t::STRING, pos));
            string2->set_string("rabbit");
            child->add_child(string2);

            csspp::node::pointer_t decimal_number2(new csspp::node(csspp::node_type_t::DECIMAL_NUMBER, pos));
            decimal_number2->set_decimal_number(208.0);
            child->add_child(decimal_number2);

    std::stringstream ss;
    ss << *root;

    REQUIRE_TREES(ss.str(),

"LIST\n"
"  INTEGER \"\" I:123\n"
"  STRING \"bear\"\n"
"  DECIMAL_NUMBER \"\" D:100\n"
"  AT_KEYWORD \"@-char\"\n"
"    INTEGER \"\" I:409\n"
"    STRING \"rabbit\"\n"
"    DECIMAL_NUMBER \"\" D:208\n"

        );

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
