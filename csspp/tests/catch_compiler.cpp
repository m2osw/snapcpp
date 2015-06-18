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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


TEST_CASE("Compile Simple Stylesheets", "[compiler] [stylesheet] [attribute]")
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

        REQUIRE(c.get_root() == n);
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

        REQUIRE(c.get_root() == n);
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

        REQUIRE(c.get_root() == n);
    }
}

TEST_CASE("Invalid Arguments", "[compiler] [invalid]")
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

        REQUIRE(c.get_root() == n);
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

        REQUIRE(c.get_root() == n);
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

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Selector Attribute Tests", "[compiler] [stylesheet] [attribute]")
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

        REQUIRE(c.get_root() == n);
    }
}

TEST_CASE("Invalid Attributes", "[compiler] [invalid]")
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
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

            REQUIRE(c.get_root() == n);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Undefined Paths", "[compiler] [invalid]")
{
    // compile without defining the paths
    //
    // (The result may be a success if you installed CSS Preprocessor
    // before since it will look for the scripts at "the right place!"
    // when the packages are installed properly on a system.)
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
        c.clear_paths();
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

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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

        REQUIRE(c.get_root() == n);
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
            c.clear_paths();
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

            REQUIRE(c.get_root() == n);
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
            c.clear_paths();
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

            REQUIRE(c.get_root() == n);
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
            c.clear_paths();
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
"        COLOR H:ff115566\n"

                );

            REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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
"        COLOR H:ff115566\n"

            );

        // no error left over
        REQUIRE_ERRORS("");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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

        REQUIRE(c.get_root() == n);
    }

    // test the lang() multiple times to verify that the cache works
    {
        std::stringstream ss;
        ss << "div b:lang(qu-vg-rr),section i:lang(ks-sm-dp)"
           << "{color:brisque}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
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
"        IDENTIFIER \"qu-vg-rr\"\n"
"    ARG\n"
// #abd
"      IDENTIFIER \"section\"\n"
"      WHITESPACE\n"
// identifier
"      IDENTIFIER \"i\"\n"
"      COLON\n"
"      FUNCTION \"lang\"\n"
"        IDENTIFIER \"ks-sm-dp\"\n"
// {color:#651}
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"color\"\n"
"        IDENTIFIER \"brisque\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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

        REQUIRE(c.get_root() == n);
    }
}

TEST_CASE("Invalid Simple Terms", "[compiler] [invalid]")
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the scope operator (|) requires a right hand side identifier or '*'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the right hand side of a scope operator (|) must be an identifier or '*'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a scope selector (|) must be followed by an identifier or '*'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the right hand side of a scope operator (|) must be an identifier or '*'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a standalone ':'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("scripts/pseudo-classes.scss(35): error: unknown is not a valid name for a pseudo class; CSS only supports root, first-child, last-child, first-of-type, last-of-type, only-child, only-of-type, empty, link, visitived, active, hover, focus, target, enabled, disabled, and checked. (functions are not included in this list since you did not use '(' at the end of the word.)\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("scripts/pseudo-functions.scss(20): error: unknown is not a valid name for a pseudo function; CSS only supports lang() and not().\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a ':' selector must be followed by an identifier or a function, a PERIOD was found instead.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token GREATER_THAN, which is expected to be followed by another selector term.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token INTEGER, which is not a valid selector token (simple term).\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found function \"func()\", which may be a valid selector token but only if immediately preceeded by a ':' (simple term).\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token GREATER_THAN, which cannot be used to start a selector expression.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token ADD, which cannot be used to start a selector expression.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token PRECEDED, which cannot be used to start a selector expression.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a standalone ':'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a standalone '.'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a class selector (after a period: '.') must be an identifier.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: The first number has to be followed by the 'n' character.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: the :not() selector does not accept an inner :not().\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a ':' selector must be followed by an identifier or a function, a FUNCTION was found instead.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: a lang() function selector must have exactly one identifier as its parameter.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("scripts/languages.scss(154): error: notalanguagename is not a valid language name for :lang().\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("scripts/languages.scss(154): error: stillnotalanguagename is not a valid language name for :lang().\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("scripts/countries.scss(267): error: withaninvalidcountry is not a valid country name for :lang().\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: a lang() function selector expects an identifier as its parameter.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: a ':' selector must be followed by an identifier or a function, a INTEGER was found instead.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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
"        COLOR H:ff557711\n"

            );

        // no error left over
        REQUIRE_ERRORS("");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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
"        COLOR H:ff557711\n"

            );

        // no error left over
        REQUIRE_ERRORS("");

        REQUIRE(c.get_root() == n);
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
            c.clear_paths();
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

            REQUIRE(c.get_root() == n);
        }

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Complex Terms", "[compiler] [invalid]")
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a selector list cannot end with a '::'.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a pseudo element name (defined after a '::' in a list of selectors) must be defined using an identifier.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token GREATER_THAN, which cannot be used to start a selector expression.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token ADD, which cannot be used to start a selector expression.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token PRECEDED, which cannot be used to start a selector expression.\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found function \"func()\", which may be a valid selector token but only if immediately preceeded by a ':' (term).\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token INTEGER, which is not a valid selector token (term).\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token DECIMAL_NUMBER, which is not a valid selector token (term).\n");

        REQUIRE(c.get_root() == n);
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
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: found token PERCENT, which is not a valid selector token (term).\n");

        REQUIRE(c.get_root() == n);
    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid Node", "[compiler] [invalid]")
{
    // create a fake node tree with some invalid node types to
    // exercise the compile() switch default entry
    {
        csspp::node_type_t invalid_types[] =
        {
            csspp::node_type_t::COMMA,
            csspp::node_type_t::ADD,
            csspp::node_type_t::CLOSE_CURLYBRACKET,
        };

        for(size_t idx(0); idx < sizeof(invalid_types) / sizeof(invalid_types[0]); ++idx)
        {
            csspp::position pos("invalid-types.scss");
            csspp::node::pointer_t n(new csspp::node(invalid_types[idx], pos));

            csspp::compiler c;
            c.set_root(n);
            c.clear_paths();
            c.add_path(csspp_test::get_script_path());

            REQUIRE_THROWS_AS(c.compile(), csspp::csspp_exception_unexpected_token);

            REQUIRE(c.get_root() == n);
        }
    }

    // qualified rule must start with an identifier
    {
        std::stringstream ss;
        ss << "{color:red}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: a qualified rule without selectors is not valid.\n");

        REQUIRE(c.get_root() == n);
    }

    // qualified rule must start with an identifier
    {
        std::stringstream ss;
        ss << "this would be a declaration without a colon;";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // the qualified rule is invalid...
        REQUIRE_ERRORS("test.css(1): error: A qualified rule must end with a { ... } block.\n");

        // ...but we still compile it so we get a specific error that we do
        // not get otherwise.
        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: expected a ':' after the identifier of this declaration value; got a: COMPONENT_VALUE instead.\n");

        REQUIRE(c.get_root() == n);
    }

    // a declaration needs an identifier
    {
        std::stringstream ss;
        ss << "rule{+: red;}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: expected an identifier to start a declaration value; got a: ADD instead.\n");

        REQUIRE(c.get_root() == n);
    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("Nested Declarations", "[compiler] [invalid]")
{
    // define a sub-declaration inside a declaration
    {
        std::stringstream ss;
        ss << "div { font: { family: helvetica; color: red; size: 3px + 5px }; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("");

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"font\"\n"
"        OPEN_CURLYBRACKET\n"
"          DECLARATION \"family\"\n"
"            IDENTIFIER \"helvetica\"\n"
"          DECLARATION \"color\"\n"
"            IDENTIFIER \"red\"\n"
"          DECLARATION \"size\"\n"
"            INTEGER \"px\" I:8\n"

            );

        REQUIRE(c.get_root() == n);
    }

    // define a sub-declaration inside a declaration
    {
        std::stringstream ss;
        ss << "div { margin: { left: 300px + 51px / 3; top: 3px + 5px }; }"
           << " $size: 300px;"
           << " p { margin: 10px + $size * 3 25px - $size * 3 }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("");

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"    V:size\n"
"      LIST\n"
"        VARIABLE \"size\"\n"
"        INTEGER \"px\" I:300\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"margin\"\n"
"        OPEN_CURLYBRACKET\n"
"          DECLARATION \"left\"\n"
"            INTEGER \"px\" I:317\n"
"          DECLARATION \"top\"\n"
"            INTEGER \"px\" I:8\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"margin\"\n"
"        INTEGER \"px\" I:910\n"
"        WHITESPACE\n"
"        INTEGER \"px\" I:-875\n"

            );

        REQUIRE(c.get_root() == n);
    }

    // define the sub-declaration in a variable
    {
        std::stringstream ss;
        ss << "$m : { left: 300px + 51px / 3; top: 3px + 5px };"
           << " div { margin: $m; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("");

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"    V:m\n"
"      LIST\n"
"        VARIABLE \"m\"\n"
"        OPEN_CURLYBRACKET\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"left\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:300\n"
"            WHITESPACE\n"
"            ADD\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:51\n"
"            WHITESPACE\n"
"            DIVIDE\n"
"            WHITESPACE\n"
"            INTEGER \"\" I:3\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"top\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:3\n"
"            WHITESPACE\n"
"            ADD\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:5\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"margin\"\n"
"        DECLARATION \"left\"\n"
"          INTEGER \"px\" I:317\n"
"        DECLARATION \"top\"\n"
"          INTEGER \"px\" I:8\n"

            );

        REQUIRE(c.get_root() == n);
    }

//    // define the sub-declaration in a variable
//    {
//std::cerr << "------------------------------------------------ WORKING ON straight entry\n";
//        std::stringstream ss;
//        ss << "$m : left: 300px + 51px / 3; top: 3px + 5px;"
//           << " div { margin: $m; }";
//        csspp::position pos("test.css");
//        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));
//
//        csspp::parser p(l);
//
//        csspp::node::pointer_t n(p.stylesheet());
//
//        // no errors so far
//        REQUIRE_ERRORS("");
//
//        csspp::compiler c;
//        c.set_root(n);
//        c.clear_paths();
//        c.add_path(csspp_test::get_script_path());
//
//        c.compile();
//
//std::cerr << "Result is: [" << *c.get_root() << "]\n";
//
//        REQUIRE_ERRORS("");
//
//        std::stringstream out;
//        out << *n;
//        REQUIRE_TREES(out.str(),
//
//"LIST\n"
//"    V:m\n"
//"      OPEN_CURLYBRACKET\n"
//"        COMPONENT_VALUE\n"
//"          IDENTIFIER \"left\"\n"
////"          COLON\n"
////"          WHITESPACE\n"
////"          INTEGER \"px\" I:300\n"
////"          WHITESPACE\n"
////"          ADD\n"
////"          WHITESPACE\n"
////"          INTEGER \"px\" I:51\n"
////"          WHITESPACE\n"
////"          DIVIDE\n"
////"          WHITESPACE\n"
////"          INTEGER \"\" I:3\n"
//"        COMPONENT_VALUE\n"
//"          IDENTIFIER \"top\"\n"
////"          COLON\n"
////"          WHITESPACE\n"
////"          INTEGER \"px\" I:3\n"
////"          WHITESPACE\n"
////"          ADD\n"
////"          WHITESPACE\n"
////"          INTEGER \"px\" I:5\n"
//"  COMPONENT_VALUE\n"
//"    ARG\n"
//"      IDENTIFIER \"div\"\n"
//"    OPEN_CURLYBRACKET\n"
//"      DECLARATION \"margin\"\n"
//"        DECLARATION \"left\"\n"
//"          INTEGER \"px\" I:317\n"
//"        DECLARATION \"top\"\n"
//"          INTEGER \"px\" I:8\n"
//
//            );
//
//        REQUIRE(c.get_root() == n);
//    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("Advanced Variable", "[compiler] [invalid]")
{
    // define a variable function with a parameter
    {
        std::stringstream ss;
        ss << "$m( $width, $border: 1px ) : { left: $width + 51px / 3; top: $border + 5px };"
           << " div { margin: $m(300px, 3px); }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("");

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"    V:m\n"
"      LIST\n"
"        VARIABLE_FUNCTION \"m\"\n"
"          ARG\n"
"            VARIABLE \"width\"\n"
"          ARG\n"
"            VARIABLE \"border\"\n"
"            INTEGER \"px\" I:1\n"
"        OPEN_CURLYBRACKET\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"left\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            VARIABLE \"width\"\n"
"            WHITESPACE\n"
"            ADD\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:51\n"
"            WHITESPACE\n"
"            DIVIDE\n"
"            WHITESPACE\n"
"            INTEGER \"\" I:3\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"top\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            VARIABLE \"border\"\n"
"            WHITESPACE\n"
"            ADD\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:5\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      DECLARATION \"margin\"\n"
"        OPEN_CURLYBRACKET\n"
"          DECLARATION \"left\"\n"
"            INTEGER \"px\" I:317\n"
"          DECLARATION \"top\"\n"
"            INTEGER \"px\" I:8\n"

            );

        REQUIRE(c.get_root() == n);
    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("At-Keyword Messages", "[compiler] [invalid]")
{
    // generate an error with @error
    {
        std::stringstream ss;
        ss << "@error \"This is an error.\";";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): error: This is an error.\n");

        REQUIRE(c.get_root() == n);
    }

    // generate a warning with @warning
    {
        std::stringstream ss;
        ss << "@warning \"This is a warning.\";";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): warning: This is a warning.\n");

        REQUIRE(c.get_root() == n);
    }

    // output a message with @info
    {
        std::stringstream ss;
        ss << "@info \"This is an info message.\";";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): info: This is an info message.\n");

        REQUIRE(c.get_root() == n);
    }

    // make sure @message does the same as @info
    {
        std::stringstream ss;
        ss << "@message \"This is an info message.\";";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        REQUIRE_ERRORS("test.css(1): info: This is an info message.\n");

        REQUIRE(c.get_root() == n);
    }

    // test @debug does nothing by default
    {
        std::stringstream ss;
        ss << "@debug \"This is a debug message.\";";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        c.compile();

        // by default debug messages do not make it to the output
        REQUIRE_ERRORS("");

        REQUIRE(c.get_root() == n);
    }

    // make sure @debug does the same as @info
    {
        std::stringstream ss;
        ss << "@debug \"This is a debug message.\";";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no errors so far
        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.clear_paths();
        c.add_path(csspp_test::get_script_path());

        csspp::error::instance().set_show_debug(true);
        c.compile();
        csspp::error::instance().set_show_debug(false);

        REQUIRE_ERRORS("test.css(1): debug: This is a debug message.\n");

        REQUIRE(c.get_root() == n);
    }

    // no left over?
    REQUIRE_ERRORS("");
}

// This does not work under Linux, the ifstream.open() accepts a
// directory name as input without generating an error
//
//TEST_CASE("Cannot Open File", "[compiler] [invalid] [input]")
//{
//    // generate an error with @error
//    {
//        // create a directory in place of the script, so it exists
//        // and is readable but cannot be opened
//        rmdir("pseudo-nth-functions.scss"); // in case you run more than once
//        REQUIRE(mkdir("pseudo-nth-functions.scss", 0700) == 0);
//
//        std::stringstream ss;
//        ss << "div:nth-child(3n+2){font-style:normal}";
//        csspp::position pos("test.css");
//        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));
//
//        csspp::parser p(l);
//
//        csspp::node::pointer_t n(p.stylesheet());
//
//        // no errors so far
//        REQUIRE_ERRORS("");
//
//        csspp::compiler c;
//        c.set_root(n);
//        c.clear_paths();
//        c.add_path(".");
//
//        REQUIRE_THROWS_AS(c.compile(), csspp::csspp_exception_exit);
//
//        // TODO: use an RAII class instead
//        rmdir("pseudo-nth-functions.scss"); // in case you run more than once
//
//        REQUIRE_ERRORS("pseudo-nth-functions(1): fatal: validation script \"pseudo-nth-functions\" was not found.\n");
//
//        REQUIRE(c.get_root() == n);
//    }
//
//    // no left over?
//    REQUIRE_ERRORS("");
//}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
