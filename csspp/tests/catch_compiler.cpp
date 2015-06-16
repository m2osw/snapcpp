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
 * \brief Test the compiler.cpp file.
 *
 * This test runs a battery of tests agains the compiler.cpp file to ensure
 * full coverage and many edge cases as expected by CSS 3 and many of the
 * CSS Preprocessor extensions.
 */

#include "catch_tests.h"

#include "csspp/exceptions.h"
#include "csspp/compiler.h"
#include "csspp/parser.h"

#include <sstream>

#include <string.h>

namespace
{

} // no name namespace




TEST_CASE("Compile Simple Stylesheets", "[compiler] [stylesheet]")
{
    // with many spaces
    {
        std::stringstream ss;
        ss << "/* testing compile */"
           << "body, a[q] > b[p=\"344.5\"] + c[z=33] ~ d[e], html *[ ff = fire ] *.blue { background : white url( /images/background.png ) }"
           << "/* @preserver test \"Compile Simple Stylesheet\" */";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"body\"\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"q\"\n"
"      GREATER_THAN\n"
"      IDENTIFIER \"b\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"p\"\n"
"        EQUAL\n"
"        STRING \"344.5\"\n"
"      ADD\n"
"      IDENTIFIER \"c\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"z\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:33\n"
"      PRECEDED\n"
"      IDENTIFIER \"d\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"e\"\n"
"    ARG\n"
"      IDENTIFIER \"html\"\n"
"      WHITESPACE\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"ff\"\n"
"        EQUAL\n"
"        IDENTIFIER \"fire\"\n"
"      WHITESPACE\n"
"      PERIOD\n"
"      IDENTIFIER \"blue\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"background\"\n"
"        IDENTIFIER \"white\"\n"
"        WHITESPACE\n"
"        URL \"/images/background.png\"\n"
"  COMMENT \"@preserver test \"Compile Simple Stylesheet\"\" I:1\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // without spaces
    {
        std::stringstream ss;
        ss << "/* testing compile */"
           << "body,a[q]>b[p=\"344.5\"]+c[z=33]~d[e],html *[ff=fire] *.blue { background:white url(/images/background.png) }"
           << "/* @preserver test \"Compile Simple Stylesheet\" */";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"body\"\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"q\"\n"
"      GREATER_THAN\n"
"      IDENTIFIER \"b\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"p\"\n"
"        EQUAL\n"
"        STRING \"344.5\"\n"
"      ADD\n"
"      IDENTIFIER \"c\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"z\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:33\n"
"      PRECEDED\n"
"      IDENTIFIER \"d\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"e\"\n"
"    ARG\n"
"      IDENTIFIER \"html\"\n"
"      WHITESPACE\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"ff\"\n"
"        EQUAL\n"
"        IDENTIFIER \"fire\"\n"
"      WHITESPACE\n"
"      PERIOD\n"
"      IDENTIFIER \"blue\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"background\"\n"
"        IDENTIFIER \"white\"\n"
"        WHITESPACE\n"
"        URL \"/images/background.png\"\n"
"  COMMENT \"@preserver test \"Compile Simple Stylesheet\"\" I:1\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Check All Argify", "[compiler] [stylesheet]")
{
    {
        std::stringstream ss;
        ss << "a,b{color:red}\n"
           << "a, b{color:red}\n"
           << "a,b ,c{color:red}\n"
           << "a , b,c{color:red}\n"
           << "a{color:red}\n"
           << "a {color:red}\n"
           << "a,b {color:red}\n"
           ;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    ARG\n"
"      IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    ARG\n"
"      IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Arguments", "[compiler] [stylesheet]")
{
    // A starting comma is illegal
    {
        std::stringstream ss;
        ss << ",a{color:red}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        // no error left over
        REQUIRE_ERRORS("test.css(1): error: dangling comma at the beginning of a list of arguments or selectors.\n");
    }

    // An ending comma is illegal
    {
        std::stringstream ss;
        ss << "a,{color:red}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        // no error left over
        REQUIRE_ERRORS("test.css(1): error: dangling comma at the end of a list of arguments or selectors.\n");
    }

    // Two commas in a row is illegal
    {
        std::stringstream ss;
        ss << "a,,b{color:red}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        // no error left over
        REQUIRE_ERRORS("test.css(1): error: two commas in a row are invalid in a list of arguments or selectors.\n");
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Selector Attribute Tests", "[compiler] [stylesheet]")
{
    // TODO: rewrite that one to use a few less lines
    {
        std::stringstream ss;
        ss << "a[b]{color:red}\n"
           << "a[b ]{color:red}\n"
           << "a[ b]{color:red}\n"
           << "a[ b ]{color:red}\n"

           << "a[b=c]{color:red}\n"
           << "a[b=c ]{color:red}\n"
           << "a[b= c]{color:red}\n"
           << "a[b= c ]{color:red}\n"
           << "a[b =c]{color:red}\n"
           << "a[b =c ]{color:red}\n"
           << "a[b = c]{color:red}\n"
           << "a[b = c ]{color:red}\n"
           << "a[ b=c]{color:red}\n"
           << "a[ b=c ]{color:red}\n"
           << "a[ b= c]{color:red}\n"
           << "a[ b= c ]{color:red}\n"
           << "a[ b =c]{color:red}\n"
           << "a[ b =c ]{color:red}\n"
           << "a[ b = c]{color:red}\n"
           << "a[ b = c ]{color:red}\n"

           << "a[b=' c ']{color:red}\n"
           << "a[b=' c ' ]{color:red}\n"
           << "a[b= ' c ']{color:red}\n"
           << "a[b= ' c ' ]{color:red}\n"
           << "a[b =' c ']{color:red}\n"
           << "a[b =' c ' ]{color:red}\n"
           << "a[b = ' c ']{color:red}\n"
           << "a[b = ' c ' ]{color:red}\n"
           << "a[ b=' c ']{color:red}\n"
           << "a[ b=' c ' ]{color:red}\n"
           << "a[ b= ' c ']{color:red}\n"
           << "a[ b= ' c ' ]{color:red}\n"
           << "a[ b =' c ']{color:red}\n"
           << "a[ b =' c ' ]{color:red}\n"
           << "a[ b = ' c ']{color:red}\n"
           << "a[ b = ' c ' ]{color:red}\n"

           << "a[b=123]{color:red}\n"
           << "a[b=123 ]{color:red}\n"
           << "a[b= 123]{color:red}\n"
           << "a[b= 123 ]{color:red}\n"
           << "a[b =123]{color:red}\n"
           << "a[b =123 ]{color:red}\n"
           << "a[b = 123]{color:red}\n"
           << "a[b = 123 ]{color:red}\n"
           << "a[ b=123]{color:red}\n"
           << "a[ b=123 ]{color:red}\n"
           << "a[ b= 123]{color:red}\n"
           << "a[ b= 123 ]{color:red}\n"
           << "a[ b =123]{color:red}\n"
           << "a[ b =123 ]{color:red}\n"
           << "a[ b = 123]{color:red}\n"
           << "a[ b = 123 ]{color:red}\n"

           << "a[b=1.23]{color:red}\n"
           << "a[b=1.23 ]{color:red}\n"
           << "a[b= 1.23]{color:red}\n"
           << "a[b= 1.23 ]{color:red}\n"
           << "a[b =1.23]{color:red}\n"
           << "a[b =1.23 ]{color:red}\n"
           << "a[b = 1.23]{color:red}\n"
           << "a[b = 1.23 ]{color:red}\n"
           << "a[ b=1.23]{color:red}\n"
           << "a[ b=1.23 ]{color:red}\n"
           << "a[ b= 1.23]{color:red}\n"
           << "a[ b= 1.23 ]{color:red}\n"
           << "a[ b =1.23]{color:red}\n"
           << "a[ b =1.23 ]{color:red}\n"
           << "a[ b = 1.23]{color:red}\n"
           << "a[ b = 1.23 ]{color:red}\n"

           << "a[b~=c]{color:red}\n"
           << "a[b~=c ]{color:red}\n"
           << "a[b~= c]{color:red}\n"
           << "a[b~= c ]{color:red}\n"
           << "a[b ~=c]{color:red}\n"
           << "a[b ~=c ]{color:red}\n"
           << "a[b ~= c]{color:red}\n"
           << "a[b ~= c ]{color:red}\n"
           << "a[ b~=c]{color:red}\n"
           << "a[ b~=c ]{color:red}\n"
           << "a[ b~= c]{color:red}\n"
           << "a[ b~= c ]{color:red}\n"
           << "a[ b ~=c]{color:red}\n"
           << "a[ b ~=c ]{color:red}\n"
           << "a[ b ~= c]{color:red}\n"
           << "a[ b ~= c ]{color:red}\n"

           << "a[b~=' c ']{color:red}\n"
           << "a[b~=' c ' ]{color:red}\n"
           << "a[b~= ' c ']{color:red}\n"
           << "a[b~= ' c ' ]{color:red}\n"
           << "a[b ~=' c ']{color:red}\n"
           << "a[b ~=' c ' ]{color:red}\n"
           << "a[b ~= ' c ']{color:red}\n"
           << "a[b ~= ' c ' ]{color:red}\n"
           << "a[ b~=' c ']{color:red}\n"
           << "a[ b~=' c ' ]{color:red}\n"
           << "a[ b~= ' c ']{color:red}\n"
           << "a[ b~= ' c ' ]{color:red}\n"
           << "a[ b ~=' c ']{color:red}\n"
           << "a[ b ~=' c ' ]{color:red}\n"
           << "a[ b ~= ' c ']{color:red}\n"
           << "a[ b ~= ' c ' ]{color:red}\n"

           << "a[b~=123]{color:red}\n"
           << "a[b~=123 ]{color:red}\n"
           << "a[b~= 123]{color:red}\n"
           << "a[b~= 123 ]{color:red}\n"
           << "a[b ~=123]{color:red}\n"
           << "a[b ~=123 ]{color:red}\n"
           << "a[b ~= 123]{color:red}\n"
           << "a[b ~= 123 ]{color:red}\n"
           << "a[ b~=123]{color:red}\n"
           << "a[ b~=123 ]{color:red}\n"
           << "a[ b~= 123]{color:red}\n"
           << "a[ b~= 123 ]{color:red}\n"
           << "a[ b ~=123]{color:red}\n"
           << "a[ b ~=123 ]{color:red}\n"
           << "a[ b ~= 123]{color:red}\n"
           << "a[ b ~= 123 ]{color:red}\n"

           << "a[b~=1.23]{color:red}\n"
           << "a[b~=1.23 ]{color:red}\n"
           << "a[b~= 1.23]{color:red}\n"
           << "a[b~= 1.23 ]{color:red}\n"
           << "a[b ~=1.23]{color:red}\n"
           << "a[b ~=1.23 ]{color:red}\n"
           << "a[b ~= 1.23]{color:red}\n"
           << "a[b ~= 1.23 ]{color:red}\n"
           << "a[ b~=1.23]{color:red}\n"
           << "a[ b~=1.23 ]{color:red}\n"
           << "a[ b~= 1.23]{color:red}\n"
           << "a[ b~= 1.23 ]{color:red}\n"
           << "a[ b ~=1.23]{color:red}\n"
           << "a[ b ~=1.23 ]{color:red}\n"
           << "a[ b ~= 1.23]{color:red}\n"
           << "a[ b ~= 1.23 ]{color:red}\n"

           << "a[b^=c]{color:red}\n"
           << "a[b^=c ]{color:red}\n"
           << "a[b^= c]{color:red}\n"
           << "a[b^= c ]{color:red}\n"
           << "a[b ^=c]{color:red}\n"
           << "a[b ^=c ]{color:red}\n"
           << "a[b ^= c]{color:red}\n"
           << "a[b ^= c ]{color:red}\n"
           << "a[ b^=c]{color:red}\n"
           << "a[ b^=c ]{color:red}\n"
           << "a[ b^= c]{color:red}\n"
           << "a[ b^= c ]{color:red}\n"
           << "a[ b ^=c]{color:red}\n"
           << "a[ b ^=c ]{color:red}\n"
           << "a[ b ^= c]{color:red}\n"
           << "a[ b ^= c ]{color:red}\n"

           << "a[b^=' c ']{color:red}\n"
           << "a[b^=' c ' ]{color:red}\n"
           << "a[b^= ' c ']{color:red}\n"
           << "a[b^= ' c ' ]{color:red}\n"
           << "a[b ^=' c ']{color:red}\n"
           << "a[b ^=' c ' ]{color:red}\n"
           << "a[b ^= ' c ']{color:red}\n"
           << "a[b ^= ' c ' ]{color:red}\n"
           << "a[ b^=' c ']{color:red}\n"
           << "a[ b^=' c ' ]{color:red}\n"
           << "a[ b^= ' c ']{color:red}\n"
           << "a[ b^= ' c ' ]{color:red}\n"
           << "a[ b ^=' c ']{color:red}\n"
           << "a[ b ^=' c ' ]{color:red}\n"
           << "a[ b ^= ' c ']{color:red}\n"
           << "a[ b ^= ' c ' ]{color:red}\n"

           << "a[b^=123]{color:red}\n"
           << "a[b^=123 ]{color:red}\n"
           << "a[b^= 123]{color:red}\n"
           << "a[b^= 123 ]{color:red}\n"
           << "a[b ^=123]{color:red}\n"
           << "a[b ^=123 ]{color:red}\n"
           << "a[b ^= 123]{color:red}\n"
           << "a[b ^= 123 ]{color:red}\n"
           << "a[ b^=123]{color:red}\n"
           << "a[ b^=123 ]{color:red}\n"
           << "a[ b^= 123]{color:red}\n"
           << "a[ b^= 123 ]{color:red}\n"
           << "a[ b ^=123]{color:red}\n"
           << "a[ b ^=123 ]{color:red}\n"
           << "a[ b ^= 123]{color:red}\n"
           << "a[ b ^= 123 ]{color:red}\n"

           << "a[b^=1.23]{color:red}\n"
           << "a[b^=1.23 ]{color:red}\n"
           << "a[b^= 1.23]{color:red}\n"
           << "a[b^= 1.23 ]{color:red}\n"
           << "a[b ^=1.23]{color:red}\n"
           << "a[b ^=1.23 ]{color:red}\n"
           << "a[b ^= 1.23]{color:red}\n"
           << "a[b ^= 1.23 ]{color:red}\n"
           << "a[ b^=1.23]{color:red}\n"
           << "a[ b^=1.23 ]{color:red}\n"
           << "a[ b^= 1.23]{color:red}\n"
           << "a[ b^= 1.23 ]{color:red}\n"
           << "a[ b ^=1.23]{color:red}\n"
           << "a[ b ^=1.23 ]{color:red}\n"
           << "a[ b ^= 1.23]{color:red}\n"
           << "a[ b ^= 1.23 ]{color:red}\n"

           << "a[b$=c]{color:red}\n"
           << "a[b$=c ]{color:red}\n"
           << "a[b$= c]{color:red}\n"
           << "a[b$= c ]{color:red}\n"
           << "a[b $=c]{color:red}\n"
           << "a[b $=c ]{color:red}\n"
           << "a[b $= c]{color:red}\n"
           << "a[b $= c ]{color:red}\n"
           << "a[ b$=c]{color:red}\n"
           << "a[ b$=c ]{color:red}\n"
           << "a[ b$= c]{color:red}\n"
           << "a[ b$= c ]{color:red}\n"
           << "a[ b $=c]{color:red}\n"
           << "a[ b $=c ]{color:red}\n"
           << "a[ b $= c]{color:red}\n"
           << "a[ b $= c ]{color:red}\n"

           << "a[b$=' c ']{color:red}\n"
           << "a[b$=' c ' ]{color:red}\n"
           << "a[b$= ' c ']{color:red}\n"
           << "a[b$= ' c ' ]{color:red}\n"
           << "a[b $=' c ']{color:red}\n"
           << "a[b $=' c ' ]{color:red}\n"
           << "a[b $= ' c ']{color:red}\n"
           << "a[b $= ' c ' ]{color:red}\n"
           << "a[ b$=' c ']{color:red}\n"
           << "a[ b$=' c ' ]{color:red}\n"
           << "a[ b$= ' c ']{color:red}\n"
           << "a[ b$= ' c ' ]{color:red}\n"
           << "a[ b $=' c ']{color:red}\n"
           << "a[ b $=' c ' ]{color:red}\n"
           << "a[ b $= ' c ']{color:red}\n"
           << "a[ b $= ' c ' ]{color:red}\n"

           << "a[b$=123]{color:red}\n"
           << "a[b$=123 ]{color:red}\n"
           << "a[b$= 123]{color:red}\n"
           << "a[b$= 123 ]{color:red}\n"
           << "a[b $=123]{color:red}\n"
           << "a[b $=123 ]{color:red}\n"
           << "a[b $= 123]{color:red}\n"
           << "a[b $= 123 ]{color:red}\n"
           << "a[ b$=123]{color:red}\n"
           << "a[ b$=123 ]{color:red}\n"
           << "a[ b$= 123]{color:red}\n"
           << "a[ b$= 123 ]{color:red}\n"
           << "a[ b $=123]{color:red}\n"
           << "a[ b $=123 ]{color:red}\n"
           << "a[ b $= 123]{color:red}\n"
           << "a[ b $= 123 ]{color:red}\n"

           << "a[b$=1.23]{color:red}\n"
           << "a[b$=1.23 ]{color:red}\n"
           << "a[b$= 1.23]{color:red}\n"
           << "a[b$= 1.23 ]{color:red}\n"
           << "a[b $=1.23]{color:red}\n"
           << "a[b $=1.23 ]{color:red}\n"
           << "a[b $= 1.23]{color:red}\n"
           << "a[b $= 1.23 ]{color:red}\n"
           << "a[ b$=1.23]{color:red}\n"
           << "a[ b$=1.23 ]{color:red}\n"
           << "a[ b$= 1.23]{color:red}\n"
           << "a[ b$= 1.23 ]{color:red}\n"
           << "a[ b $=1.23]{color:red}\n"
           << "a[ b $=1.23 ]{color:red}\n"
           << "a[ b $= 1.23]{color:red}\n"
           << "a[ b $= 1.23 ]{color:red}\n"

           << "a[b*=c]{color:red}\n"
           << "a[b*=c ]{color:red}\n"
           << "a[b*= c]{color:red}\n"
           << "a[b*= c ]{color:red}\n"
           << "a[b *=c]{color:red}\n"
           << "a[b *=c ]{color:red}\n"
           << "a[b *= c]{color:red}\n"
           << "a[b *= c ]{color:red}\n"
           << "a[ b*=c]{color:red}\n"
           << "a[ b*=c ]{color:red}\n"
           << "a[ b*= c]{color:red}\n"
           << "a[ b*= c ]{color:red}\n"
           << "a[ b *=c]{color:red}\n"
           << "a[ b *=c ]{color:red}\n"
           << "a[ b *= c]{color:red}\n"
           << "a[ b *= c ]{color:red}\n"

           << "a[b*=' c ']{color:red}\n"
           << "a[b*=' c ' ]{color:red}\n"
           << "a[b*= ' c ']{color:red}\n"
           << "a[b*= ' c ' ]{color:red}\n"
           << "a[b *=' c ']{color:red}\n"
           << "a[b *=' c ' ]{color:red}\n"
           << "a[b *= ' c ']{color:red}\n"
           << "a[b *= ' c ' ]{color:red}\n"
           << "a[ b*=' c ']{color:red}\n"
           << "a[ b*=' c ' ]{color:red}\n"
           << "a[ b*= ' c ']{color:red}\n"
           << "a[ b*= ' c ' ]{color:red}\n"
           << "a[ b *=' c ']{color:red}\n"
           << "a[ b *=' c ' ]{color:red}\n"
           << "a[ b *= ' c ']{color:red}\n"
           << "a[ b *= ' c ' ]{color:red}\n"

           << "a[b*=123]{color:red}\n"
           << "a[b*=123 ]{color:red}\n"
           << "a[b*= 123]{color:red}\n"
           << "a[b*= 123 ]{color:red}\n"
           << "a[b *=123]{color:red}\n"
           << "a[b *=123 ]{color:red}\n"
           << "a[b *= 123]{color:red}\n"
           << "a[b *= 123 ]{color:red}\n"
           << "a[ b*=123]{color:red}\n"
           << "a[ b*=123 ]{color:red}\n"
           << "a[ b*= 123]{color:red}\n"
           << "a[ b*= 123 ]{color:red}\n"
           << "a[ b *=123]{color:red}\n"
           << "a[ b *=123 ]{color:red}\n"
           << "a[ b *= 123]{color:red}\n"
           << "a[ b *= 123 ]{color:red}\n"

           << "a[b*=1.23]{color:red}\n"
           << "a[b*=1.23 ]{color:red}\n"
           << "a[b*= 1.23]{color:red}\n"
           << "a[b*= 1.23 ]{color:red}\n"
           << "a[b *=1.23]{color:red}\n"
           << "a[b *=1.23 ]{color:red}\n"
           << "a[b *= 1.23]{color:red}\n"
           << "a[b *= 1.23 ]{color:red}\n"
           << "a[ b*=1.23]{color:red}\n"
           << "a[ b*=1.23 ]{color:red}\n"
           << "a[ b*= 1.23]{color:red}\n"
           << "a[ b*= 1.23 ]{color:red}\n"
           << "a[ b *=1.23]{color:red}\n"
           << "a[ b *=1.23 ]{color:red}\n"
           << "a[ b *= 1.23]{color:red}\n"
           << "a[ b *= 1.23 ]{color:red}\n"

           << "a[b|=c]{color:red}\n"
           << "a[b|=c ]{color:red}\n"
           << "a[b|= c]{color:red}\n"
           << "a[b|= c ]{color:red}\n"
           << "a[b |=c]{color:red}\n"
           << "a[b |=c ]{color:red}\n"
           << "a[b |= c]{color:red}\n"
           << "a[b |= c ]{color:red}\n"
           << "a[ b|=c]{color:red}\n"
           << "a[ b|=c ]{color:red}\n"
           << "a[ b|= c]{color:red}\n"
           << "a[ b|= c ]{color:red}\n"
           << "a[ b |=c]{color:red}\n"
           << "a[ b |=c ]{color:red}\n"
           << "a[ b |= c]{color:red}\n"
           << "a[ b |= c ]{color:red}\n"

           << "a[b|=' c ']{color:red}\n"
           << "a[b|=' c ' ]{color:red}\n"
           << "a[b|= ' c ']{color:red}\n"
           << "a[b|= ' c ' ]{color:red}\n"
           << "a[b |=' c ']{color:red}\n"
           << "a[b |=' c ' ]{color:red}\n"
           << "a[b |= ' c ']{color:red}\n"
           << "a[b |= ' c ' ]{color:red}\n"
           << "a[ b|=' c ']{color:red}\n"
           << "a[ b|=' c ' ]{color:red}\n"
           << "a[ b|= ' c ']{color:red}\n"
           << "a[ b|= ' c ' ]{color:red}\n"
           << "a[ b |=' c ']{color:red}\n"
           << "a[ b |=' c ' ]{color:red}\n"
           << "a[ b |= ' c ']{color:red}\n"
           << "a[ b |= ' c ' ]{color:red}\n"

           << "a[b|=123]{color:red}\n"
           << "a[b|=123 ]{color:red}\n"
           << "a[b|= 123]{color:red}\n"
           << "a[b|= 123 ]{color:red}\n"
           << "a[b |=123]{color:red}\n"
           << "a[b |=123 ]{color:red}\n"
           << "a[b |= 123]{color:red}\n"
           << "a[b |= 123 ]{color:red}\n"
           << "a[ b|=123]{color:red}\n"
           << "a[ b|=123 ]{color:red}\n"
           << "a[ b|= 123]{color:red}\n"
           << "a[ b|= 123 ]{color:red}\n"
           << "a[ b |=123]{color:red}\n"
           << "a[ b |=123 ]{color:red}\n"
           << "a[ b |= 123]{color:red}\n"
           << "a[ b |= 123 ]{color:red}\n"

           << "a[b|=1.23]{color:red}\n"
           << "a[b|=1.23 ]{color:red}\n"
           << "a[b|= 1.23]{color:red}\n"
           << "a[b|= 1.23 ]{color:red}\n"
           << "a[b |=1.23]{color:red}\n"
           << "a[b |=1.23 ]{color:red}\n"
           << "a[b |= 1.23]{color:red}\n"
           << "a[b |= 1.23 ]{color:red}\n"
           << "a[ b|=1.23]{color:red}\n"
           << "a[ b|=1.23 ]{color:red}\n"
           << "a[ b|= 1.23]{color:red}\n"
           << "a[ b|= 1.23 ]{color:red}\n"
           << "a[ b |=1.23]{color:red}\n"
           << "a[ b |=1.23 ]{color:red}\n"
           << "a[ b |= 1.23]{color:red}\n"
           << "a[ b |= 1.23 ]{color:red}\n"

           ;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        EQUAL\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        INCLUDE_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        PREFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUFFIX_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        SUBSTRING_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        IDENTIFIER \"c\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        STRING \" c \"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        INTEGER \"\" I:123\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"a\"\n"
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"b\"\n"
"        DASH_MATCH\n"
"        DECIMAL_NUMBER \"\" D:1.23\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Attributes", "[compiler] [stylesheet]")
{
    // attribute name cannot be an integer, decimal number, opening
    // brackets or parenthesis, delimiter, etc. only an identifier
    {
        char const * invalid_value[] =
        {
            "123",
            "1.23",
            "'1.23'",
            "1.23%",
            "(b)",
            "[b]",
            "{b}",
            "+b",
            //"@b",
            //"<!--",
            //"-->",
            //")",
            //"}",
            ",b,",
            "/* @preserve this comment */",
            "|=b",
            "/b",
            "$ b",
            "=b",
            "!b",
            "b(1)",
            ">b",
            "#123",
            "~=b",
            "*b",
            ".top",
            "%name",
            "~b",
            "&b",
            "|b",
            //";b",
        };

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[" << iv << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            REQUIRE_ERRORS("test.css(1): error: an attribute selector expects to first find an identifier.\n");
        }
    }

    // attribute only accept a very few binary operators: =, |=, ~=, $=, ^=, *=
    // anything else is an error (including another identifier)
    {
        char const * invalid_value[] =
        {
            "identifier-too"
            "123",
            "1.23",
            "'1.23'",
            "1.23%",
            "(b)",
            "[b]",
            //"{b}", -- causes lexer problems at this time... not too sure whether that's normal though
            "+",
            ",",
            "/* @preserve this comment */",
            "/",
            "$",
            "!",
            ">",
            "#123",
            "*",
            ".top",
            "%name",
            "~",
            "&",
            "|",
        };

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b " << iv << " c]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            REQUIRE_ERRORS("test.css(1): error: expected attribute operator missing, supported operators are '=', '~=', '^=', '$=', '*=', and '|='.\n");
        }
    }

    // attribute and a binary operators: =, |=, ~=, $=, ^=, *=
    // not followed by any value
    {
        char const * invalid_value[] =
        {
            "=",
            " =",
            "= ",
            " = ",
            "|=",
            " |=",
            "|= ",
            " |= ",
            "~=",
            " ~=",
            "~= ",
            " ~= ",
            "$=",
            " $=",
            "$= ",
            " $= ",
            "^=",
            " ^=",
            "^= ",
            " ^= ",
            "*=",
            " *=",
            "*= ",
            " *= ",
        };

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << iv << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            REQUIRE_ERRORS("test.css(1): error: the attribute selector is expected to be an IDENTIFIER optionally followed by an operator and a value.\n");
        }
    }

    // attribute value can only be identifier, string, integer,
    // and decimal number
    {
        char const * invalid_value[] =
        {
            "1.23%",
            "(b)",
            "[b]",
            "{b}",
            "+",
            //"@b",
            //"<!--",
            //"-->",
            //")",
            //"}",
            ",",
            "/* @preserve this comment */",
            "|=",
            "/",
            "$",
            "=",
            "!",
            ">",
            "#123",
            "~=",
            "*",
            ".top",
            "%name",
            "~",
            "&",
            "|",
            //";b",
        };
        char const *op[] =
        {
            "=",
            "|=",
            "~=",
            "$=",
            "^=",
            "*="
        };

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << iv << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(2));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector value must be an identifier, a string, an integer, or a decimal number, a "
                   << op_node->get_type()
                   << " is not acceptable.\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << " " << iv << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(2));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector value must be an identifier, a string, an integer, or a decimal number, a "
                   << op_node->get_type()
                   << " is not acceptable.\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << iv << " ]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(2));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector value must be an identifier, a string, an integer, or a decimal number, a "
                   << op_node->get_type()
                   << " is not acceptable.\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << " " << iv << " ]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(2));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector value must be an identifier, a string, an integer, or a decimal number, a "
                   << op_node->get_type()
                   << " is not acceptable.\n";
            REQUIRE_ERRORS(errmsg.str());
        }
    }

    // attribute value can only be one token
    {
        char const * invalid_value[] =
        {
            "identifier",
            "123",
            "1.23",
            "'1.23'",
            "1.23%",
            "(b)",
            "[b]",
            "{b}",
            "+",
            //"@b",
            //"<!--",
            //"-->",
            //")",
            //"}",
            ",",
            "/* @preserve this comment */",
            "|=",
            "/",
            "$",
            "=",
            "!",
            ">",
            "#123",
            "~=",
            "*",
            ".top",
            "%name",
            "~",
            "&",
            "|",
            //";b",
        };
        char const *op[] =
        {
            "=",
            "|=",
            "~=",
            "$=",
            "^=",
            "*="
        };

        for(auto iv : invalid_value)
        {
            // without a space these gets glued to "c"
            std::string const v(iv);
            if(v == "identifier"    // "cidentifier"
            || v == "123"           // "c123"
            || v[0] == '(')         // "c(...)"
            {
                continue;
            }
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << "c" << iv << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(3));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector cannot be followed by more than one value, found "
                   << op_node->get_type()
                   << " after the value, missing quotes?\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << "c " << iv << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(3));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector cannot be followed by more than one value, found "
                   << op_node->get_type()
                   << " after the value, missing quotes?\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto iv : invalid_value)
        {
            // without a space these gets glued to "c"
            std::string const v(iv);
            if(v == "identifier"    // "cidentifier"
            || v == "123"           // "c123"
            || v[0] == '(')         // "c(...)"
            {
                continue;
            }
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << "c" << iv << " ]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(3));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector cannot be followed by more than one value, found "
                   << op_node->get_type()
                   << " after the value, missing quotes?\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto iv : invalid_value)
        {
            std::stringstream ss;
            ss << "a[b" << op[rand() % 6] << "c " << iv << " ]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(3));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: attribute selector cannot be followed by more than one value, found "
                   << op_node->get_type()
                   << " after the value, missing quotes?\n";
            REQUIRE_ERRORS(errmsg.str());
        }
    }

    // attribute value can only be one token
    {
        char const *op[] =
        {
            "=",
            "|=",
            "~=",
            "$=",
            "^=",
            "*="
        };

        for(auto o : op)
        {
            std::stringstream ss;
            ss << "a[b" << o << "]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            //csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(3));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: the attribute selector is expected to be an IDENTIFIER optionally followed by an operator and a value.\n";
            REQUIRE_ERRORS(errmsg.str());
        }

        for(auto o : op)
        {
            std::stringstream ss;
            ss << "a[b" << o << " ]{color:red}\n";
            csspp::position pos("test.css");
//std::cerr << "Test <<<" << ss.str() << ">>>\n";
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            // the node that caused a problem is:
            // LIST
            //   COMPONENT_VALUE
            //     ARG
            //       ...
            //       OPEN_SQUAREBRACKET
            //         ...
            //         ...
            //         <this one>
            //csspp::node::pointer_t op_node(n->get_child(0)->get_child(0)->get_child(1)->get_child(3));

            std::stringstream errmsg;
            errmsg << "test.css(1): error: the attribute selector is expected to be an IDENTIFIER optionally followed by an operator and a value.\n";
            REQUIRE_ERRORS(errmsg.str());
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Undefined Paths", "[compiler] [stylesheet]")
{
    // compile without defining the paths
    // (the result may be a success if you installed CSS Preprocessor
    // before since it will look for the scripts at "the right place!")
    {
        std::stringstream ss;
        ss << ":lang(fr) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        // c.add_path(...); -- check system default
//c.add_path(csspp_test::get_script_path());

        std::stringstream ignore;
        csspp::safe_error_stream_t safe_output(ignore);

        try
        {
            c.compile();

            // in case the system scripts are there, we want to check
            // that the result is fine
            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      COLON\n"
"      FUNCTION \"lang\"\n"
"        IDENTIFIER \"fr\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"

                );

        }
        catch(csspp::csspp_exception_exit const &)
        {
            REQUIRE(ignore.str() == "pseudo-nth-functions(1): fatal: validation script \"pseudo-nth-functions\" was not found.\n");
        }
    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("Simple Terms", "[compiler] [stylesheet]")
{
    // simple terms are:
    //      HASH
    //      IDENTIFIER
    //      IDENTIFIER '|' IDENTIFIER
    //      IDENTIFIER '|' '*'
    //      '*'
    //      '*' '|' IDENTIFIER
    //      '*' '|' '*'
    //      '|' IDENTIFIER
    //      '|' '*'
    //      ':' IDENTIFIER -- see below
    //      ':' FUNCTION ... ')'
    //      '.' IDENTIFIER
    //      '[' ... ']'
    {
        std::stringstream ss;
        ss << "#abd identifier ns|id namespace|* * *|abc *|*"
               << " |abc |* a:root :nth-child(3n+4) .class [foo]"
           << "{color:red;width:12px}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      HASH \"abd\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"identifier\"\n"
"      WHITESPACE\n"
// ns|id
"      IDENTIFIER \"ns\"\n"
"      SCOPE\n"
"      IDENTIFIER \"id\"\n"
"      WHITESPACE\n"
// namespace|*
"      IDENTIFIER \"namespace\"\n"
"      SCOPE\n"
"      MULTIPLY\n"
"      WHITESPACE\n"
// *
"      MULTIPLY\n"
"      WHITESPACE\n"
// *|abc
"      MULTIPLY\n"
"      SCOPE\n"
"      IDENTIFIER \"abc\"\n"
"      WHITESPACE\n"
// *|*
"      MULTIPLY\n"
"      SCOPE\n"
"      MULTIPLY\n"
"      WHITESPACE\n"
// |abc
"      SCOPE\n"
"      IDENTIFIER \"abc\"\n"
"      WHITESPACE\n"
// |*
"      SCOPE\n"
  "      MULTIPLY\n"
"      WHITESPACE\n"
// a:root
"      IDENTIFIER \"a\"\n"
"      COLON\n"
"      IDENTIFIER \"root\"\n"
"      WHITESPACE\n"
// :nth-child
"      COLON\n"
"      FUNCTION \"nth-child\"\n"
"        AN_PLUS_B S:3n+4\n"
"      WHITESPACE\n"
// .class
"      PERIOD\n"
"      IDENTIFIER \"class\"\n"
//"      WHITESPACE\n"
// [foo]
"      OPEN_SQUAREBRACKET\n"
"        IDENTIFIER \"foo\"\n"
// {color:red}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"
"      DECLARATION \"width\"\n"
"        INTEGER \"px\" I:12\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // check all pseudo-classes
    {
        char const * pseudo_name_table[] =
        {
            "root",
            "first-child",
            "last-child",
            "first-of-type",
            "last-of-type",
            "only-child",
            "only-of-type",
            "empty",
            "link",
            "visited",
            "active",
            "hover",
            "focus",
            "target",
            "enabled",
            "disabled",
            "checked"
        };

        for(auto pseudo_name : pseudo_name_table)
        {

            std::stringstream ss;
            ss << ":"
               << pseudo_name
               << "{color:red}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      COLON\n"
"      IDENTIFIER \"" + std::string(pseudo_name) + "\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"

            );
        }

        // no error left over
        REQUIRE_ERRORS("");
    }

    // check all pseudo-classes
    {
        char const * pseudo_name_table[] =
        {
            "root",
            "first-child",
            "last-child",
            "first-of-type",
            "last-of-type",
            "only-child",
            "only-of-type",
            "empty",
            "link",
            "visited",
            "active",
            "hover",
            "focus",
            "target",
            "enabled",
            "disabled",
            "checked"
        };

        for(auto pseudo_name : pseudo_name_table)
        {

            std::stringstream ss;
            ss << ":"
               << pseudo_name
               << "{color:red}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      COLON\n"
"      IDENTIFIER \"" + std::string(pseudo_name) + "\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"red\"\n"

            );
        }

        // no error left over
        REQUIRE_ERRORS("");
    }

    // test all nth pseudo-functions
    {
        char const * nth_functions[] =
        {
            "child",
            "last-child",
            "of-type",
            "last-of-type"
        };
        for(size_t i(0); i < sizeof(nth_functions) / sizeof(nth_functions[0]); ++i)
        {
            std::stringstream ss;
            ss << "div a:nth-" << nth_functions[i] << "(3n+1)"
               << "{color:#651}";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());

            c.compile();

            // no error left over
            REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"a\"\n"
"      COLON\n"
"      FUNCTION \"nth-" + std::string(nth_functions[i]) + "\"\n"
"        AN_PLUS_B S:3n+1\n"
// {color:blue}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        HASH \"651\"\n"

                );
        }

        // no error left over
        REQUIRE_ERRORS("");
    }

    // test the lang() function
    {
        std::stringstream ss;
        ss << "div q:lang(zu-za)"
           << "{color:#651}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"q\"\n"
"      COLON\n"
"      FUNCTION \"lang\"\n"
"        IDENTIFIER \"zu-za\"\n"
// {color:#651}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        HASH \"651\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // test the lang() function with 3 parameters
    {
        std::stringstream ss;
        ss << "div b:lang(fr-ca-nc)"
           << "{color:brisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"b\"\n"
"      COLON\n"
"      FUNCTION \"lang\"\n"
"        IDENTIFIER \"fr-ca-nc\"\n"
// {color:#651}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"brisque\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // :not(...)
    {
        std::stringstream ss;
        ss << "div:not(.red.blue) {color:coral}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
// :not(...)
"      COLON\n"
"      FUNCTION \"not\"\n"
"        PERIOD\n"
"        IDENTIFIER \"red\"\n"
"        PERIOD\n"
"        IDENTIFIER \"blue\"\n"
// {color:coral}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"coral\"\n"

            );

        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Simple Terms", "[compiler] [stylesheet]")
{
    // scope must be followed by * or IDENTIFIER
    {
        std::stringstream ss;
        ss << "*| {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the scope operator (|) requires a right hand side identifier or '*'.\n");
    }

    // scope must be followed by * or IDENTIFIER
    {
        std::stringstream ss;
        ss << "*|.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the right hand side of a scope operator (|) must be an identifier or '*'.\n");
    }

    // scope must be followed by * or IDENTIFIER
    {
        std::stringstream ss;
        ss << "div.white | {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a scope selector (|) must be followed by an identifier or '*'.\n");
    }

    // scope must be followed by * or IDENTIFIER
    {
        std::stringstream ss;
        ss << "div.white |#hash {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the right hand side of a scope operator (|) must be an identifier or '*'.\n");
    }

    // ':' must be followed by an IDENTIFIER or a FUNCTION
    {
        std::stringstream ss;
        ss << "div.white : {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a standalone ':'.\n");
    }

    // ':' must be followed a known pseudo-class name
    {
        std::stringstream ss;
        ss << "div.white :unknown {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("scripts/pseudo-classes.scss(35): error: unknown is not a valid name for a pseudo class; CSS only supports root, first-child, last-child, first-of-type, last-of-type, only-child, only-of-type, empty, link, visitived, active, hover, focus, target, enabled, disabled, and checked. (functions are not included in this list since you did not use '(' at the end of the word.)\n");
    }

    // ':' must be followed a known pseudo-function name
    {
        std::stringstream ss;
        ss << "div.white :unknown() {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("scripts/pseudo-functions.scss(20): error: unknown is not a valid name for a pseudo function; CSS only supports lang() and not().\n");
    }

    // ':' must be followed an identifier or a function
    {
        std::stringstream ss;
        ss << "div.white :.shark {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a ':' selector must be followed by an identifier or a function, a PERIOD was found instead.\n");
    }

    // '>' at the wrong place
    {
        std::stringstream ss;
        ss << "div.white > {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token GREATER_THAN, which is expected to be followed by another selector term.\n");
    }

    // :not(INTEGER) is not good
    {
        std::stringstream ss;
        ss << "div.white:not(11) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token INTEGER, which is not a valid selector token (simple term).\n");
    }

    // :not(FUNCTION) is not good
    {
        std::stringstream ss;
        ss << "div.white:not(func()) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found function \"func()\", which may be a valid selector token but only if immediately preceeded by a ':' (simple term).\n");
    }

    // :not(>) is not good
    {
        std::stringstream ss;
        ss << "div.white:not(>) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token GREATER_THAN, which cannot be used to start a selector expression.\n");
    }

    // :not(+) is not good
    {
        std::stringstream ss;
        ss << "div.white:not(+) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token ADD, which cannot be used to start a selector expression.\n");
    }

    // :not(~) is not good
    {
        std::stringstream ss;
        ss << "div.white:not(~) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token PRECEDED, which cannot be used to start a selector expression.\n");
    }

    // :not(:) is not good
    {
        std::stringstream ss;
        ss << "div.white:not(:) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a standalone ':'.\n");
    }

    // '.' by itself (at the end)
    {
        std::stringstream ss;
        ss << "div.lone . {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a standalone '.'.\n");
    }

    // '.' must be followed by IDENTIFIER
    {
        std::stringstream ss;
        ss << "div.lone .< {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a class selector (after a period: '.') must be an identifier.\n");
    }

    // test an invalid An+B in an :nth-child() function
    {
        std::stringstream ss;
        ss << "div:nth-child(3+5) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: The first number has to be followed by the 'n' character.\n");
    }

    // :not(:not(...))
    {
        std::stringstream ss;
        ss << "div:not(:not(.red)) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the :not() selector does not accept an inner :not().\n");
    }

    // :not(:.white)
    {
        std::stringstream ss;
        ss << "div:not(:.white) {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a ':' selector must be followed by an identifier or a function, a FUNCTION was found instead.\n");
    }

    // :lang() accepts only one argument
    {
        std::stringstream ss;
        ss << "div:lang(red blue) {color:bisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: a lang() function selector must have exactly one identifier as its parameter.\n");
    }

    // invalid name for :lang()
    {
        std::stringstream ss;
        ss << "div:lang(notalanguagename) {color:bisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("scripts/languages.scss(154): error: notalanguagename is not a valid language name for :lang().\n");
    }

    // invalid name for :lang(), with a valid country
    {
        std::stringstream ss;
        ss << "div:lang(stillnotalanguagename-us) {color:bisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("scripts/languages.scss(154): error: stillnotalanguagename is not a valid language name for :lang().\n");
    }

    // invalid name for :lang(), with a valid country
    {
        std::stringstream ss;
        ss << "div:lang(mn-withaninvalidcountry-andmore) {color:bisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("scripts/countries.scss(267): error: withaninvalidcountry is not a valid country name for :lang().\n");
    }

    // :lang() name must be an identifier
    {
        std::stringstream ss;
        ss << "div:lang(\"de\") {color:bisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: a lang() function selector expects an identifier as its parameter.\n");
    }

    // :INTEGER
    {
        std::stringstream ss;
        ss << "div:556 {color:bisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: a ':' selector must be followed by an identifier or a function, a INTEGER was found instead.\n");
    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("Complex Terms", "[compiler] [stylesheet]")
{
    // [complex] terms are:
    // term: simple-term
    //     | PLACEHOLDER
    //     | REFERENCE
    //     | ':' FUNCTION (="not") component-value-list ')'
    //     | ':' ':' IDENTIFIER

    // test a placeholder
    {
        std::stringstream ss;
        ss << "div p%image"
           << "{color:blue}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"p\"\n"
"      PLACEHOLDER \"image\"\n"
// {color:blue}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"blue\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // test a reference
    {
        std::stringstream ss;
        ss << "div a"
           << "{color:blue;&:hover{color:red}}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"a\"\n"
// {color:blue}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"blue\"\n"
"      COMPONENT_VALUE\n"
"        ARG\n"
// &:hover
"          REFERENCE\n"
"          COLON\n"
"          IDENTIFIER \"hover\"\n"
"        OPEN_CURLYBRACKET\n"
"          DECLARATION \"color\"\n"
"            IDENTIFIER \"red\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // test the not() function
    {
        std::stringstream ss;
        ss << "div a:not(:hover)"
           << "{color:#175}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"a\"\n"
"      COLON\n"
"      FUNCTION \"not\"\n"
"        COLON\n"
"        IDENTIFIER \"hover\"\n"
// {color:blue}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        HASH \"175\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // test the not() function + a sub-function
    {
        std::stringstream ss;
        ss << "div a:not(:nth-last-of-type(5n+3))"
           << "{color:#175}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"a\"\n"
"      COLON\n"
"      FUNCTION \"not\"\n"
"        COLON\n"
"        FUNCTION \"nth-last-of-type\"\n"
"          AN_PLUS_B S:5n+3\n"
// {color:blue}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        HASH \"175\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // check all pseudo-elements
    {
        char const * pseudo_name_table[] =
        {
            "first-line",
            "first-letter",
            "before",
            "after"
        };

        for(auto pseudo_name : pseudo_name_table)
        {
            std::stringstream ss;
            ss << "div ::"
               << pseudo_name
               << "{color:teal}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            // no errors so far
            REQUIRE_ERRORS("");

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());

            c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      COLON\n"
"      IDENTIFIER \"" + std::string(pseudo_name) + "\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"teal\"\n"

            );
        }

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Complex Terms", "[compiler] [stylesheet]")
{
    // '::' must be followed by an IDENTIFIER
    {
        std::stringstream ss;
        ss << "div.white :: {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a '::'.\n");
    }

    // '::' must be followed a known pseudo-element name
    {
        std::stringstream ss;
        ss << "div.white ::unknown {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("scripts/pseudo-elements.scss(22): error: unknown is not a valid name for a pseudo element; CSS only supports first-line, first-letter, before, and after.\n");
    }

    // '::' must be followed an IDENTIFIER
    {
        std::stringstream ss;
        ss << "div.white ::.shark {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a pseudo element name (defined after a '::' in a list of selectors) must be defined using an identifier.\n");
    }

    // '>' cannot start a selector list
    {
        std::stringstream ss;
        ss << "> div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token GREATER_THAN, which cannot be used to start a selector expression.\n");
    }

    // '+' cannot start a selector list
    {
        std::stringstream ss;
        ss << "+ div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token ADD, which cannot be used to start a selector expression.\n");
    }

    // '~' cannot start a selector list
    {
        std::stringstream ss;
        ss << "~ div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token PRECEDED, which cannot be used to start a selector expression.\n");
    }

    // selector cannot start with a FUNCTION
    {
        std::stringstream ss;
        ss << "func() div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found function \"func()\", which may be a valid selector token but only if immediately preceeded by a ':' (term).\n");
    }

    // selectors do not support INTEGER
    {
        std::stringstream ss;
        ss << "13 div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token INTEGER, which is not a valid selector token (term).\n");
    }

    // selectors do not support DECIMAL_NUMBER
    {
        std::stringstream ss;
        ss << "13.25 div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token DECIMAL_NUMBER, which is not a valid selector token (term).\n");
    }

    // selectors do not support PERCENT
    {
        std::stringstream ss;
        ss << "13% div.white {color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token PERCENT, which is not a valid selector token (term).\n");
    }

    // no left over?
    REQUIRE_ERRORS("");
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
