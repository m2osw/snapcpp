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
 * \brief Test the expression.cpp file: "&&" operator.
 *
 * This test runs a battery of tests agains the expression.cpp "&&" (logical
 * and) operator to ensure full coverage and that all possible left
 * hand side and right hand side types are checked for the equality
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

TEST_CASE("Expression value && value", "[expression] [logical-and]")
{
    struct value_t
    {
        char const *    f_string;
        bool            f_true;
    };

    value_t const values[] =
    {
        { "10",         true  },
        { "3",          true  },
        { "0",          false },
        { "10.2",       true  },
        { "3.7",        true  },
        { "0.0",        false },
        { "5.1%",       true  },
        { "1%",         true  },
        { "0%",         false },
        { "0.0%",       false },
        { "true",       true  },
        { "false",      false },
        { "null",       false },
        { "black",      false },
        { "#7194F0",    true  },
        { "white",      true  },
        { "''",         false },
        { "'black'",    true  },
        { "'empty'",    true  },
        { "'false'",    true  }
    };
    size_t const value_count(sizeof(values) / sizeof(values[0]));

    for(size_t i(0); i < value_count; ++i)
    {
        for(size_t j(0); j < value_count; ++j)
        {
            std::stringstream ss;
            ss << "div { z-index: "
               << values[i].f_string
               << (rand() & 1 ? " && " : " and ")
               << values[j].f_string
               << " ? 9 : 5; }";
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
"          INTEGER \"\" I:" + (values[i].f_true && values[j].f_true ? "9" : "5") + "\n"
+ csspp_test::get_close_comment(true)

                );

            std::stringstream assembler_out;
            csspp::assembler a(assembler_out);
            a.output(n, csspp::output_mode_t::COMPRESSED);

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            REQUIRE(assembler_out.str() ==

std::string("div{z-index:") + (values[i].f_true && values[j].f_true ? "9" : "5") + "}\n"
+ csspp_test::get_close_comment()

                    );

            REQUIRE(c.get_root() == n);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Expression invalid && invalid", "[expression] [logical-and] [invalid]")
{
    SECTION("just ? is not a valid boolean")
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

    SECTION("boolean && ? is invalid")
    {
        std::stringstream ss;
        ss << "div { width: true && ?; }";
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

    SECTION("boolean && U+A?? is invalid")
    {
        std::stringstream ss;
        ss << "div { width: false && U+A??; }";
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

        REQUIRE_ERRORS("test.css(1): error: a boolean expression was expected.\n");

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
