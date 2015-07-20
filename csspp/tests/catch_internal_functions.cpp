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
 * \brief Test the internal_functions.cpp file.
 *
 * This test runs a battery of tests agains internal_functions.cpp
 * to ensure full coverage and that all the internal functions are
 * checked for the equality CSS Preprocessor extensions.
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

TEST_CASE("Expression cos()/sin()/tan()", "[expression] [internal-functions] [cos] [sin] [tan]")
{
    SECTION("cos(pi)")
    {
        for(int angle(-180); angle <= 180; angle += rand() % 25 + 1)
        {
            // unspecified (defaults to degrees)
            {
                std::stringstream ss;
                ss << "div { z-index: cos("
                   << angle
                   << "); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // degrees
            {
                std::stringstream ss;
                ss << "div { z-index: cos("
                   << angle
                   << "deg); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // radians
            {
                std::stringstream ss;
                ss << "div { z-index: cos("
                   << angle * M_PI / 180.0
                   << "rad); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // gradians
            {
                std::stringstream ss;
                ss << "div { z-index: cos("
                   << angle * 200 / 180.0
                   << "grad); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // turns
            {
                std::stringstream ss;
                ss << "div { z-index: cos("
                   << angle / 360.0
                   << "turn); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(cos(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }
        }
    }

    SECTION("sin(pi)")
    {
        for(int angle(-180); angle <= 180; angle += rand() % 12)
        {
            // unspecified (defaults to degrees)
            {
                std::stringstream ss;
                ss << "div { z-index: sin("
                   << angle
                   << "); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // degrees
            {
                std::stringstream ss;
                ss << "div { z-index: sin("
                   << angle
                   << "deg); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // radians
            {
                std::stringstream ss;
                ss << "div { z-index: sin("
                   << angle * M_PI / 180.0
                   << "rad); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // gradians
            {
                std::stringstream ss;
                ss << "div { z-index: sin("
                   << angle * 200 / 180.0
                   << "grad); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // turns
            {
                std::stringstream ss;
                ss << "div { z-index: sin("
                   << angle / 360.0
                   << "turn); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(sin(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }
        }
    }

    SECTION("tan(pi)")
    {
        for(int angle(-180); angle <= 180; angle += rand() % 12)
        {

            // unspecified (defaults to degrees)
            {
                std::stringstream ss;
                ss << "div { z-index: tan("
                   << angle
                   << "); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(tan(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(tan(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // degrees
            {
                std::stringstream ss;
                ss << "div { z-index: tan("
                   << angle
                   << "deg); }";
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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(tan(angle * M_PI / 180.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(tan(angle * M_PI / 180.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // radians
            {
                std::stringstream rad;
                rad << angle * M_PI / 180.0;
                std::stringstream ss;
                ss << "div { z-index: tan("
                   << rad.str()
                   << "rad); }";
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

                std::string const r(rad.str());
                csspp::decimal_number_t rd(atof(r.c_str()));

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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(tan(rd), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(tan(rd), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // gradians
            {
                std::stringstream grad;
                grad << angle * 200.0 / 180.0;
                std::stringstream ss;
                ss << "div { z-index: tan("
                   << grad.str()
                   << "grad); }";
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

                std::string const g(grad.str());
                csspp::decimal_number_t gd(atof(g.c_str()));

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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(tan(gd * M_PI / 200.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(tan(gd * M_PI / 200.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // turns
            {
                std::stringstream turn;
                turn << angle / 360.0;
                std::stringstream ss;
                ss << "div { z-index: tan("
                   << turn.str()
                   << "turn); }";
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

                std::string const t(turn.str());
                csspp::decimal_number_t tn(atof(t.c_str()));

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
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(tan(tn * M_PI * 2.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(tan(tn * M_PI * 2.0), true) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression acos()/asin()/atan()", "[expression] [internal-functions] [acos] [asin] [atan]")
{
    SECTION("acos(ratio)")
    {
        for(int angle(-180); angle <= 180; angle += rand() % 25 + 1)
        {
            std::stringstream ss;
            ss << "div { z-index: acos("
               << cos(angle * M_PI / 180.0)
               << "rad); }";
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
"          DECIMAL_NUMBER \"rad\" D:" + csspp::decimal_number_to_string(labs(angle) * M_PI / 180.0, false) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(labs(angle) * M_PI / 180.0, true) + "rad}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }

        // another test with an integer
        {
            std::stringstream ss;
            ss << "div { z-index: acos(2); }";
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
"          DECIMAL_NUMBER \"rad\" D:" + csspp::decimal_number_to_string(acos(2.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(acos(2), true) + "rad}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("asin(pi)")
    {
        for(int angle(-180); angle <= 180; angle += rand() % 12)
        {
            std::stringstream ss;
            ss << "div { z-index: asin("
               << sin(angle * M_PI / 180.0)
               << "rad); }";
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
"          DECIMAL_NUMBER \"rad\" D:" + csspp::decimal_number_to_string(asin(sin(angle * M_PI / 180.0)), false) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(asin(sin(angle * M_PI / 180.0)), true) + "rad}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }

        // another test with an integer
        {
            std::stringstream ss;
            ss << "div { z-index: asin(2); }";
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
"          DECIMAL_NUMBER \"rad\" D:" + csspp::decimal_number_to_string(asin(2.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(asin(2), true) + "rad}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("atan(pi)")
    {
        for(int angle(-180); angle <= 180; angle += rand() % 12)
        {
            std::stringstream ss;
            ss << "div { z-index: atan("
               << tan(angle * M_PI / 180.0)
               << "); }";
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
"          DECIMAL_NUMBER \"rad\" D:" + csspp::decimal_number_to_string(atan(tan(angle * M_PI / 180.0)), false) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(atan(tan(angle * M_PI / 180.0)), true) + "rad}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }

        // another test with an integer
        {
            std::stringstream ss;
            ss << "div { z-index: atan(2); }";
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
"          DECIMAL_NUMBER \"rad\" D:" + csspp::decimal_number_to_string(atan(2.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(atan(2), true) + "rad}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression abs()/ceil()/floor()", "[expression] [internal-functions] [abs] [ceil] [floor]")
{
    SECTION("abs(number)")
    {
        for(int number(-10000); number <= 10000; number += rand() % 250 + 1)
        {
            // abs(int)
            {
                std::string const dimension(rand() & 1 ? "cm" : "mm");
                std::stringstream ss;
                ss << "div { width: abs("
                   << number
                   << dimension
                   << "); }";
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
"          INTEGER \"" + dimension + "\" I:" + std::to_string(labs(number)) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{width:") + std::to_string(labs(number)) + dimension + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // abs(float)
            {
                std::string const dimension(rand() & 1 ? "em" : "px");
                std::stringstream ss;
                ss << "div { width: abs("
                   << std::setprecision(6) << std::fixed
                   << (static_cast<csspp::decimal_number_t>(number) / 1000.0)
                   << dimension
                   << "); }";
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
"          DECIMAL_NUMBER \"" + dimension + "\" D:" + csspp::decimal_number_to_string(fabs(static_cast<csspp::decimal_number_t>(number) / 1000.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{width:") + csspp::decimal_number_to_string(fabs(static_cast<csspp::decimal_number_t>(number) / 1000.0), true) + dimension + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }
        }
    }

    SECTION("ceil(number)")
    {
        for(int number(-10000); number <= 10000; number += rand() % 250 + 1)
        {
            // ceil(int)
            {
                std::stringstream ss;
                ss << "div { z-index: ceil("
                   << number
                   << "); }";
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
"          INTEGER \"\" I:" + std::to_string(number) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + std::to_string(number) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // ceil(float)
            {
                std::string const dimension(rand() & 1 ? "deg" : "rad");
                std::stringstream ss;
                ss << "div { z-index: ceil("
                   << std::setprecision(6) << std::fixed
                   << (static_cast<csspp::decimal_number_t>(number) / 1000.0)
                   << dimension
                   << "); }";
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
"          DECIMAL_NUMBER \"" + dimension + "\" D:" + csspp::decimal_number_to_string(ceil(static_cast<csspp::decimal_number_t>(number) / 1000.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + csspp::decimal_number_to_string(ceil(static_cast<csspp::decimal_number_t>(number) / 1000.0), true) + dimension + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }
        }
    }

    SECTION("floor(number)")
    {
        for(int number(-10000); number <= 10000; number += rand() % 250 + 1)
        {
            // floor(int)
            {
                std::stringstream ss;
                ss << "div { z-index: floor("
                   << number
                   << "); }";
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
"          INTEGER \"\" I:" + std::to_string(number) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + std::to_string(number) + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }

            // floor(float)
            {
                std::string const dimension(rand() & 1 ? "em" : "px");
                std::stringstream ss;
                ss << "div { width: floor("
                   << std::setprecision(6) << std::fixed
                   << (static_cast<csspp::decimal_number_t>(number) / 1000.0)
                   << dimension
                   << "); }";
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
"          DECIMAL_NUMBER \"" + dimension + "\" D:" + csspp::decimal_number_to_string(floor(static_cast<csspp::decimal_number_t>(number) / 1000.0), false) + "\n"
+ csspp_test::get_close_comment(true)

                    );

                std::stringstream assembler_out;
                csspp::assembler a(assembler_out);
                a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                REQUIRE(assembler_out.str() ==

std::string("div{width:") + csspp::decimal_number_to_string(floor(static_cast<csspp::decimal_number_t>(number) / 1000.0), true) + dimension + "}\n"
+ csspp_test::get_close_comment()

                        );

                REQUIRE(c.get_root() == n);
            }
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression red()/greeb()/blue()/alpha()", "[expression] [internal-functions] [red] [green] [blue] [alpha]")
{
    SECTION("check color components")
    {
        for(int r(0); r < 256; r += rand() % 100 + 1)
        {
            for(int g(0); g < 256; g += rand() % 100 + 1)
            {
                for(int b(0); b < 256; b += rand() % 100 + 1)
                {
                    for(int alpha(0); alpha < 256; alpha += rand() % 100 + 1)
                    {
                        {
                            std::stringstream ss;
                            ss << "div { z-index: red(rgba("
                               << r
                               << ", "
                               << g
                               << ", "
                               << b
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "span { z-index: green(rgba("
                               << r
                               << ", "
                               << g
                               << ", "
                               << b
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "p { z-index: blue(rgba("
                               << r
                               << ", "
                               << g
                               << ", "
                               << b
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "i { z-index: alpha(rgba("
                               << r / 255.0
                               << ", "
                               << g / 255.0
                               << ", "
                               << b / 255.0
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "div { z-index: red(frgba("
                               << r / 255.0
                               << ", "
                               << g / 255.0
                               << ", "
                               << b / 255.0
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "span { z-index: green(frgba("
                               << r / 255.0
                               << ", "
                               << g / 255.0
                               << ", "
                               << b / 255.0
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "p { z-index: blue(frgba("
                               << r / 255.0
                               << ", "
                               << g / 255.0
                               << ", "
                               << b / 255.0
                               << ", "
                               << alpha / 255.0
                               << ")); }\n"
                               << "i { z-index: alpha(frgba("
                               << r / 255.0
                               << ", "
                               << g / 255.0
                               << ", "
                               << b / 255.0
                               << ", "
                               << alpha / 255.0
                               << ")); }\n";
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
"          INTEGER \"\" I:" + std::to_string(r) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"span\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:" + std::to_string(g) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:" + std::to_string(b) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"i\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(alpha / 255.0, false) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"div\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:" + std::to_string(r) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"span\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:" + std::to_string(g) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:" + std::to_string(b) + "\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"i\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:" + csspp::decimal_number_to_string(alpha / 255.0, false) + "\n"
+ csspp_test::get_close_comment(true)

                                );

                            std::stringstream assembler_out;
                            csspp::assembler a(assembler_out);
                            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + std::to_string(r) + "}"
"span{z-index:" + std::to_string(g) + "}"
"p{z-index:" + std::to_string(b) + "}"
"i{z-index:" + csspp::decimal_number_to_string(alpha / 255.0, true) + "}"
"div{z-index:" + std::to_string(r) + "}"
"span{z-index:" + std::to_string(g) + "}"
"p{z-index:" + std::to_string(b) + "}"
"i{z-index:" + csspp::decimal_number_to_string(alpha / 255.0, true) + "}"
"\n"
+ csspp_test::get_close_comment()

                                    );

                            REQUIRE(c.get_root() == n);
                        }
                    }
                }
            }
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression decimal_number()/integer()/string()/identifier()", "[expression] [internal-functions] [decimal-number] [integer] [string] [identifier]")
{
    SECTION("check conversions to decimal number")
    {
        std::stringstream ss;
        ss << "div { z-index: decimal_number(314) }\n"
           << "span { z-index: decimal_number(\"3.14\") }\n"
           << "p { z-index: decimal_number('3.14px') }\n"
           << "i { z-index: decimal_number(\\33\\.14) }\n"
           << "q { z-index: decimal_number(3.14%) }\n"
           << "s { z-index: decimal_number(\" 123 \") }\n"
           << "b { z-index: decimal_number(\"123\") }\n"
           << "u { z-index: decimal_number(1.23) }\n"
           << "blockquote { z-index: decimal_number(\"1.23%\") }\n";
//std::cerr << "*** from " << ss.str() << "\n";
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
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:314\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"span\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3.14\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"px\" D:3.14\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"i\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:3.14\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"q\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:0.031\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"s\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:123\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:123\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"u\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:1.23\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"blockquote\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          DECIMAL_NUMBER \"\" D:0.012\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==

"div{z-index:314}"
"span{z-index:3.14}"
"p{z-index:3.14px}"
"i{z-index:3.14}"
"q{z-index:.031}"
"s{z-index:123}"
"b{z-index:123}"
"u{z-index:1.23}"
"blockquote{z-index:.012}"
"\n"
+ csspp_test::get_close_comment()

                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("check conversions to integer")
    {
        std::stringstream ss;
        ss << "div { z-index: integer(314) }\n"
           << "span { z-index: integer(\"3.14\") }\n"
           << "p { z-index: integer('3.14px') }\n"
           << "i { z-index: integer(\\33\\.14) }\n"
           << "q { z-index: integer(314%) }\n"
           << "s { z-index: integer(\" 123 \") }\n"
           << "b { z-index: integer(\"123\") }\n"
           << "u { z-index: integer(1.23) }\n";
//std::cerr << "*** from " << ss.str() << "\n";
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
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:314\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"span\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:3\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"px\" I:3\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"i\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:3\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"q\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:3\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"s\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:123\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          INTEGER \"\" I:123\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"u\"\n"
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

"div{z-index:314}"
"span{z-index:3}"
"p{z-index:3px}"
"i{z-index:3}"
"q{z-index:3}"
"s{z-index:123}"
"b{z-index:123}"
"u{z-index:1}"
"\n"
+ csspp_test::get_close_comment()

                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("check conversions to string")
    {
        std::stringstream ss;
        ss << "div { z-index: string(314) }\n"
           << "span { z-index: string(\"3.14\") }\n"
           << "p { z-index: string('3.14px') }\n"
           << "i { z-index: string(\\33\\.14) }\n"
           << "q { z-index: string(3.14%) }\n"
           << "s { z-index: string(\" 123 \") }\n"
           << "b { z-index: string(\"123\") }\n"
           << "u { z-index: string(1.23) }\n";
//std::cerr << "*** from " << ss.str() << "\n";
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
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"314\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"span\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"3.14\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"3.14px\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"i\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"3.14\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"q\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"3.14%\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"s\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \" 123 \"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"123\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"u\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          STRING \"1.23\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==

"div{z-index:\"314\"}"
"span{z-index:\"3.14\"}"
"p{z-index:\"3.14px\"}"
"i{z-index:\"3.14\"}"
"q{z-index:\"3.14%\"}"
"s{z-index:\" 123 \"}"
"b{z-index:\"123\"}"
"u{z-index:\"1.23\"}"
"\n"
+ csspp_test::get_close_comment()

                );

        REQUIRE(c.get_root() == n);
    }

    SECTION("check conversions to identifiers")
    {
        std::stringstream ss;
        ss << "div { z-index: identifier(test) }\n"
           << "span { z-index: identifier(\"test\") }\n"
           << "p { z-index: identifier('test') }\n"
           << "i { z-index: identifier(123) }\n"
           << "q { z-index: identifier(1.23%) }\n"
           << "s { z-index: identifier(\" 123 \") }\n"
           << "b { z-index: identifier(\"123\") }\n"
           << "u { z-index: identifier(1.23) }\n";
//std::cerr << "*** from " << ss.str() << "\n";
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
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"test\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"span\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"test\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"p\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"test\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"i\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"123\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"q\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"1.23%\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"s\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \" 123 \"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"b\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"123\"\n"
"  COMPONENT_VALUE\n"
"    ARG\n"
"      IDENTIFIER \"u\"\n"
"    OPEN_CURLYBRACKET B:true\n"
"      DECLARATION \"z-index\"\n"
"        ARG\n"
"          IDENTIFIER \"1.23\"\n"
+ csspp_test::get_close_comment(true)

            );

        std::stringstream assembler_out;
        csspp::assembler a(assembler_out);
        a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        REQUIRE(assembler_out.str() ==

"div{z-index:test}"
"span{z-index:test}"
"p{z-index:test}"
"i{z-index:\\31 23}"
"q{z-index:\\31\\.23\\%}"
"s{z-index:\\ 123\\ }"
"b{z-index:\\31 23}"
"u{z-index:\\31\\.23}"
"\n"
+ csspp_test::get_close_comment()

                );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid sub-expression decimal_number()/integer()/string()/identifier()", "[expression] [internal-functions] [decimal-number] [integer] [string] [identifier] [invalid]")
{
    SECTION("check conversions to decimal number with  an invalid string")
    {
        std::stringstream ss;
        ss << "div { z-index: decimal_number(\"invalid\") }\n";
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

        REQUIRE_ERRORS("test.css(1): error: decimal-number() expects a string parameter to represent a valid integer, decimal number, or percent value.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("check conversions to integer with an invalid string")
    {
        std::stringstream ss;
        ss << "div { z-index: integer(\"invalid\") }\n";
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

        REQUIRE_ERRORS("test.css(1): error: decimal-number() expects a string parameter to represent a valid integer, decimal number, or percent value.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("check conversions to integer with an invalid expression as parameter")
    {
        std::stringstream ss;
        ss << "div { z-index: integer(?) }\n";
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

        REQUIRE_ERRORS(
                "test.css(1): error: unsupported type CONDITIONAL as a unary expression token.\n"
                "test.css(1): error: integer() expects one value as parameter.\n"
            );

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression calling functions with invalid parameters", "[expression] [internal-functions] [invalid]")
{
    SECTION("abs(\"wrong\")")
    {
        std::stringstream ss;
        ss << "div { width: abs(\"wrong\"); }";
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

        REQUIRE_ERRORS("test.css(1): error: abs() expects a number as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("acos(true)")
    {
        std::stringstream ss;
        ss << "div { width: acos(true); }";
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

        REQUIRE_ERRORS("test.css(1): error: acos() expects a number as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("alpha(12)")
    {
        std::stringstream ss;
        ss << "div { width: alpha(12); }";
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

        REQUIRE_ERRORS("test.css(1): error: alpha() expects a color as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("asin(U+4\x3F?)")
    {
        std::stringstream ss;
        ss << "div { width: asin(U+4\x3F?); }";
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

        REQUIRE_ERRORS("test.css(1): error: asin() expects a number as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("atan(U+1-2)")
    {
        std::stringstream ss;
        ss << "div { width: atan(U+1-2); }";
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

        REQUIRE_ERRORS("test.css(1): error: atan() expects a number as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("blue(15)")
    {
        std::stringstream ss;
        ss << "div { width: blue(15); }";
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

        REQUIRE_ERRORS("test.css(1): error: blue() expects a color as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("ceil(false)")
    {
        std::stringstream ss;
        ss << "div { width: ceil(false); }";
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

        REQUIRE_ERRORS("test.css(1): error: ceil() expects a number as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("cos(white)")
    {
        std::stringstream ss;
        ss << "div { width: cos(white); }";
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

        REQUIRE_ERRORS("test.css(1): error: cos() expects an angle as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("floor(false)")
    {
        std::stringstream ss;
        ss << "div { width: floor(false); }";
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

        REQUIRE_ERRORS("test.css(1): error: floor() expects a number as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("red(15)")
    {
        std::stringstream ss;
        ss << "div { width: red(15); }";
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

        REQUIRE_ERRORS("test.css(1): error: red() expects a color as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("sin('number')")
    {
        std::stringstream ss;
        ss << "div { width: sin('number'); }";
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

        REQUIRE_ERRORS("test.css(1): error: sin() expects an angle as parameter.\n");

        REQUIRE(c.get_root() == n);
    }

    SECTION("tan(true)")
    {
        std::stringstream ss;
        ss << "div { width: tan(true); }";
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

        REQUIRE_ERRORS("test.css(1): error: tan() expects an angle as parameter.\n");

        REQUIRE(c.get_root() == n);
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
