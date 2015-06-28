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
 * \brief Test the assembler.cpp file.
 *
 * This test runs a battery of tests agains the assembler.cpp file to ensure
 * full coverage and many edge cases of CSS encoding.
 */

#include "catch_tests.h"

#include "csspp/assembler.h"
#include "csspp/compiler.h"
#include "csspp/exceptions.h"
#include "csspp/parser.h"

#include <fstream>
#include <sstream>

#include <string.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

namespace
{

bool is_valid_mode(csspp::output_mode_t const mode)
{
    switch(mode)
    {
    case csspp::output_mode_t::COMPACT:
    case csspp::output_mode_t::COMPRESSED:
    case csspp::output_mode_t::EXPANDED:
    case csspp::output_mode_t::TIDY:
        return true;

    default:
        return false;

    }
}

bool is_valid_char(csspp::wide_char_t c)
{
    switch(c)
    {
    case 0:
    case 0xFFFD:
        return false;

    default:
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            // UTF-16 surrogates are not valid wide characters
            return false;
        }
        return true;

    }
}

} // no name namespace

TEST_CASE("Assemble Two Rules", "[assembler]")
{
    // with many spaces
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div { color: black; }"
           << "span { border: 3px solid #f7d0cf; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div { color: black }\n"
"span { border: 3px solid #f7d0cf }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div{color:black}span{border:3px solid #f7d0cf}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div\n"
"{\n"
"  color: black;\n"
"}\n"
"span\n"
"{\n"
"  border: 3px solid #f7d0cf;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div{color:black}\n"
"span{border:3px solid #f7d0cf}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test multiple declarations in one rule
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div\n"
           << "{\n"
           << "  color: black;\n"
           << "  font-size: 1.3em;\n"
           << "}\n"
           << "\n"
           << "span\n"
           << "{\n"
           << "  border: 3px solid #f7d0cf;\n"
           << "\tborder-bottom-width: 1px;\n"
           << "  font: 17.2px/1.35em\tArial;\n"
           << "}\n"
           << "\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div { color: black; font-size: 1.3em }\n"
"span { border: 3px solid #f7d0cf; border-bottom-width: 1px; font: 17.2px/1.35em arial }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div{color:black;font-size:1.3em}span{border:3px solid #f7d0cf;border-bottom-width:1px;font:17.2px/1.35em arial}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div\n"
"{\n"
"  color: black;\n"
"  font-size: 1.3em;\n"
"}\n"
"span\n"
"{\n"
"  border: 3px solid #f7d0cf;\n"
"  border-bottom-width: 1px;\n"
"  font: 17.2px/1.35em arial;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div{color:black;font-size:1.3em}\n"
"span{border:3px solid #f7d0cf;border-bottom-width:1px;font:17.2px/1.35em arial}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test multiple selector lists
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div a b,\n"
           << "p span i\n"
           << "{\n"
           << "  color: black;\n"
           << "\t  font-size: 1.3em;\n"
           << " \n"
           << "  border: 3px solid #f7d0cf;\n"
           << "\tborder-bottom-width: 1px;\n"
           << "}\n"
           << "\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div a b, p span i { color: black; font-size: 1.3em; border: 3px solid #f7d0cf; border-bottom-width: 1px }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div a b,p span i{color:black;font-size:1.3em;border:3px solid #f7d0cf;border-bottom-width:1px}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div a b, p span i\n"
"{\n"
"  color: black;\n"
"  font-size: 1.3em;\n"
"  border: 3px solid #f7d0cf;\n"
"  border-bottom-width: 1px;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div a b,p span i{color:black;font-size:1.3em;border:3px solid #f7d0cf;border-bottom-width:1px}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble Selectors", "[assembler] [selectors]")
{
    // check various selectors without the operators

    // simple identifiers
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div span a { color: black; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div span a { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div span a{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div span a\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div span a{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test a simple attribute
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div[foo] {color: black}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div[foo] { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div[foo]{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div[foo]\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div[foo]{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // Test with a class
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div.foo{color:black}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div.foo { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div.foo{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div.foo\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div.foo{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // Test with an identifier
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "#foo div{color:black}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"#foo div { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"#foo div{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"#foo div\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"#foo div{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test an attribute with a test
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div[foo=\"a b c\"] {color: black}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div[foo = \"a b c\"] { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div[foo=\"a b c\"]{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div[foo = \"a b c\"]\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div[foo=\"a b c\"]{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test an :lang() pseudo function
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div:lang(fr) {color: black}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div:lang(fr) { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div:lang(fr){color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div:lang(fr)\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div:lang(fr){color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test an :not() pseudo function
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "div:not(:lang(fr)):not(:nth-child(2n+1)) {color: black}\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div:not(:lang(fr)):not(:nth-child(odd)) { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div:not(:lang(fr)):not(:nth-child(odd)){color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div:not(:lang(fr)):not(:nth-child(odd))\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div:not(:lang(fr)):not(:nth-child(odd)){color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // test the pseudo classes
    char const * pseudo_classes[] =
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
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        for(size_t j(0); j < sizeof(pseudo_classes) / sizeof(pseudo_classes[0]); ++j)
        {
            std::stringstream ss;
            ss << "div:" << pseudo_classes[j] << " {color: black}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());
            c.add_path(csspp_test::get_version_script_path());

            c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

            std::stringstream out;
            csspp::assembler a(out);
            a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            std::stringstream expected;
            switch(static_cast<csspp::output_mode_t>(i))
            {
            case csspp::output_mode_t::COMPACT:
expected << "div:" << pseudo_classes[j] << " { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::COMPRESSED:
expected << "div:" << pseudo_classes[j] << "{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::EXPANDED:
expected << "div:" << pseudo_classes[j] << "\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::TIDY:
expected << "div:" << pseudo_classes[j] << "{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            }

            REQUIRE(out.str() == expected.str());
            REQUIRE(c.get_root() == n);
        }
    }

    // test the pseudo classes
    char const * pseudo_elements[] =
    {
        "first-line",
        "first-letter",
        "before",
        "after"
    };
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        for(size_t j(0); j < sizeof(pseudo_elements) / sizeof(pseudo_elements[0]); ++j)
        {
            std::stringstream ss;
            ss << "div::" << pseudo_elements[j] << " {color: black}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());
            c.add_path(csspp_test::get_version_script_path());

            c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

            std::stringstream out;
            csspp::assembler a(out);
            a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            std::stringstream expected;
            switch(static_cast<csspp::output_mode_t>(i))
            {
            case csspp::output_mode_t::COMPACT:
expected << "div::" << pseudo_elements[j] << " { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::COMPRESSED:
expected << "div::" << pseudo_elements[j] << "{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::EXPANDED:
expected << "div::" << pseudo_elements[j] << "\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::TIDY:
expected << "div::" << pseudo_elements[j] << "{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            }

            REQUIRE(out.str() == expected.str());
            REQUIRE(c.get_root() == n);
        }
    }

    // test the An+B pseudo classes
    char const * pseudo_functions[] =
    {
        "nth-child",
        "nth-last-child",
        "nth-of-type",
        "nth-last-of-type"
    };
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        for(size_t j(0); j < sizeof(pseudo_functions) / sizeof(pseudo_functions[0]); ++j)
        {
            std::stringstream ss;
            ss << "div:" << pseudo_functions[j] << "(5n+2) {color: black}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());
            c.add_path(csspp_test::get_version_script_path());

            c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

            std::stringstream out;
            csspp::assembler a(out);
            a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            std::stringstream expected;
            switch(static_cast<csspp::output_mode_t>(i))
            {
            case csspp::output_mode_t::COMPACT:
expected << "div:" << pseudo_functions[j] << "(5n+2) { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::COMPRESSED:
expected << "div:" << pseudo_functions[j] << "(5n+2){color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::EXPANDED:
expected << "div:" << pseudo_functions[j] << "(5n+2)\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::TIDY:
expected << "div:" << pseudo_functions[j] << "(5n+2){color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            }

            REQUIRE(out.str() == expected.str());
            REQUIRE(c.get_root() == n);
        }
    }

    // test the scope operator
    char const * scope[] =
    {
        "*|div",
        "*|*",
        "div|*",
        "|div",
        "|*"
    };
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        for(size_t j(0); j < sizeof(scope) / sizeof(scope[0]); ++j)
        {
            std::stringstream ss;
            ss << "with " << scope[j] << " scope {color: black}\n";
            csspp::position pos("test.css");
            csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

            csspp::parser p(l);

            csspp::node::pointer_t n(p.stylesheet());

            csspp::compiler c;
            c.set_root(n);
            c.add_path(csspp_test::get_script_path());
            c.add_path(csspp_test::get_version_script_path());

            c.compile(false);

//std::cerr << "Compiler result is: [" << *n << "]\n";

            std::stringstream out;
            csspp::assembler a(out);
            a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

            std::stringstream expected;
            switch(static_cast<csspp::output_mode_t>(i))
            {
            case csspp::output_mode_t::COMPACT:
expected << "with " << scope[j] << " scope { color: black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::COMPRESSED:
expected << "with " << scope[j] << " scope{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::EXPANDED:
expected << "with " << scope[j] << " scope\n"
"{\n"
"  color: black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            case csspp::output_mode_t::TIDY:
expected << "with " << scope[j] << " scope{color:black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                break;

            }

            REQUIRE(out.str() == expected.str());
            REQUIRE(c.get_root() == n);
        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble Numbers", "[assembler] [numbers]")
{
    // create strings with more single quotes (')
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::integer_t integer(rand() % 10000);
        csspp::decimal_number_t decimal_number(static_cast<csspp::decimal_number_t>(rand() % 10000) / 100.0);
        csspp::decimal_number_t percent(static_cast<csspp::decimal_number_t>(rand() % 10000) / 100.0);

        ss << "#wrapper div * span a:hover {\n"
           << "  width: " << integer << ";\n"
           << "  height: " << decimal_number << ";\n"
           << "  font-size: " << percent << "%;\n"
           << "}\n";

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        std::stringstream expected;
        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
expected << "#wrapper div * span a:hover {"
         << " width: " << integer << ";"
         << " height: " << decimal_number << ";"
         << " font-size: " << percent << "%"
         << " }\n"
         << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
            break;

        case csspp::output_mode_t::COMPRESSED:
expected << "#wrapper div * span a:hover{"
         << "width:" << integer << ";"
         << "height:" << decimal_number << ";"
         << "font-size:" << percent << "%"
         << "}\n"
         << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
            break;

        case csspp::output_mode_t::EXPANDED:
expected << "#wrapper div * span a:hover\n"
         << "{\n"
         << "  width: " << integer << ";\n"
         << "  height: " << decimal_number << ";\n"
         << "  font-size: " << percent << "%;\n"
         << "}\n"
         << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
            break;

        case csspp::output_mode_t::TIDY:
expected << "#wrapper div * span a:hover{"
         << "width:" << integer << ";"
         << "height:" << decimal_number << ";"
         << "font-size:" << percent << "%"
         << "}\n"
         << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
            break;

        }

        REQUIRE(out.str() == expected.str());
        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble Unicode Range", "[assembler] [unicode-range] [at-keyword]")
{
    // a valid @supports
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "@font-face\n"
           << "{\n"
           << "  unicode-range: U+400-4fF;\n"
           << "  font-style: italic;\n"
           << "}\n";
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
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Result is: [" << *c.get_root() << "]\n";

        REQUIRE_ERRORS("");

//std::cerr << "Compiler result is: [" << *n << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"@font-face \n"
"{\n"
"unicode-range: U+4??; "
"font-style: italic"
"}\n"
"\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"@font-face {unicode-range:U+4??;font-style:italic}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"@font-face \n"
"{\n"
"  unicode-range: U+4??;\n"
"  font-style: italic}\n"
"\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"@font-face \n"
"{\n"
"unicode-range:U+4??;font-style:italic}\n"
"\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no left over?
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble Strings", "[assembler] [strings]")
{
    // create strings with more single quotes (')
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        std::string str;
        int const size(rand() % 20 + 1);
        int dq(rand() % 5 + 1);
        int sq(rand() % 8 + dq); // if sq >= dq use " for strings
        for(int j(0); j < size; ++j)
        {
            if(dq > 0 && rand() % 1 == 0)
            {
                --dq;
                str += '\\';
                str += '"';
            }
            if(sq > 0 && rand() % 1 == 0)
            {
                --sq;
                str += '\'';
            }
            str += static_cast<char>(rand() % 26 + 'a');
        }
        while(dq + sq > 0)
        {
            if(dq > 0 && rand() % 1 == 0)
            {
                --dq;
                str += '\\';
                str += '"';
            }
            if(sq > 0 && rand() % 1 == 0)
            {
                --sq;
                str += '\'';
            }
        }
        ss << "div::before { content: \""
           << str
           << "\" }";

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div::before { content: \"" + str + "\" }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div::before{content:\"" + str + "\"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div::before\n"
"{\n"
"  content: \"" + str + "\";\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div::before{content:\"" + str + "\"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // create strings with more double quotes (")
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        std::string str;
        int const size(rand() % 20 + 1);
        int sq(rand() % 5 + 1);
        int dq(rand() % 8 + 1 + sq);  // we need dq > sq
        for(int j(0); j < size; ++j)
        {
            if(dq > 0 && rand() % 1 == 0)
            {
                --dq;
                str += '"';
            }
            if(sq > 0 && rand() % 1 == 0)
            {
                --sq;
                str += '\\';
                str += '\'';
            }
            str += static_cast<char>(rand() % 26 + 'a');
        }
        while(dq + sq > 0)
        {
            if(dq > 0 && rand() % 1 == 0)
            {
                --dq;
                str += '"';
            }
            if(sq > 0 && rand() % 1 == 0)
            {
                --sq;
                str += '\\';
                str += '\'';
            }
        }
        ss << "div::after { content: '"
           << str
           << "' }";

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div::after { content: '" + str + "' }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div::after{content:'" + str + "'}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div::after\n"
"{\n"
"  content: '" + str + "';\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div::after{content:'" + str + "'}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble URI", "[assembler] [uri]")
{
    // all characters can be inserted as is (no switching to string)
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        std::string name;
        int const size(rand() % 20 + 1);
        for(int j(0); j < size; ++j)
        {
            csspp::wide_char_t c(rand() % 0x110000);
            while(!is_valid_char(c)
               || c == '\''
               || c == '"'
               || c == '('
               || c == ')'
               || l->is_non_printable(c))
            {
                c = rand() % 0x110000;
            }
            name += l->wctomb(c);
        }
        ss << "div { background-image: url(/images/"
           << name
           << ".png); }";

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div { background-image: url( /images/" + name + ".png ) }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div{background-image:url(/images/" + name + ".png)}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div\n"
"{\n"
"  background-image: url( /images/" + name + ".png );\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div{background-image:url(/images/" + name + ".png)}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // at least one character requires the use of a string
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        std::string name;
        csspp::wide_char_t special;
        int const size(rand() % 20 + 1);
        for(int j(0); j < size; ++j)
        {
            csspp::wide_char_t c;
            if(j == size / 2)
            {
                c = special = "'\"()"[rand() % 4];
            }
            else
            {
                c = rand() % 0x110000;
                while(!is_valid_char(c)
                   || c == '\''
                   || c == '"'
                   || c == '('
                   || c == ')'
                   || l->is_non_printable(c))
                {
                    c = rand() % 0x110000;
                }
            }
            name += l->wctomb(c);
        }
        std::string quote;
        quote += special == '"' ? '\'' : '"';
        ss << "div { background-image: url("
           << quote
           << "/images/"
           << name
           << ".png"
           << quote
           << "); }";

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"div { background-image: url( " + quote + "/images/" + name + ".png" + quote + " ) }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div{background-image:url(" + quote + "/images/" + name + ".png" + quote + ")}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"div\n"
"{\n"
"  background-image: url( " + quote + "/images/" + name + ".png" + quote + " );\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div{background-image:url(" + quote + "/images/" + name + ".png" + quote + ")}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble C++ Comment", "[assembler] [comment]")
{
    // One line C++ comment
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "// Copyright (c) 2015  Made to Order Software Corp. -- Assembler Test Version {$_csspp_version} -- @preserve\n"
           << "body.error { color: red }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        REQUIRE_ERRORS("test.css(1): warning: C++ comments should not be preserved as they are not supported by most CSS parsers.\n");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp. -- Assembler Test Version 1.0.0 -- @preserve */\n"
"body.error { color: red }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp. -- Assembler Test Version 1.0.0 -- @preserve */\n"
"body.error{color:red}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp. -- Assembler Test Version 1.0.0 -- @preserve */\n"
"body.error\n"
"{\n"
"  color: red;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp. -- Assembler Test Version 1.0.0 -- @preserve */\n"
"body.error{color:red}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // Multi-line C++ comment
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "// Copyright (c) 2015  Made to Order Software Corp.\n"
           << "// Assembler Test\n"
           << "// @preserve\n"
           << "body.error { color: red }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        REQUIRE_ERRORS("test.css(1): warning: C++ comments should not be preserved as they are not supported by most CSS parsers.\n");

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp.\n"
" * Assembler Test\n"
" * @preserve\n"
" */\n"
"body.error { color: red }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp.\n"
" * Assembler Test\n"
" * @preserve\n"
" */\n"
"body.error{color:red}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp.\n"
" * Assembler Test\n"
" * @preserve\n"
" */\n"
"body.error\n"
"{\n"
"  color: red;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"/* Copyright (c) 2015  Made to Order Software Corp.\n"
" * Assembler Test\n"
" * @preserve\n"
" */\n"
"body.error{color:red}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble @-keyword", "[assembler] [at-keyword]")
{
    // Standard @document with a sub-rule
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "@document url(http://www.example.com/), regexp(\"https://.*\")\n"
           << "{\n"
           << "  body { width: 8.5in; height: 9in; }\n"
           << "  div { border: 0.25in solid lightgray }\n"
           << "}\n"
           << "#edge { border: 1px solid black }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"@document url( http://www.example.com/ ), regexp(\"https://.*\")\n"
"{\n"
"body { width: 8.5in; height: 9in }\n"
"div { border: 0.25in solid lightgray }\n"
"}\n"
"\n"
"#edge { border: 1px solid black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"@document url(http://www.example.com/),regexp(\"https://.*\"){body{width:8.5in;height:9in}div{border:0.25in solid lightgray}}#edge{border:1px solid black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"@document url( http://www.example.com/ ), regexp(\"https://.*\")\n"
"{\n"
"body\n"
"{\n"
"  width: 8.5in;\n"
"  height: 9in;\n"
"}\n"
"div\n"
"{\n"
"  border: 0.25in solid lightgray;\n"
"}\n"
"}\n"
"\n"
"#edge\n"
"{\n"
"  border: 1px solid black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"@document url(http://www.example.com/),regexp(\"https://.*\")\n"
"{\n"
"body{width:8.5in;height:9in}\n"
"div{border:0.25in solid lightgray}\n"
"}\n"
"\n"
"#edge{border:1px solid black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // Standard @media with a sub-rule
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "@media screen or (printer and color) { body { width: 8.5in; height: 9in; } }\n"
           << "#edge { border: 1px solid black }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"@media screen or (printer and color) \n"
"{\n"
"body { width: 8.5in; height: 9in }\n"
"}\n"
"\n"
"#edge { border: 1px solid black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"@media screen or (printer and color){body{width:8.5in;height:9in}}#edge{border:1px solid black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"@media screen or (printer and color) \n"
"{\n"
"body\n"
"{\n"
"  width: 8.5in;\n"
"  height: 9in;\n"
"}\n"
"}\n"
"\n"
"#edge\n"
"{\n"
"  border: 1px solid black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"@media screen or (printer and color)\n"
"{\n"
"body{width:8.5in;height:9in}\n"
"}\n"
"\n"
"#edge{border:1px solid black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // @media with many parenthesis and multiple sub-rules
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "@media not (screen or ((laser or matrix or jet-printer) and color)) {\n"
           << "  body { width: 8.5in; height: 9in; }\n"
           << "  div { margin: 0.15in; padding: 0.07in; }\n"
           << "  p { margin-bottom: 2em; }\n"
           << "}\n"
           << "#edge { border: 1px solid black }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"@media not (screen or ((laser or matrix or jet-printer) and color)) \n"
"{\n"
"body { width: 8.5in; height: 9in }\n"
"div { margin: 0.15in; padding: 0.07in }\n"
"p { margin-bottom: 2em }\n"
"}\n"
"\n"
"#edge { border: 1px solid black }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"@media not (screen or ((laser or matrix or jet-printer) and color)){body{width:8.5in;height:9in}div{margin:0.15in;padding:0.07in}p{margin-bottom:2em}}#edge{border:1px solid black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"@media not (screen or ((laser or matrix or jet-printer) and color)) \n"
"{\n"
"body\n"
"{\n"
"  width: 8.5in;\n"
"  height: 9in;\n"
"}\n"
"div\n"
"{\n"
"  margin: 0.15in;\n"
"  padding: 0.07in;\n"
"}\n"
"p\n"
"{\n"
"  margin-bottom: 2em;\n"
"}\n"
"}\n"
"\n"
"#edge\n"
"{\n"
"  border: 1px solid black;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"@media not (screen or ((laser or matrix or jet-printer) and color))\n"
"{\n"
"body{width:8.5in;height:9in}\n"
"div{margin:0.15in;padding:0.07in}\n"
"p{margin-bottom:2em}\n"
"}\n"
"\n"
"#edge{border:1px solid black}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // simple @import to see the ';' at the end of the line
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "@import url(//css.m2osw.com/store/colors.css) only screen or (printer and color);\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"@import url( //css.m2osw.com/store/colors.css ) only screen or (printer and color) ;\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"@import url(//css.m2osw.com/store/colors.css) only screen or (printer and color);\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"@import url( //css.m2osw.com/store/colors.css ) only screen or (printer and color) ;\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"@import url(//css.m2osw.com/store/colors.css) only screen or (printer and color);\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble Functions", "[assembler] [function]")
{
    // Test with gradient() function
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "$box($color, $width, $height): { border: 1px * 3 solid $color; width: $width * 1.5; height: $height };\n"
           << "a ~ b { -csspp-null: $box(#39458A, 300px, 200px); }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

std::cerr << "Parser result is: [" << *n << "]\n";

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        std::stringstream expected;
        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
expected << "a ~ b { border: 3px solid #39458a; width: 450px; height: 200px }\n";
            break;

        case csspp::output_mode_t::COMPRESSED:
expected << "a~b{border:3px solid #39458a;width:450px;height:200px}\n";
            break;

        case csspp::output_mode_t::EXPANDED:
expected << "a ~ b\n"
 << "{\n"
 << "  border: 3px solid #39458a;\n"
 << "  width: 450px;\n"
 << "  height: 200px;\n"
 << "}\n";
            break;

        case csspp::output_mode_t::TIDY:
expected << "a~b{border:3px solid #39458a;width:450px;height:200px}\n";
            break;

        }
        expected << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
        REQUIRE(out.str() == expected.str());

        REQUIRE(c.get_root() == n);
    }

    // CSS Function
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "a b { color: rgba(1 * 7, 2, 3, 0.5); }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

//std::cerr << "Parser result is: [" << *n << "]\n";

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        std::stringstream expected;
        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
expected << "a b { color: rgba(7, 2, 3, 0.5) }\n";
            break;

        case csspp::output_mode_t::COMPRESSED:
expected << "a b{color:rgba(7,2,3,0.5)}\n";
            break;

        case csspp::output_mode_t::EXPANDED:
expected << "a b\n"
 << "{\n"
 << "  color: rgba(7, 2, 3, 0.5);\n"
 << "}\n";
            break;

        case csspp::output_mode_t::TIDY:
expected << "a b{color:rgba(7,2,3,0.5)}\n";
            break;

        }
        expected << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
        REQUIRE(out.str() == expected.str());

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assemble Operators", "[assembler] [operators]")
{
    // Selector unary operator
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "a * b { color: red; }\n";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        std::stringstream expected;
        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
expected << "a * b { color: red }\n";
            break;

        case csspp::output_mode_t::COMPRESSED:
expected << "a * b{color:red}\n";
            break;

        case csspp::output_mode_t::EXPANDED:
expected << "a * b\n"
 << "{\n"
 << "  color: red;\n"
 << "}\n";
            break;

        case csspp::output_mode_t::TIDY:
expected << "a * b{color:red}\n";
            break;

        }
        expected << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
        REQUIRE(out.str() == expected.str());

        REQUIRE(c.get_root() == n);
    }

    // Selector binary operators
    {
        char const * selector_operator[] =
        {
            "+",
            "~",
            ">"
        };

        for(size_t op(0); op < sizeof(selector_operator) / sizeof(selector_operator[0]); ++op)
        {
            for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
                i <= static_cast<int>(csspp::output_mode_t::TIDY);
                ++i)
            {
                std::stringstream ss;
                ss << "a"
                   << ((rand() % 2) == 0 ? " " : "")
                   << selector_operator[op]
                   << ((rand() % 2) == 0 ? " " : "")
                   << "b { color: red; }\n";
                csspp::position pos("test.css");
                csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

                csspp::parser p(l);

                csspp::node::pointer_t n(p.stylesheet());

                csspp::compiler c;
                c.set_root(n);
                c.add_path(csspp_test::get_script_path());
                c.add_path(csspp_test::get_version_script_path());

                c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

                std::stringstream out;
                csspp::assembler a(out);
                a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                std::stringstream expected;
                switch(static_cast<csspp::output_mode_t>(i))
                {
                case csspp::output_mode_t::COMPACT:
expected << "a " << selector_operator[op] << " b { color: red }\n";
                    break;

                case csspp::output_mode_t::COMPRESSED:
expected << "a" << selector_operator[op] << "b{color:red}\n";
                    break;

                case csspp::output_mode_t::EXPANDED:
expected << "a " << selector_operator[op] << " b\n"
         << "{\n"
         << "  color: red;\n"
         << "}\n";
                    break;

                case csspp::output_mode_t::TIDY:
expected << "a" << selector_operator[op] << "b{color:red}\n";
                    break;

                }
                expected << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                REQUIRE(out.str() == expected.str());

                REQUIRE(c.get_root() == n);
            }
        }
    }

    // Attributes binary operators
    {
        char const * attribute_operator[] =
        {
            "=",
            "~=",
            "^=",
            "$=",
            "*=",
            "|="
        };

        for(size_t op(0); op < sizeof(attribute_operator) / sizeof(attribute_operator[0]); ++op)
        {
            for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
                i <= static_cast<int>(csspp::output_mode_t::TIDY);
                ++i)
            {
                std::stringstream ss;
                ss << "a["
                   << ((rand() % 2) != 0 ? " " : "")
                   << "b"
                   << ((rand() % 2) != 0 ? " " : "")
                   << attribute_operator[op]
                   << ((rand() % 2) != 0 ? " " : "")
                   << "3"
                   << ((rand() % 2) != 0 ? " " : "")
                   << "]"
                   << ((rand() % 2) != 0 ? "\n" : "")
                   << "{"
                   << ((rand() % 2) != 0 ? " " : "")
                   << "color"
                   << ((rand() % 2) != 0 ? " " : "")
                   << ":"
                   << ((rand() % 2) != 0 ? " " : "")
                   << "red"
                   << ((rand() % 2) != 0 ? " " : "")
                   << "}\n";
                csspp::position pos("test.css");
                csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

                csspp::parser p(l);

                csspp::node::pointer_t n(p.stylesheet());

                csspp::compiler c;
                c.set_root(n);
                c.add_path(csspp_test::get_script_path());
                c.add_path(csspp_test::get_version_script_path());

                c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

                std::stringstream out;
                csspp::assembler a(out);
                a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

                std::stringstream expected;
                switch(static_cast<csspp::output_mode_t>(i))
                {
                case csspp::output_mode_t::COMPACT:
expected << "a[b " << attribute_operator[op] << " 3] { color: red }\n";
                    break;

                case csspp::output_mode_t::COMPRESSED:
expected << "a[b" << attribute_operator[op] << "3]{color:red}\n";
                    break;

                case csspp::output_mode_t::EXPANDED:
expected << "a[b " << attribute_operator[op] << " 3]\n"
        "{\n"
        "  color: red;\n"
        "}\n";
                    break;

                case csspp::output_mode_t::TIDY:
expected << "a[b" << attribute_operator[op] << "3]{color:red}\n";
                    break;

                }
                expected << "/* @preserve -- CSS file parsed by csspp v1.0.0 */\n";
                REQUIRE(out.str() == expected.str());

                REQUIRE(c.get_root() == n);
            }
        }
    }

    // '!' -- EXCLAMATION
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << "*[b = 3] { color : red !"
           << ((rand() % 2) == 0 ? " " : "")
           << "important; }";
        csspp::position pos("test.css");
        csspp::lexer::pointer_t l(new csspp::lexer(ss, pos));

        csspp::parser p(l);

        csspp::node::pointer_t n(p.stylesheet());

        csspp::compiler c;
        c.set_root(n);
        c.add_path(csspp_test::get_script_path());
        c.add_path(csspp_test::get_version_script_path());

        c.compile(false);

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

        std::stringstream out;
        csspp::assembler a(out);
        a.output(n, static_cast<csspp::output_mode_t>(i));

//std::cerr << "----------------- Result is " << static_cast<csspp::output_mode_t>(i) << "\n[" << out.str() << "]\n";

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(out.str() ==
"[b = 3] { color: red !important }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"[b=3]{color:red!important}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(out.str() ==
"[b = 3]\n"
"{\n"
"  color: red !important;\n"
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"[b=3]{color:red!important}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assembler Modes", "[assembler] [mode]")
{
    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        std::stringstream ss;
        ss << static_cast<csspp::output_mode_t>(i);

        switch(static_cast<csspp::output_mode_t>(i))
        {
        case csspp::output_mode_t::COMPACT:
            REQUIRE(ss.str() == "COMPACT");
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(ss.str() == "COMPRESSED");
            break;

        case csspp::output_mode_t::EXPANDED:
            REQUIRE(ss.str() == "EXPANDED");
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(ss.str() == "TIDY");
            break;

        }
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Invalid Assembler Mode", "[assembler] [mode] [invalid]")
{
    // with many spaces
    for(int i(0); i < 100; ++i)
    {
        std::stringstream out;
        csspp::assembler a(out);
        csspp::position pos("test.css");
        csspp::node::pointer_t n(new csspp::node(csspp::node_type_t::LIST, pos));
        csspp::output_mode_t mode(static_cast<csspp::output_mode_t>(rand()));
        while(is_valid_mode(mode))
        {
            mode = static_cast<csspp::output_mode_t>(rand());
        }
        REQUIRE_THROWS_AS(a.output(n, mode), csspp::csspp_exception_logic);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Inacceptable Nodes", "[assembler] [invalid]")
{
    // list of "invalid" nodes in the assembler
    csspp::node_type_t node_types[] =
    {
        csspp::node_type_t::UNKNOWN,
        csspp::node_type_t::AND,
        csspp::node_type_t::ASSIGNMENT,
        csspp::node_type_t::BOOLEAN,
        csspp::node_type_t::CDC,
        csspp::node_type_t::CDO,
        csspp::node_type_t::CLOSE_CURLYBRACKET,
        csspp::node_type_t::CLOSE_PARENTHESIS,
        csspp::node_type_t::CLOSE_SQUAREBRACKET,
        csspp::node_type_t::COLUMN,
        csspp::node_type_t::COMMA,
        csspp::node_type_t::CONDITIONAL,
        csspp::node_type_t::DIVIDE,
        csspp::node_type_t::DOLLAR,
        csspp::node_type_t::EOF_TOKEN,
        csspp::node_type_t::EXCLAMATION,
        csspp::node_type_t::GREATER_EQUAL,
        csspp::node_type_t::LESS_EQUAL,
        csspp::node_type_t::LESS_THAN,
        csspp::node_type_t::MODULO,
        csspp::node_type_t::NOT_EQUAL,
        csspp::node_type_t::NULL_TOKEN,
        csspp::node_type_t::PLACEHOLDER,
        csspp::node_type_t::POWER,
        csspp::node_type_t::REFERENCE,
        csspp::node_type_t::SEMICOLON,
        csspp::node_type_t::SUBTRACT,
        csspp::node_type_t::VARIABLE,
        csspp::node_type_t::VARIABLE_FUNCTION,
        csspp::node_type_t::max_type
    };

    for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
        i <= static_cast<int>(csspp::output_mode_t::TIDY);
        ++i)
    {
        for(size_t j(0); j < sizeof(node_types) / sizeof(node_types[0]); ++j)
        {
            csspp::position pos("test.css");
            csspp::node::pointer_t root(new csspp::node(node_types[j], pos));

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";

            std::stringstream out;
            csspp::assembler a(out);
            REQUIRE_THROWS_AS(a.output(root, static_cast<csspp::output_mode_t>(i)), csspp::csspp_exception_logic);
        }
    }
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
