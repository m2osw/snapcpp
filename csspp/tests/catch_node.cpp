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


TEST_CASE("Get all the types", "[csspp]")
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
        case csspp::node_type_t::FUNCTION:
        case csspp::node_type_t::HASH:
        case csspp::node_type_t::IDENTIFIER:
        case csspp::node_type_t::INTEGER:
        case csspp::node_type_t::STRING:
        case csspp::node_type_t::URL:
            n->set_string("test-string");
            REQUIRE(n->get_string() == "test-string");
            break;

        default:
            REQUIRE_THROWS_AS(n->set_string("add"), csspp::csspp_exception_logic);
            REQUIRE_THROWS_AS(n->get_string(), csspp::csspp_exception_logic);
            break;

        }

        // move to the next type
        w = static_cast<csspp::node_type_t>(static_cast<int>(w) + 1);
    }

    // no error left over
    csspp_test::trace_error::instance().expected_error("");
}

TEST_CASE("Verify type names", "[csspp]")
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

        case csspp::node_type_t::PREFIX_MATCH:
            REQUIRE(name == "PREFIX_MATCH");
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

        case csspp::node_type_t::WHITESPACE:
            REQUIRE(name == "WHITESPACE");
            break;

        case csspp::node_type_t::CHARSET:
            REQUIRE(name == "CHARSET");
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
    csspp_test::trace_error::instance().expected_error("");
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
