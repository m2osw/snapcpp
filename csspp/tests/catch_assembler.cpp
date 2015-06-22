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

TEST_CASE("Assembler Two Rules", "[assembler]")
{
    // with many spaces
    if(0) for(int i(static_cast<int>(csspp::output_mode_t::COMPACT));
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
"span { border: 3px solid #f7d0cf; border-bottom-width: 1px }\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::COMPRESSED:
            REQUIRE(out.str() ==
"div{color:black;font-size:1.3em}span{border:3px solid #f7d0cf;border-bottom-width:1px}\n"
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
"}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        case csspp::output_mode_t::TIDY:
            REQUIRE(out.str() ==
"div{color:black;font-size:1.3em}\n"
"span{border:3px solid #f7d0cf;border-bottom-width:1px}\n"
"/* @preserve -- CSS file parsed by csspp v1.0.0 */\n"
                );
            break;

        }

        REQUIRE(c.get_root() == n);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Assembler Strings", "[assembler]")
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

TEST_CASE("Assembler URI", "[assembler]")
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

TEST_CASE("Assembler C++ Comment", "[assembler]")
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

TEST_CASE("Assembler @-keyword", "[assembler]")
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

TEST_CASE("Invalid Assembler Mode", "[assembler]")
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

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
