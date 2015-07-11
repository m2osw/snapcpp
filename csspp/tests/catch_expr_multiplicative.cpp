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
 * \brief Test the expression.cpp file: "*", "/", and "%" operators.
 *
 * This test runs a battery of tests agains the expression.cpp "*", "/",
 * and "%" operators to ensure full coverage and that all possible left
 * hand side and right hand side types are checked for the multiplicative
 * CSS Preprocessor extensions.
 *
 * Note that all the tests use the full chain: lexer, parser, compiler,
 * and assembler to make sure the results are correct. So these tests
 * exercise the assembler even more than the assembler tests, except that
 * it only checks that compressed results are correct instead of all
 * output modes, since its only goal is covering all the possible
 * expression cases and not the assembler, compiler, parser, and lexer
 * classes.
 */

#include "catch_tests.h"

#include "csspp/assembler.h"
#include "csspp/compiler.h"
#include "csspp/exceptions.h"
#include "csspp/parser.h"

#include <sstream>

TEST_CASE("Expression integer *,/,% integer", "[expression] [multiplicative]")
{
    SECTION("multiple sizes without dimensions (*)")
    {
        std::stringstream ss;
        ss << "div { z-index: 3 * 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:30}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("multiple sizes without dimensions (mul)")
    {
        std::stringstream ss;
        ss << "div { z-index: 3 mul 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:30}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide sizes without dimensions (/)")
    {
        std::stringstream ss;
        ss << "div { z-index: 10 / 3; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide sizes without dimensions (div)")
    {
        std::stringstream ss;
        ss << "div { z-index: 10 div 3; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo sizes without dimensions (%)")
    {
        std::stringstream ss;
        ss << "div { z-index: 10 % 3; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:1\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:1}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo sizes without dimensions (mod)")
    {
        std::stringstream ss;
        ss << "div { z-index: 10 mod 3; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:1\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:1}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("remove dimension (simple divide)")
    {
        std::stringstream ss;
        ss << "div { width: 3px / 1px; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"\" I:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("convert px to em (multiple + divide)")
    {
        std::stringstream ss;
        ss << "div { width: 3px * 10em / 1px; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"em\" I:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:30em}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("operation going through a unit less dividend")
    {
        std::stringstream ss;
        ss << "div { width: 30px / 1px / 10 * 10cm; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"cm\" I:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:30cm}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("complex operations with unitless dividend")
    {
        std::stringstream ss;
        ss << "div { width: 3px * 10em / 1px / 5em / 2vw * 3em * 5vw; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"em\" I:45\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:45em}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("many divisions with many dimension, then multiplications")
    {
        std::stringstream ss;
        ss << "div { width: 60 / 5px / 3em / 2vw * 3em * 5vw * 2px * 3px; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"px\" I:180\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:180px}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("many multiplications and divisions with differen dimensions")
    {
        std::stringstream ss;
        // first we get "deg * px * em * vw / pt * rad * cm * mm" then
        // we clean that up and get 1
        ss << "div { width: (60deg * 5px * 3em * 2vw / 3pt / 5rad / 2cm / 3mm) / (60deg * 5px * 3em * 2vw / 3pt / 5rad / 2cm / 3mm); }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Parser result is: [" << *n << "]\n";

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"\" I:1\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:1}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo of a distance, require same dimension")
    {
        std::stringstream ss;
        ss << "div { width: 10px % 3px; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"px\" I:1\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(out.str() ==
"div{width:1px}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression string * integer", "[expression] [multiplicative] [string]")
{
    // duplicate a string (string x integer)
    {
        std::stringstream ss;
        ss << "div::before { content: \"str\" * 3; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"      COLON\n"
"      COLON\n"
"      IDENTIFIER \"before\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"content\"\n"
"        ARG\n"
"          STRING \"strstrstr\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div::before{content:\"strstrstr\"}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // duplicate a string (integer x string)
    {
        std::stringstream ss;
        ss << "div::before { content: 3 * \"str\"; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"      COLON\n"
"      COLON\n"
"      IDENTIFIER \"before\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"content\"\n"
"        ARG\n"
"          STRING \"strstrstr\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div::before{content:\"strstrstr\"}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // duplicate a string (string x 0)
    {
        std::stringstream ss;
        ss << "div::before { content: \"str\" * 0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"      COLON\n"
"      COLON\n"
"      IDENTIFIER \"before\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"content\"\n"
"        ARG\n"
"          STRING \"\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div::before{content:\"\"}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // duplicate a string (0 x string)
    {
        std::stringstream ss;
        ss << "div::before { content: 0 * \"str\"; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"      COLON\n"
"      COLON\n"
"      IDENTIFIER \"before\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"content\"\n"
"        ARG\n"
"          STRING \"\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div::before{content:\"\"}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression integer *,/,% integer with incompatible dimensions", "[expression] [multiplicative] [invalid]")
{
    // px * px -- cannot output
    {
        std::stringstream ss;
        ss << "div { width: 3px * 10px; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"px * px\" I:30\n"
+ csspp_test::get_close_comment(true)

            );

        // no errors so far
        REQUIRE_ERRORS("");

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is [" << out.str() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: \"px * px\" is not a valid CSS dimension.\n");

        REQUIRE(c.get_root() == n);
    }

    // px / em -- cannot output
    {
        std::stringstream ss;
        ss << "div { width: 10px / 3em; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"width\"\n"
"        ARG\n"
"          INTEGER \"px / em\" I:3\n"
+ csspp_test::get_close_comment(true)

            );

        // no errors so far
        REQUIRE_ERRORS("");

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is [" << out.str() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: \"px / em\" is not a valid CSS dimension.\n");

        REQUIRE(c.get_root() == n);
    }

    // px % em -- cannot calculate
    {
        std::stringstream ss;
        ss << "div { width: 10px % 3em; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible dimensions (\"px\" and \"em\") cannot be used with operator '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string * -integer
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" * -2; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: string * integer requires that the integer not be negative (-2).\n");

        REQUIRE(c.get_root() == n);
    }

    // string / integer
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" / 3; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and INTEGER for operator '/' or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string % integer
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" % 5; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and INTEGER for operator '/' or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string * decimal-number
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" * 3.14; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and DECIMAL_NUMBER for operator '*', '/', or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string * percent
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" / 31%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and PERCENT for operator '*', '/', or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string * unicode-range
    {
        std::stringstream ss;
        ss << "div { width: U+5?? % \"rhs\"; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between UNICODE_RANGE and STRING for operator '*', '/', or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string * string
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" * 'rhs'; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and STRING for operator '*', '/', or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string / string
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" / 'rhs'; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and STRING for operator '*', '/', or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // string % string
    {
        std::stringstream ss;
        ss << "div { width: \"lhs\" % 'rhs'; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between STRING and STRING for operator '*', '/', or '%'.\n");

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression multiplicative errors", "[expression] [multiplicative] [invalid]")
{
    // an invalid unary value generates an error caught in additive
    {
        std::stringstream ss;
        ss << "div { width: ?; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: unsupported type CONDITIONAL as a unary expression token.\n");

        REQUIRE(c.get_root() == n);
    }

    // an invalid unary on the right side of the operator
    {
        std::stringstream ss;
        ss << "div { width: 3 + ?; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: unsupported type CONDITIONAL as a unary expression token.\n");

        REQUIRE(c.get_root() == n);
    }

    // cannot add a unicode range with anything
    {
        std::stringstream ss;
        ss << "div { width: 3 + U+4??; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("test.css(1): error: incompatible types between INTEGER and UNICODE_RANGE for operator '+'.\n");

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression decimal number or integer *,/,% decimal number or integer", "[expression] [multiplicative]")
{
    SECTION("multiply two decimal numbers")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.5 * 10.2; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:35.7\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:35.7}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("multiply an integer with a decimal number")
    {
        std::stringstream ss;
        ss << "div { z-index: 3 * 10.2; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:30.6\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:30.6}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("multiply a decimal number with an integer")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.5 * 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:35\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:35}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("multiply decimal numbers with 0 in their fraction")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.0 * 10.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:30}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("multiply an integer and a decimal number with 0 in their fraction")
    {
        std::stringstream ss;
        ss << "div { z-index: 3 * 10.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:30}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("multiply a decimal number with 0 in their fraction and an integer")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.0 * 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:30\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:30}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide two decimal numbers")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.2 / 12.5; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:0.256\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:.256}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide an integer with a decimal number")
    {
        std::stringstream ss;
        ss << "div { z-index: 3 / 12.5; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:0.24\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:.24}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide a decimal number with an integer")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.5 / 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:0.35\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:.35}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide decimal numbers with 0 in their fraction")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.0 / 10.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:0.3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:.3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide an integer and a decimal number with 0 in their fraction")
    {
        std::stringstream ss;
        ss << "div { z-index: 350 / 10.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:35\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:35}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("divide a decimal number with 0 in their fraction and an integer")
    {
        std::stringstream ss;
        ss << "div { z-index: 30.0 / 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo two decimal numbers")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.5 % 10.2; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3.5\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3.5}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo an integer with a decimal number")
    {
        std::stringstream ss;
        ss << "div { z-index: 33 % 10.2; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:2.4\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:2.4}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo a decimal number with an integer")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.5 % 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3.5\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3.5}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo decimal numbers with 0 in their fraction")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.0 % 10.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo an integer and a decimal number with 0 in their fraction")
    {
        std::stringstream ss;
        ss << "div { z-index: 3 % 10.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("modulo a decimal number with 0 in their fraction and an integer")
    {
        std::stringstream ss;
        ss << "div { z-index: 3.0 % 10; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{z-index:3}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression percent *,/,% percent", "[expression] [multiplicative]")
{
    SECTION("percent multiplication")
    {
        std::stringstream ss;
        ss << "div { height: 3.5% * 10.2%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.00357\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:.357%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent multiplication with what looks like an integer (lhs)")
    {
        std::stringstream ss;
        ss << "div { height: 3% * 10.2%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.00306\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:.306%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent multiplication with what looks like an integer (rhs)")
    {
        std::stringstream ss;
        ss << "div { height: 3.5% * 10%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.0035\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:.35%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent division")
    {
        std::stringstream ss;
        ss << "div { height: 3.5% / 12.5%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.28\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:28%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent division with what looks like an integer (lhs)")
    {
        std::stringstream ss;
        ss << "div { height: 3% / 12.5%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.24\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:24%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent division with what looks like an integer (rhs)")
    {
        std::stringstream ss;
        ss << "div { height: 3.5% / 10%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.35\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:35%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent modulo")
    {
        std::stringstream ss;
        ss << "div { height: 13.5% mod 12.5%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.01\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:1%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent modulo with what looks like an integer (lhs)")
    {
        std::stringstream ss;
        ss << "div { height: 23% mod 12.5%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.105\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:10.5%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("percent modulo with what looks like an integer (rhs)")
    {
        std::stringstream ss;
        ss << "div { height: 3.5% mod 10%; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"height\"\n"
"        ARG\n"
"          PERCENT D:0.035\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{height:3.5%}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression with a font metrics", "[expression] [multiplicative] [font-metrics]")
{
    SECTION("Not a division, two integers")
    {
        std::stringstream ss;
        ss << "div { font: 35pt/40pt serif; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"font\"\n"
"        ARG\n"
"          FONT_METRICS FM:35pt/40pt\n"
"          WHITESPACE\n"
"          IDENTIFIER \"serif\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{font:35pt/40pt serif}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("Not a division, integer and percent")
    {
        std::stringstream ss;
        ss << "div { font: 35pt/120% sans-serif; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"font\"\n"
"        ARG\n"
"          FONT_METRICS FM:35pt/120%\n"
"          WHITESPACE\n"
"          IDENTIFIER \"sans-serif\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{font:35pt/120% sans-serif}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("Not a division, spaces and percent twice")
    {
        std::stringstream ss;
        ss << "div { font: 80% / 120% sans-serif; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        // to verify that the result is still an INTEGER we have to
        // test the root node here
        std::stringstream compiler_out;
        compiler_out << *n;
        REQUIRE_TREES(compiler_out.str(),

"LIST\n"
+ csspp_test::get_default_variables() +
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"font\"\n"
"        ARG\n"
"          FONT_METRICS FM:80%/120%\n"
"          WHITESPACE\n"
"          IDENTIFIER \"sans-serif\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==
"div{font:80%/120% sans-serif}\n"
+ csspp_test::get_close_comment()
                );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Division by zero", "[expression] [multiplicative] [invalid]")
{
    SECTION("integer division zero")
    {
        std::stringstream ss;
        ss << "div { z-index: 55 / 0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("test.css(1): error: division by zero.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("integer modulo zero")
    {
        std::stringstream ss;
        ss << "div { z-index: 55 % 0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("test.css(1): error: modulo by zero.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("decimal number division zero")
    {
        std::stringstream ss;
        ss << "div { z-index: 5.5 / 0.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("test.css(1): error: division by zero.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("decimal number modulo zero")
    {
        std::stringstream ss;
        ss << "div { z-index: 5.5 % 0.0; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        REQUIRE_ERRORS("");

        csspp::compiler c;
        c.set_root(n);
        c.set_date_time_variables(csspp_test::get_now());
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

        REQUIRE_ERRORS("test.css(1): error: modulo by zero.\n");

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression [in]valid *,/,% [in]valid", "[expression] [multiplicative] [invalid]")
{
    char const *valid[] =
    {
        "55",
        "4.78",
        "33.33%",
        "+515",
        "+3.79",
        "+7.77%",
        "-41",
        "-7.245",
        "-5%",
        "U+11??",
        "null",
        "true",
        "false",
        "other",
        "\"string\"",
        "#f09432"
    };

    char const *op[] =
    {
        "*",
        "/",
        "%"
    };

    char const *invalid[] =
    {
        "?",        "CONDITIONAL",
        ":",        "COLON",
        "[3]",      "OPEN_SQUAREBRACKET"
    };

    SECTION("Valid *,/,% invalid AND Invalid *,/,% valid")
    {
        for(size_t i(0); i < sizeof(valid) / sizeof(valid[0]); ++i)
        {
            for(size_t j(0); j < sizeof(op) / sizeof(op[0]); ++j)
            {
                for(size_t k(0); k < sizeof(invalid) / sizeof(invalid[0]); k += 2)
                {
                    {
                        std::stringstream ss;
                        ss << "div { width: "
                           << valid[i]
                           << " "
                           << op[j]
                           << " "
                           << invalid[k]
                           << "; }";
                        csspp::position pos("test.css");
                        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

                        csspp::parser p(l);

                        csspp::node::pointer_t n(p.stylesheet());

                        // no errors so far
                        REQUIRE_ERRORS("");

                        csspp::compiler c;
                        c.set_root(n);
                        c.set_date_time_variables(csspp_test::get_now());
                        c.add_path(csspp_test::get_script_path());
                        c.add_path(csspp_test::get_version_script_path());

                        c.compile(false);

                        std::stringstream errmsg;
                        errmsg << "test.css(1): error: unsupported type "
                               << invalid[k + 1]
                               << " as a unary expression token.\n";
                        REQUIRE_ERRORS(errmsg.str());

                        REQUIRE(c.get_root() == n);
                    }

                    // well, this one does not actually include the operator
                    // to make it easier for testing error messages, but
                    // then it's not really a multiplicative test per se
                    {
                        std::stringstream ss;
                        ss << "div { width: "
                           << invalid[k]
                           //<< " "
                           //<< op[j]
                           //<< " "
                           //<< valid[i]
                           << "; }";
                        csspp::position pos("test.css");
                        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

                        csspp::parser p(l);

                        csspp::node::pointer_t n(p.stylesheet());

                        // no errors so far
                        REQUIRE_ERRORS("");

                        csspp::compiler c;
                        c.set_root(n);
                        c.set_date_time_variables(csspp_test::get_now());
                        c.add_path(csspp_test::get_script_path());
                        c.add_path(csspp_test::get_version_script_path());

                        c.compile(false);

                        std::stringstream errmsg;
                        errmsg << "test.css(1): error: unsupported type "
                               << invalid[k + 1]
                               << " as a unary expression token.\n";
                        REQUIRE_ERRORS(errmsg.str());

                        REQUIRE(c.get_root() == n);
                    }
                }
            }
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
