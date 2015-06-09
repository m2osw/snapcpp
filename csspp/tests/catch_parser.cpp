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
 * \brief Test the parser.cpp file.
 *
 * This test runs a battery of tests agains the parser.cpp file to ensure
 * full coverage and many edge cases as expected by CSS 3.
 *
 * Note that the basic grammar that the parser implements is compatible
 * with CSS 1 and 2.1.
 *
 * Remember that the parser does not do any verification other than the
 * ability to parse the input data. So whether the rules are any good
 * is not known at the time the parser returns.
 */

#include "catch_tests.h"

#include "csspp/exceptions.h"
#include "csspp/parser.h"

#include <sstream>

#include <string.h>

namespace
{

} // no name namespace




TEST_CASE("Simple Stylesheets", "[parser] [stylesheet]")
{
    {
        std::stringstream ss;
        ss << "<!-- body { background : white url( /images/background.png ) } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE(out.str() ==

"LIST\n"
"  LIST\n"
"    IDENTIFIER \"body\"\n"
"    OPEN_CURLYBRACKET\n"
"      LIST\n"
"        IDENTIFIER \"background\"\n"
"        COLON\n"
"        IDENTIFIER \"white\"\n"
"        URL \"/images/background.png\"\n"


            );

        // no error left over
        csspp_test::trace_error::instance().expected_error("");
    }

    {
        std::stringstream ss;
        ss << "<!-- body { background : white url( /images/background.png ) } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE(out.str() ==

"LIST\n"
"  LIST\n"
"    IDENTIFIER \"body\"\n"
"    OPEN_CURLYBRACKET\n"
"      LIST\n"
"        IDENTIFIER \"background\"\n"
"        COLON\n"
"        IDENTIFIER \"white\"\n"
"        URL \"/images/background.png\"\n"


            );

        // no error left over
        csspp_test::trace_error::instance().expected_error("test.css(1): error: HTML comment delimiters (<!-- and -->) are not allowed in this CSS document.\n");
    }
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
