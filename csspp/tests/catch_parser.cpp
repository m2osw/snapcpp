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




TEST_CASE("Simple Stylesheets", "[parser] [stylesheet] [rules]")
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
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"body\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"background\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"white\"\n"
"      WHITESPACE\n"
"      URL \"/images/background.png\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    {
        std::stringstream ss;
        ss << "<!-- body { background : white url( /images/background.png ) } --><!-- div { border: 1px; } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"body\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"background\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"white\"\n"
"      WHITESPACE\n"
"      URL \"/images/background.png\"\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"border\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      INTEGER \"px\" I:1\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // one large rule with semicolons inside
    {
        std::stringstream ss;
        ss << "div\n"
           << "{\n"
           << "    background-color: rgba(33, 77, 99, 0.3);\n"
           << "    color: rgba(0, 3, 5, 0.95);\n"
           << "    font-style: italic;\n"
           << "}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"background-color\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        FUNCTION \"rgba\"\n"
"          INTEGER \"\" I:33\n"
"          COMMA\n"
"          WHITESPACE\n"
"          INTEGER \"\" I:77\n"
"          COMMA\n"
"          WHITESPACE\n"
"          INTEGER \"\" I:99\n"
"          COMMA\n"
"          WHITESPACE\n"
"          DECIMAL_NUMBER \"\" D:0.3\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"color\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        FUNCTION \"rgba\"\n"
"          INTEGER \"\" I:0\n"
"          COMMA\n"
"          WHITESPACE\n"
"          INTEGER \"\" I:3\n"
"          COMMA\n"
"          WHITESPACE\n"
"          INTEGER \"\" I:5\n"
"          COMMA\n"
"          WHITESPACE\n"
"          DECIMAL_NUMBER \"\" D:0.95\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"font-style\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        IDENTIFIER \"italic\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // a comment, a simple rule, a comment
    {
        std::stringstream ss;
        ss << "// $Id: ...$\n"
           << "div { border: 1px; }\n"
           << "/* @preserve Copyright (c) 2015  Made to Order Software Corp. */";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"border\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      INTEGER \"px\" I:1\n"
"  COMMENT \"@preserve Copyright (c) 2015  Made to Order Software Corp.\" I:1\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // one empty C-like comment
    {
        std::stringstream ss;
        ss << "div { /**/ border: 1px; /**/ }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"border\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      INTEGER \"px\" I:1\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // multiple empty C-like comments
    {
        std::stringstream ss;
        ss << "div { /**/ /**/ /**/ border: 1px; /**/ /**/ /**/ }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"border\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      INTEGER \"px\" I:1\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // cascading fields
    {
        std::stringstream ss;
        ss << "div {\n"
           << "  font: { family: ivory; size: 16pt; style: italic };\n"
           << "  border: { color: #112389; width: 1px } /**/ ;\n"
           << "  color: /* text color */ white;\n"
           << "}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";
        REQUIRE_ERRORS("");

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"font\"\n"
"        COLON\n"
"        OPEN_CURLYBRACKET\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"family\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            IDENTIFIER \"ivory\"\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"size\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            INTEGER \"pt\" I:16\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"style\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            IDENTIFIER \"italic\"\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"border\"\n"
"        COLON\n"
"        OPEN_CURLYBRACKET\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"color\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            HASH \"112389\"\n"
"          COMPONENT_VALUE\n"
"            IDENTIFIER \"width\"\n"
"            COLON\n"
"            WHITESPACE\n"
"            INTEGER \"px\" I:1\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"color\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        IDENTIFIER \"white\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Stylesheets", "[parser] [stylesheet] [invalid]")
{
    // closing '}' one too many times
    {
        std::stringstream ss;
        ss << "<!-- body { background : white url( /images/background.png ) } --> }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS("test.css(1): error: Unexpected closing block of type: CLOSE_CURLYBRACKET.\n");
    }

    // closing ']' one too many times
    {
        std::stringstream ss;
        ss << "<!-- body[browser~=\"great\"]] { background : white url( /images/background.png ) } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule must end with a { ... } block.\n"
                "test.css(1): error: Unexpected closing block of type: CLOSE_SQUAREBRACKET.\n"
            );
    }

    // closing ')' one too many times
    {
        std::stringstream ss;
        ss << "<!-- body[browser~=\"great\"] { background : white url( /images/background.png ); border-top-color: rgb(1,2,3)); } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: Block expected to end with CLOSE_CURLYBRACKET but got CLOSE_PARENTHESIS instead.\n"
                "test.css(1): error: Unexpected closing block of type: CLOSE_PARENTHESIS.\n"
            );
    }

    // extra ';'
    {
        std::stringstream ss;
        ss << "illegal { semi: colon };";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule cannot end a { ... } block with a ';'.\n"
            );
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Simple Rules", "[parser] [rule-list]")
{
    // a simple valid rule
    {
        std::stringstream ss;
        ss << " body { background : gradient(to bottom, #012, #384513 75%, #452) } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"body\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"background\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      WHITESPACE\n"
"      FUNCTION \"gradient\"\n"
"        IDENTIFIER \"to\"\n"
"        WHITESPACE\n"
"        IDENTIFIER \"bottom\"\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"012\"\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"384513\"\n"
"        WHITESPACE\n"
"        PERCENT D:0.75\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"452\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // a simple valid rule
    {
        std::stringstream ss;
        ss << " div { color: blue; }"
           << " @media screen { viewport: 1000px 500px; } "
           << " div#op{color:hsl(120,1,0.5)}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"blue\"\n"
"  AT_KEYWORD \"media\" I:0\n"
"    IDENTIFIER \"screen\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"viewport\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      INTEGER \"px\" I:1000\n"
"      WHITESPACE\n"
"      INTEGER \"px\" I:500\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    HASH \"op\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      COLON\n"
"      FUNCTION \"hsl\"\n"
"        INTEGER \"\" I:120\n"
"        COMMA\n"
"        INTEGER \"\" I:1\n"
"        COMMA\n"
"        DECIMAL_NUMBER \"\" D:0.5\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Nested Rules", "[parser] [rule-list]")
{
    // at rule inside another at rule
    {
        std::stringstream ss;
        ss << "@if true { @message \"blah\"; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule_list());

        // no error left over
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  AT_KEYWORD \"if\" I:0\n"
"    IDENTIFIER \"true\"\n"
"    OPEN_CURLYBRACKET\n"
"      AT_KEYWORD \"message\" I:0\n"
"        STRING \"blah\"\n"

            );
    }
}

TEST_CASE("Invalid Rules", "[parser] [rule-list] [invalid]")
{
    // breaks on the <!--
    {
        std::stringstream ss;
        ss << "<!-- body { background : white url( /images/background.png ) } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS("test.css(1): error: HTML comment delimiters (<!-- and -->) are not allowed in this CSS document.\n");
    }

    // breaks on the -->
    {
        std::stringstream ss;
        ss << "body { background : white url( /images/background.png ) 44px } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule cannot be empty; you are missing a { ... } block.\n"
                "test.css(1): error: HTML comment delimiters (<!-- and -->) are not allowed in this CSS document.\n"
            );
    }

    // breaks on the }
    {
        std::stringstream ss;
        ss << "body { background : white url( /images/background.png ) } }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule cannot be empty; you are missing a { ... } block.\n"
                "test.css(1): error: Unexpected closing block of type: CLOSE_CURLYBRACKET.\n"
            );
    }

    // breaks on the ]
    {
        std::stringstream ss;
        ss << "body[lili=\"joe\"]] { background : white url( /images/background.png ) } }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule must end with a { ... } block.\n"
                "test.css(1): error: Unexpected closing block of type: CLOSE_SQUAREBRACKET.\n"
            );
    }

    // breaks on the )
    {
        std::stringstream ss;
        ss << " body[lili=\"joe\"] { background : white url( /images/background.png ); color:rgba(0,0,0,0)); } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: Block expected to end with CLOSE_CURLYBRACKET but got CLOSE_PARENTHESIS instead.\n"
                "test.css(1): error: Unexpected closing block of type: CLOSE_PARENTHESIS.\n"
            );
    }

    // a @-rule cannot be empty
    {
        std::stringstream ss;
        ss << " div { color: blue; }"
           << " @media";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        REQUIRE_ERRORS("test.css(1): error: At '@' command cannot be empty (missing expression or block) unless ended by a semicolon (;).\n");
    }

    // a @-rule cannot be empty
    {
        std::stringstream ss;
        ss << "@media test and (this one too) or (that maybe)";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.rule_list());

//std::cerr << "Result is: [" << *n << "]\n";

        REQUIRE_ERRORS("test.css(1): error: At '@' command must end with a block or a ';'.\n");
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("One Simple Rule", "[parser] [rule]")
{
    // a simple valid rule
    {
        std::stringstream ss;
        ss << " body { background : gradient(to bottom, #012, #384513 75%, #452) } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"body\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"background\"\n"
"    WHITESPACE\n"
"    COLON\n"
"    WHITESPACE\n"
"    FUNCTION \"gradient\"\n"
"      IDENTIFIER \"to\"\n"
"      WHITESPACE\n"
"      IDENTIFIER \"bottom\"\n"
"      COMMA\n"
"      WHITESPACE\n"
"      HASH \"012\"\n"
"      COMMA\n"
"      WHITESPACE\n"
"      HASH \"384513\"\n"
"      WHITESPACE\n"
"      PERCENT D:0.75\n"
"      COMMA\n"
"      WHITESPACE\n"
"      HASH \"452\"\n"

            );

    }

    // a simple valid rule
    {
        std::stringstream ss;
        ss << " div { color: blue; }"
           << " @media screen { viewport: 1000px 500px; } "
           << " div#op{color:hsl(120,1,0.5)}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"div\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"color\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    IDENTIFIER \"blue\"\n"

            );

        n = p.rule();

        out.str("");
        out << *n;
        REQUIRE_TREES(out.str(),

"AT_KEYWORD \"media\" I:0\n"
"  IDENTIFIER \"screen\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"viewport\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    INTEGER \"px\" I:1000\n"
"    WHITESPACE\n"
"    INTEGER \"px\" I:500\n"

            );

        n = p.rule();

        out.str("");
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"div\"\n"
"  HASH \"op\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"color\"\n"
"    COLON\n"
"    FUNCTION \"hsl\"\n"
"      INTEGER \"\" I:120\n"
"      COMMA\n"
"      INTEGER \"\" I:1\n"
"      COMMA\n"
"      DECIMAL_NUMBER \"\" D:0.5\n"

            );

    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid One Rule", "[parser] [rule] [invalid]")
{
    // breaks on the <!--
    {
        std::stringstream ss;
        ss << "<!-- body { background : white url( /images/background.png ) } -->";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS("test.css(1): error: HTML comment delimiters (<!-- and -->) are not allowed in this CSS document.\n");
    }

    // breaks on the -->
    {
        std::stringstream ss;
        ss << "--> body { background : white url( /images/background.png ) }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS("test.css(1): error: HTML comment delimiters (<!-- and -->) are not allowed in this CSS document.\n");
    }

    // breaks on the }
    {
        std::stringstream ss;
        ss << "body { background : white url( /images/background.png ) } }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // the first read works as expected
        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"body\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"background\"\n"
"    WHITESPACE\n"
"    COLON\n"
"    WHITESPACE\n"
"    IDENTIFIER \"white\"\n"
"    WHITESPACE\n"
"    URL \"/images/background.png\"\n"

            );

        // this failed with an error, no need to check the "broken" output
        n = p.rule();

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule cannot be empty; you are missing a { ... } block.\n"
                //"test.css(1): error: Unexpected closing block of type: CLOSE_CURLYBRACKET.\n"
            );
    }

    // breaks on the ]
    {
        std::stringstream ss;
        ss << "body[lili=\"joe\"]] { background : white url( /images/background.png ) } }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule up to the spurious ']' is all proper
        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"body\"\n"
"  OPEN_SQUAREBRACKET\n"
"    IDENTIFIER \"lili\"\n"
"    EQUAL\n"
"    STRING \"joe\"\n"

            );

        // this failed with an error, no need to check the "broken" output
        n = p.rule();

        REQUIRE_ERRORS(
                "test.css(1): error: A qualified rule must end with a { ... } block.\n"
                "test.css(1): error: Unexpected closing block of type: CLOSE_SQUAREBRACKET.\n"
            );
    }

    // breaks on the )
    {
        std::stringstream ss;
        ss << " body[lili=\"joe\"] { background : white url( /images/background.png ); color:rgba(0,0,0,0)); } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: Block expected to end with CLOSE_CURLYBRACKET but got CLOSE_PARENTHESIS instead.\n"
                //"test.css(1): error: Unexpected closing block of type: CLOSE_PARENTHESIS.\n"
            );
    }

    // a @-rule cannot be empty
    {
        std::stringstream ss;
        ss << " div { color: blue; }"
           << " @media";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"div\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"color\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    IDENTIFIER \"blue\"\n"

            );

        // this failed with an error, no need to check the "broken" output
        n = p.rule();

        REQUIRE_ERRORS("test.css(1): error: At '@' command cannot be empty (missing expression or block) unless ended by a semicolon (;).\n");
    }

    // a @-rule cannot be empty
    {
        std::stringstream ss;
        ss << "@media test and (this one too) or (that maybe)";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.rule());

//std::cerr << "Result is: [" << *n << "]\n";

        REQUIRE_ERRORS("test.css(1): error: At '@' command must end with a block or a ';'.\n");
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Simple Component Values", "[parser] [component-value]")
{
    // a simple valid rule
    {
        std::stringstream ss;
        ss << " body { background : gradient(to bottom, #012, #384513 75%, #452) } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.component_value_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"body\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"background\"\n"
"    WHITESPACE\n"
"    COLON\n"
"    WHITESPACE\n"
"    FUNCTION \"gradient\"\n"
"      IDENTIFIER \"to\"\n"
"      WHITESPACE\n"
"      IDENTIFIER \"bottom\"\n"
"      COMMA\n"
"      WHITESPACE\n"
"      HASH \"012\"\n"
"      COMMA\n"
"      WHITESPACE\n"
"      HASH \"384513\"\n"
"      WHITESPACE\n"
"      PERCENT D:0.75\n"
"      COMMA\n"
"      WHITESPACE\n"
"      HASH \"452\"\n"

            );

    }

    // a simple valid rule
    {
        std::stringstream ss;
        ss << " div { color: blue; }"
           << " @media screen { viewport: 1000px 500px; } "
           << " div#op{color:hsl(120,1,0.5)}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.component_value_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"div\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"color\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    IDENTIFIER \"blue\"\n"

            );

        n = p.rule();

        out.str("");
        out << *n;
        REQUIRE_TREES(out.str(),

"AT_KEYWORD \"media\" I:0\n"
"  IDENTIFIER \"screen\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"viewport\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    INTEGER \"px\" I:1000\n"
"    WHITESPACE\n"
"    INTEGER \"px\" I:500\n"

            );

        n = p.rule();

        out.str("");
        out << *n;
        REQUIRE_TREES(out.str(),

"COMPONENT_VALUE\n"
"  IDENTIFIER \"div\"\n"
"  HASH \"op\"\n"
"  OPEN_CURLYBRACKET\n"
"    IDENTIFIER \"color\"\n"
"    COLON\n"
"    FUNCTION \"hsl\"\n"
"      INTEGER \"\" I:120\n"
"      COMMA\n"
"      INTEGER \"\" I:1\n"
"      COMMA\n"
"      DECIMAL_NUMBER \"\" D:0.5\n"

            );

    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid Component Values", "[parser] [component-value] [invalid]")
{
    // breaks on missing }
    {
        std::stringstream ss;
        ss << "body { background : white url( /images/background.png )";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.component_value_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: Block expected to end with CLOSE_CURLYBRACKET but got EOF_TOKEN instead.\n"
            );
    }

    // breaks on missing ]
    {
        std::stringstream ss;
        ss << "body[lili=\"joe\" { background : white url( /images/background.png ) } }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.component_value_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: Block expected to end with CLOSE_SQUAREBRACKET but got CLOSE_CURLYBRACKET instead.\n"
            );
    }

    // breaks on missing )
    {
        std::stringstream ss;
        ss << " body[lili=\"joe\"] { background : white url( /images/background.png ); color:rgba(0,0,0,0; } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.component_value_list());

//std::cerr << "Result is: [" << *n << "]\n";

        // this failed with an error, no need to check the "broken" output

        REQUIRE_ERRORS(
                "test.css(1): error: Block expected to end with CLOSE_PARENTHESIS but got CLOSE_CURLYBRACKET instead.\n"
            );
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Simple One Component Value", "[parser] [component-value]")
{
    // a simple valid rule
    {
        std::stringstream ss;
        ss << " body { background : gradient(to bottom, #012, #384513 75%, #452) }"
           << " @media screen { viewport: 1000px 500px; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        char const *results[] =
        {
            "WHITESPACE\n",

            "IDENTIFIER \"body\"\n",

            "WHITESPACE\n",

            "OPEN_CURLYBRACKET\n"
            "  IDENTIFIER \"background\"\n"
            "  WHITESPACE\n"
            "  COLON\n"
            "  WHITESPACE\n"
            "  FUNCTION \"gradient\"\n"
            "    IDENTIFIER \"to\"\n"
            "    WHITESPACE\n"
            "    IDENTIFIER \"bottom\"\n"
            "    COMMA\n"
            "    WHITESPACE\n"
            "    HASH \"012\"\n"
            "    COMMA\n"
            "    WHITESPACE\n"
            "    HASH \"384513\"\n"
            "    WHITESPACE\n"
            "    PERCENT D:0.75\n"
            "    COMMA\n"
            "    WHITESPACE\n"
            "    HASH \"452\"\n",

            "WHITESPACE\n",

            "AT_KEYWORD \"media\" I:0\n",

            "WHITESPACE\n",

            "IDENTIFIER \"screen\"\n",

            "WHITESPACE\n",

            "OPEN_CURLYBRACKET\n"
            "  IDENTIFIER \"viewport\"\n"
            "  COLON\n"
            "  WHITESPACE\n"
            "  INTEGER \"px\" I:1000\n"
            "  WHITESPACE\n"
            "  INTEGER \"px\" I:500\n",

            // make sure to keep the following to make sure we got everything
            // through the parser
            "EOF_TOKEN\n"
        };

        for(size_t i(0); i < sizeof(results) / sizeof(results[0]); ++i)
        {
            csspp::node::pointer_t n(p.component_value());
            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(), results[i]);
            csspp_test::compare(out.str(), results[i], __FILE__, i + 1);
        }

    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid One Component Value", "[parser] [component-value] [invalid]")
{
    // breaks on missing }
    {
        std::stringstream ss;
        ss << "body { background : 123";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        char const *results[] =
        {
            "IDENTIFIER \"body\"\n",

            "WHITESPACE\n",

            "OPEN_CURLYBRACKET\n"
            "  IDENTIFIER \"background\"\n"
            "  WHITESPACE\n"
            "  COLON\n"
            "  WHITESPACE\n"
            "  INTEGER \"\" I:123\n",

            // make sure to keep the following to make sure we got everything
            // through the parser
            "EOF_TOKEN\n"
        };

        for(size_t i(0); i < sizeof(results) / sizeof(results[0]); ++i)
        {
            csspp::node::pointer_t n(p.component_value());
            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(), results[i]);
            csspp_test::compare(out.str(), results[i], __FILE__, i + 1);
        }

        REQUIRE_ERRORS("test.css(1): error: Block expected to end with CLOSE_CURLYBRACKET but got EOF_TOKEN instead.\n");
    }

    // breaks on missing ]
    {
        std::stringstream ss;
        ss << "body[color='55'";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        char const *results[] =
        {
            "IDENTIFIER \"body\"\n",

            "OPEN_SQUAREBRACKET\n"
            "  IDENTIFIER \"color\"\n"
            "  EQUAL\n"
            "  STRING \"55\"\n",

            // make sure to keep the following to make sure we got everything
            // through the parser
            "EOF_TOKEN\n"
        };

        for(size_t i(0); i < sizeof(results) / sizeof(results[0]); ++i)
        {
            csspp::node::pointer_t n(p.component_value());
            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(), results[i]);
            csspp_test::compare(out.str(), results[i], __FILE__, i + 1);
        }

        REQUIRE_ERRORS("test.css(1): error: Block expected to end with CLOSE_SQUAREBRACKET but got EOF_TOKEN instead.\n");
    }

    // breaks on missing )
    {
        std::stringstream ss;
        ss << "body{color:rgba(1,2}";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        char const *results[] =
        {
            "IDENTIFIER \"body\"\n",

            "OPEN_CURLYBRACKET\n"
            "  IDENTIFIER \"color\"\n"
            "  COLON\n"
            "  FUNCTION \"rgba\"\n"
            "    INTEGER \"\" I:1\n"
            "    COMMA\n"
            "    INTEGER \"\" I:2\n",

            // make sure to keep the following to make sure we got everything
            // through the parser
            "EOF_TOKEN\n"
        };

        for(size_t i(0); i < sizeof(results) / sizeof(results[0]); ++i)
        {
            csspp::node::pointer_t n(p.component_value());
            std::stringstream out;
            out << *n;
            REQUIRE_TREES(out.str(), results[i]);
            csspp_test::compare(out.str(), results[i], __FILE__, i + 1);
        }

        REQUIRE_ERRORS("test.css(1): error: Block expected to end with CLOSE_PARENTHESIS but got CLOSE_CURLYBRACKET instead.\n");
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Simple Declarations", "[parser] [declaration]")
{
    // a simple valid declaration
    {
        std::stringstream ss;
        ss << " background : gradient(to bottom, #012, #384513 75%, #452) { width: 300px } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  DECLARATION \"background\"\n"
"    COMPONENT_VALUE\n"
"      FUNCTION \"gradient\"\n"
"        IDENTIFIER \"to\"\n"
"        WHITESPACE\n"
"        IDENTIFIER \"bottom\"\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"012\"\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"384513\"\n"
"        WHITESPACE\n"
"        PERCENT D:0.75\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"452\"\n"
"      OPEN_CURLYBRACKET\n"
"        IDENTIFIER \"width\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        INTEGER \"px\" I:300\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // a @-rule in a declaration
    {
        std::stringstream ss;
        ss << " @enhanced capabilities { background : gradient(to bottom, #012, #384513 75%, #452) } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  AT_KEYWORD \"enhanced\" I:0\n"
"    IDENTIFIER \"capabilities\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"background\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      WHITESPACE\n"
"      FUNCTION \"gradient\"\n"
"        IDENTIFIER \"to\"\n"
"        WHITESPACE\n"
"        IDENTIFIER \"bottom\"\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"012\"\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"384513\"\n"
"        WHITESPACE\n"
"        PERCENT D:0.75\n"
"        COMMA\n"
"        WHITESPACE\n"
"        HASH \"452\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // multiple declarations require a ';'
    {
        std::stringstream ss;
        ss << "a: 33px; b: 66px; c: 123px";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  DECLARATION \"a\"\n"
"    COMPONENT_VALUE\n"
"      INTEGER \"px\" I:33\n"
"  DECLARATION \"b\"\n"
"    COMPONENT_VALUE\n"
"      INTEGER \"px\" I:66\n"
"  DECLARATION \"c\"\n"
"    COMPONENT_VALUE\n"
"      INTEGER \"px\" I:123\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }

    // multiple declarations require a ';'
    {
        std::stringstream ss;
        ss << "a: 33px ! important ; b: 66px !global ; c: 123px 55em !import";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  DECLARATION \"a\"\n"
"    COMPONENT_VALUE\n"
"      INTEGER \"px\" I:33\n"
"    EXCLAMATION \"important\"\n"
"  DECLARATION \"b\"\n"
"    COMPONENT_VALUE\n"
"      INTEGER \"px\" I:66\n"
"    EXCLAMATION \"global\"\n"
"  DECLARATION \"c\"\n"
"    COMPONENT_VALUE\n"
"      INTEGER \"px\" I:123\n"
"      WHITESPACE\n"
"      INTEGER \"em\" I:55\n"
"    EXCLAMATION \"import\"\n"

            );

        // no error left over
        REQUIRE_ERRORS("");
    }
}

TEST_CASE("Invalid Declarations", "[parser] [declaration] [invalid]")
{
    // declarations must end with EOF
    {
        std::stringstream ss;
        ss << " background : gradient(to bottom, #012, #384513 75%, #452) { width: 300px } <!--";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        REQUIRE_ERRORS("test.css(1): error: the end of the stream was not reached in this declaration, we stopped on a CDO.\n");
    }

    // declarations missing a ':'
    {
        std::stringstream ss;
        ss << " background gradient(to bottom, #012, #384513 75%, #452) { width: 300px } ";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        REQUIRE_ERRORS(
                "test.css(1): error: ':' missing in your declaration starting with \"background\".\n"
            );
    }

    // '!' without an identifier
    {
        std::stringstream ss;
        ss << "background: !gradient(to bottom, #012, #384513 75%, #452)";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        // rule list does not like <!-- and -->
        csspp::node::pointer_t n(p.declaration_list());

//std::cerr << "Result is: [" << *n << "]\n";

        REQUIRE_ERRORS(
                "test.css(1): error: A '!' must be followed by an identifier, got a FUNCTION instead.\n"
                "test.css(1): error: the end of the stream was not reached in this declaration, we stopped on a FUNCTION.\n"
            );
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Multi-line, multi-level stylesheet", "[parser] [rules]")
{
    {
        std::stringstream ss;
        ss << "body { background : white url( /images/background.png ) }"
              "div.power-house { !important margin: 0; color: red ; }"
              "a { text-decoration: none; }"
              "$green: #080;"
              "#doll { background-color: $green; &:hover { color: teal; } }"
              "@supports (background-color and border-radius) or (background-image) { body > E ~ F + G H { font-style: italic } }"
           ;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"body\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"background\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"white\"\n"
"      WHITESPACE\n"
"      URL \"/images/background.png\"\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"div\"\n"
"    PERIOD\n"
"    IDENTIFIER \"power-house\"\n"
"    OPEN_CURLYBRACKET\n"
"      COMPONENT_VALUE\n"
"        EXCLAMATION \"\"\n"
"        IDENTIFIER \"important\"\n"
"        WHITESPACE\n"
"        IDENTIFIER \"margin\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        INTEGER \"\" I:0\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"color\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        IDENTIFIER \"red\"\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"a\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"text-decoration\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"none\"\n"
"  COMPONENT_VALUE\n"
"    VARIABLE \"green\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    HASH \"080\"\n"
"  COMPONENT_VALUE\n"
"    HASH \"doll\"\n"
"    OPEN_CURLYBRACKET\n"
"      COMPONENT_VALUE\n"
"        IDENTIFIER \"background-color\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        VARIABLE \"green\"\n"
"      COMPONENT_VALUE\n"
"        REFERENCE\n"
"        COLON\n"
"        IDENTIFIER \"hover\"\n"
"        OPEN_CURLYBRACKET\n"
"          IDENTIFIER \"color\"\n"
"          COLON\n"
"          WHITESPACE\n"
"          IDENTIFIER \"teal\"\n"
"  AT_KEYWORD \"supports\" I:0\n"
"    OPEN_PARENTHESIS\n"
"      IDENTIFIER \"background-color\"\n"
"      WHITESPACE\n"
"      IDENTIFIER \"and\"\n"
"      WHITESPACE\n"
"      IDENTIFIER \"border-radius\"\n"
"    WHITESPACE\n"
"    IDENTIFIER \"or\"\n"
"    OPEN_PARENTHESIS\n"
"      IDENTIFIER \"background-image\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"body\"\n"
"      WHITESPACE\n"
"      GREATER_THAN\n"
"      WHITESPACE\n"
"      IDENTIFIER \"e\"\n"
"      WHITESPACE\n"
"      PRECEDED\n"
"      WHITESPACE\n"
"      IDENTIFIER \"f\"\n"
"      WHITESPACE\n"
"      ADD\n"
"      WHITESPACE\n"
"      IDENTIFIER \"g\"\n"
"      WHITESPACE\n"
"      IDENTIFIER \"h\"\n"
"      OPEN_CURLYBRACKET\n"
"        IDENTIFIER \"font-style\"\n"
"        COLON\n"
"        WHITESPACE\n"
"        IDENTIFIER \"italic\"\n"

            );
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Is Variable Set", "[parser] [variable] [invalid]")
{
    // simple test with a value + value (SASS compatible)
    {
        std::stringstream ss;
        ss << "$a: 33px;";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    VARIABLE \"a\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    INTEGER \"px\" I:33\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE(csspp::parser::is_variable_set(var, false));
        REQUIRE_FALSE(csspp::parser::is_variable_set(var, true));
    }

    // case were we actually use a variable to define a selector
    // this is not a variable set
    {
        std::stringstream ss;
        ss << "$a .cute { color: red; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    VARIABLE \"a\"\n"
"    WHITESPACE\n"
"    PERIOD\n"
"    IDENTIFIER \"cute\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"red\"\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE_FALSE(csspp::parser::is_variable_set(var, false));
        REQUIRE_FALSE(csspp::parser::is_variable_set(var, true));
    }

    // test with a variable block
    {
        std::stringstream ss;
        ss << "$a: { color: red; };";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    VARIABLE \"a\"\n"
"    COLON\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"red\"\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE(csspp::parser::is_variable_set(var, false));
        REQUIRE(csspp::parser::is_variable_set(var, true));
    }

    // test with the missing ';'
    {
        std::stringstream ss;
        ss << "$a: { color: red; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // the ';' at the end is missing
        REQUIRE_ERRORS("test.css(1): error: Variable set to a block and a nested property block must end with a semicolon (;) after said block.\n");
    }

    // simple test with a value + value (SASS compatible)
    {
        std::stringstream ss;
        ss << "$a($arg1): 33px;";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    VARIABLE_FUNCTION \"a\"\n"
"      VARIABLE \"arg1\"\n"
"    COLON\n"
"    WHITESPACE\n"
"    INTEGER \"px\" I:33\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE(csspp::parser::is_variable_set(var, false));
        REQUIRE_FALSE(csspp::parser::is_variable_set(var, true));
    }

    // case were we actually use a variable to define a selector
    // this is not a variable set
    {
        std::stringstream ss;
        ss << "$a(33) .cute { color: red; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    VARIABLE_FUNCTION \"a\"\n"
"      INTEGER \"\" I:33\n"
"    WHITESPACE\n"
"    PERIOD\n"
"    IDENTIFIER \"cute\"\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"red\"\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE_FALSE(csspp::parser::is_variable_set(var, false));
        REQUIRE_FALSE(csspp::parser::is_variable_set(var, true));
    }

    // test with a variable block
    {
        std::stringstream ss;
        ss << "$a($arg1): { color: red; };";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    VARIABLE_FUNCTION \"a\"\n"
"      VARIABLE \"arg1\"\n"
"    COLON\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"red\"\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE(csspp::parser::is_variable_set(var, false));
        REQUIRE(csspp::parser::is_variable_set(var, true));
    }

    // test with the missing ';'
    {
        std::stringstream ss;
        ss << "$a($arg1): { color: red; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // the ';' at the end is missing
        REQUIRE_ERRORS("test.css(1): error: Variable set to a block and a nested property block must end with a semicolon (;) after said block.\n");
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Is Nested Declaration", "[parser] [variable] [invalid]")
{
    // which a field name with a simple nested declaration
    {
        std::stringstream ss;
        ss << "width : { color : red } ;";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("");

//std::cerr << "Result is: [" << *n << "]\n";

        std::stringstream out;
        out << *n;
        REQUIRE_TREES(out.str(),

"LIST\n"
"  COMPONENT_VALUE\n"
"    IDENTIFIER \"width\"\n"
"    WHITESPACE\n"
"    COLON\n"
"    OPEN_CURLYBRACKET\n"
"      IDENTIFIER \"color\"\n"
"      WHITESPACE\n"
"      COLON\n"
"      WHITESPACE\n"
"      IDENTIFIER \"red\"\n"

            );

        csspp::node::pointer_t var(n->get_child(0));
        REQUIRE(csspp::parser::is_nested_declaration(var));
    }

    // a nested block must end with a ';'
    {
        std::stringstream ss;
        ss << "width : { color : red }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        // no error happened
        REQUIRE_ERRORS("test.css(1): error: Variable set to a block and a nested property block must end with a semicolon (;) after said block.\n");
    }

    // test special cases which woudl be really hard to get from the
    // normal parser/lexer combo
    for(int i(0); i < (1 << 5); ++i)
    {
        csspp::position pos("test.css");
        csspp::node::pointer_t root(new csspp::node(csspp::node_type_t::LIST, pos));

        // name WS ':' WS '{'
        if((i & (1 << 0)) != 0)
        {
            csspp::node::pointer_t name(new csspp::node(csspp::node_type_t::IDENTIFIER, pos));
            name->set_string("field-name");
            root->add_child(name);
        }

        if((i & (1 << 1)) != 0)
        {
            csspp::node::pointer_t whitespace1(new csspp::node(csspp::node_type_t::WHITESPACE, pos));
            root->add_child(whitespace1);
        }

        if((i & (1 << 2)) != 0)
        {
            csspp::node::pointer_t colon(new csspp::node(csspp::node_type_t::COLON, pos));
            root->add_child(colon);
        }

        if((i & (1 << 3)) != 0)
        {
            csspp::node::pointer_t whitespace2(new csspp::node(csspp::node_type_t::WHITESPACE, pos));
            root->add_child(whitespace2);
        }

        if((i & (1 << 4)) != 0)
        {
            csspp::node::pointer_t curlybracket(new csspp::node(csspp::node_type_t::OPEN_CURLYBRACKET, pos));
            root->add_child(curlybracket);
        }

        // this one is "valid"
        switch(i)
        {
        case 0x1F: // all with and without spaces are valid
        case 0x1D:
        case 0x17:
        case 0x15:
            REQUIRE(csspp::parser::is_nested_declaration(root));
            break;

        default:
            REQUIRE_FALSE(csspp::parser::is_nested_declaration(root));
            break;

        }
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
