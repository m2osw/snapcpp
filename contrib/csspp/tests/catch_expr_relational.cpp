// CSS Preprocessor -- Test Suite
// Copyright (C) 2015-2017  Made to Order Software Corp.
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
 * \brief Test the expression.cpp file: "<", "<=", ">", ">=" operators.
 *
 * This test runs a battery of tests agains the expression.cpp "<" (less
 * than), "<=" (less or equal), ">" (greater than), ">=" (greater or equal)
 * operators to ensure full coverage and that all possible left
 * hand side and right hand side types are checked for the relational
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

TEST_CASE("Expression number <,<=,>,>= number", "[expression] [relational]")
{
    struct operator_results_t
    {
        char const *        f_operator;
        bool                f_different_result;
        bool                f_equal_result;
    };

    // different is true if a < b is true
    // equal is true if a == b is true
    operator_results_t const op[] =
    {
        { "<",      true,       false   },
        { "<=",     true,       true    },
        { ">",      false,      false   },
        { ">=",     false,      true    }
    };

    SECTION("compare 10 ?? 3")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10 "
               << op[idx].f_operator
               << " 3 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 3 ?? 10")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 3 "
               << op[idx].f_operator
               << " 10 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10 ?? 10")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10 "
               << op[idx].f_operator
               << " 10 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10% ?? 3%")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10% "
               << op[idx].f_operator
               << " 3% ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 3% ?? 10%")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 3% "
               << op[idx].f_operator
               << " 10% ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10% ?? 10%")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10% "
               << op[idx].f_operator
               << " 10% ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10.5 ?? 3.15")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10.5 "
               << op[idx].f_operator
               << " 3.15 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 3.15 ?? 10.5")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 3.15 "
               << op[idx].f_operator
               << " 10.5 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10.5 ?? 10.5")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10.5 "
               << op[idx].f_operator
               << " 10.5 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10 ?? 3.15")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10 "
               << op[idx].f_operator
               << " 3.15 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 3.15 ?? 10")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 3.15 "
               << op[idx].f_operator
               << " 10 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10 ?? 10.0")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10 "
               << op[idx].f_operator
               << " 10.0 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10.5 ?? 3")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10.5 "
               << op[idx].f_operator
               << " 3 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 3 ?? 10.5")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 3 "
               << op[idx].f_operator
               << " 10.5 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 10 ?? 10.0")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10.0 "
               << op[idx].f_operator
               << " 10 ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare true ?? false")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: false "
               << op[idx].f_operator
               << " true ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }

        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: true "
               << op[idx].f_operator
               << " false ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }

        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: true "
               << op[idx].f_operator
               << " true ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }

        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: false "
               << op[idx].f_operator
               << " false ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 'xyz' ?? 'abc'")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 'xyz' "
               << op[idx].f_operator
               << " 'abc' ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(!op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (!op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 'abc' ?? 'xyz'")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 'abc' "
               << op[idx].f_operator
               << " 'xyz' ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_different_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_different_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("compare 'abc' ?? 'abc'")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 'abc' "
               << op[idx].f_operator
               << " 'abc' ? 9 : 5; }";
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
"          INTEGER \"\" I:" + std::to_string(op[idx].f_equal_result ? 9 : 5) + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==
std::string("div{z-index:") + (op[idx].f_equal_result ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()
                    );

            REQUIRE(c.get_root() == n);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression number/invalid <,<=,>,>= number/invalid", "[expression] [relational] [invalid]")
{
    char const * op[] =
    {
        "<",
        "<=",
        ">",
        ">="
    };

    SECTION("just ? is not a valid number")
    {
        std::stringstream ss;
        ss << "div { border: ?; }";
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

    SECTION("number ?? ? is invalid")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { width: 10px "
               << op[idx]
               << " ?; }";
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
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Relational expressions with invalid dimensions or decimal numbers", "[expression] [relational] [invalid]")
{
    char const * op[] =
    {
        "<",
        "<=",
        ">",
        ">="
    };

    SECTION("left and right must have the same dimension")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { border: 3px "
               << op[idx]
               << " 2em; }";
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

            REQUIRE_ERRORS("test.css(1): error: incompatible types or dimensions between INTEGER and INTEGER for operator '=', '!=', '<', '<=', '>', '>=', '~=', '^=', '$=', '*=', or '|='.\n");

            REQUIRE(c.get_root() == n);
        }
    }

    SECTION("try again with a percent number")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10 "
               << op[idx]
               << " 5%; }";
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

            REQUIRE_ERRORS(
                    "test.css(1): error: incompatible types or dimensions between INTEGER and PERCENT for operator '=', '!=', '<', '<=', '>', '>=', '~=', '^=', '$=', '*=', or '|='.\n"
                );

            REQUIRE(c.get_root() == n);
        }

        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10em "
               << op[idx]
               << " 5%; }";
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

            REQUIRE_ERRORS("test.css(1): error: incompatible types or dimensions between INTEGER and PERCENT for operator '=', '!=', '<', '<=', '>', '>=', '~=', '^=', '$=', '*=', or '|='.\n");

            REQUIRE(c.get_root() == n);
        }

        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10% "
               << op[idx]
               << " 5; }";
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

            REQUIRE_ERRORS("test.css(1): error: incompatible types or dimensions between PERCENT and INTEGER for operator '=', '!=', '<', '<=', '>', '>=', '~=', '^=', '$=', '*=', or '|='.\n");

            REQUIRE(c.get_root() == n);
        }

        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { z-index: 10% "
               << op[idx]
               << " 5px; }";
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

            REQUIRE_ERRORS("test.css(1): error: incompatible types or dimensions between PERCENT and INTEGER for operator '=', '!=', '<', '<=', '>', '>=', '~=', '^=', '$=', '*=', or '|='.\n");

            REQUIRE(c.get_root() == n);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Relational expressions with colors fail", "[expression] [relational] [invalid]")
{
    char const * op[] =
    {
        "<",
        "<=",
        ">",
        ">="
    };

    SECTION("color op color always fails")
    {
        for(size_t idx(0); idx < sizeof(op) / sizeof(op[0]); ++idx)
        {
            std::stringstream ss;
            ss << "div { border: red "
               << op[idx]
               << " white; }";
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

            REQUIRE_ERRORS("test.css(1): error: incompatible types between COLOR and COLOR for operator '<', '<=', '>', or '>='.\n");

            REQUIRE(c.get_root() == n);
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
