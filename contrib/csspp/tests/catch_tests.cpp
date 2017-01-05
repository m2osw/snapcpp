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
 * \brief csspp main unit test.
 *
 * This test suite uses catch.hpp, for details see:
 *
 *   https://github.com/philsquared/Catch/blob/master/docs/tutorial.md
 */

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

#include "catch_tests.h"

#include "csspp/csspp.h"
#include "csspp/error.h"
#include "csspp/node.h"

#include <cstring>

#include <stdlib.h>

namespace csspp_test
{

// static variables
namespace
{

char * g_progname;

trace_error * g_trace_error;

std::string g_script_path;

std::string g_version_script_path;

time_t const g_now(1435871798); // 07/02/2015 14:16:38

}

trace_error::trace_error()
{
    csspp::error::instance().set_error_stream(m_error_message);
}

trace_error & trace_error::instance()
{
    if(g_trace_error == nullptr)
    {
        g_trace_error = new trace_error();
    }
    return *g_trace_error;
}

void trace_error::expected_error(std::string const & msg, char const * filename, int line)
{
    std::string e(m_error_message.str());
    m_error_message.str("");

    std::string::size_type pos(e.find("/scripts"));
    if(pos != std::string::npos)
    {
        e = e.substr(pos + 1);
    }

//std::cerr << "require " << msg << "\n";
//std::cerr << "    got " << e << "\n";
    if(e != msg)
    {
        // print a message otherwise filename & line get lost
        std::cerr << filename << "(" << line << "): error: error messages are not equal.\n"; // LCOV_EXCL_LINE
    }
    REQUIRE(e == msg);
}

our_unicode_range_t::our_unicode_range_t(csspp::wide_char_t start, csspp::wide_char_t end)
    : f_start(start)
    , f_end(end)
{
}

void our_unicode_range_t::set_start(csspp::wide_char_t start)
{
    f_start = start;
}

void our_unicode_range_t::set_end(csspp::wide_char_t end)
{
    f_end = end;
}

void our_unicode_range_t::set_range(csspp::range_value_t range)
{
    f_start = static_cast<csspp::wide_char_t>(range);
    f_end = static_cast<csspp::wide_char_t>(range >> 32);
}

csspp::wide_char_t our_unicode_range_t::get_start() const
{
    return f_start;
}

csspp::wide_char_t our_unicode_range_t::get_end() const
{
    return f_end;
}

csspp::range_value_t our_unicode_range_t::get_range() const
{
    return f_start + (static_cast<csspp::range_value_t>(f_end) << 32);
}

void compare(std::string const & generated, std::string const & expected, char const * filename, int line)
{
    char const *g(generated.c_str());
    char const *e(expected.c_str());

    int pos(1);
    for(; *g != '\0' && *e != '\0'; ++pos)
    {
        // one line from the left
        char const *start;
        for(start = g; *g != '\n' && *g != '\0'; ++g);
        std::string gs(start, g - start);
        if(*g == '\n')
        {
            ++g;
        }

        // one line from the right
        start = e;
        for(start = e; *e != '\n' && *e != '\0'; ++e);
        std::string es(start, e - start);
        if(*e == '\n')
        {
            ++e;
        }

        if(gs != es)
        {
            std::cerr << filename << "(" << line << "):error: compare trees: on line " << pos << ": \"" << gs << "\" != \"" << es << "\".\n"; // LCOV_EXCL_LINE
        }
        REQUIRE(gs == es);
    }

    if(*g != '\0' && *e != '\0')
    {
        throw std::logic_error("we reached this line when the previous while() implies at least one of g or e is pointing to '\\0'."); // LCOV_EXCL_LINE
    }

    if(*g != '\0')
    {
        std::cerr << filename << "(" << line << "):error: compare trees: on line " << pos << ": end of expected reached, still have \"" << g << "\" left in generated.\n"; // LCOV_EXCL_LINE
    }
    REQUIRE(*g == '\0');

    if(*e != '\0')
    {
        std::cerr << filename << "(" << line << "):error: compare trees: on line " << pos << ": end of generated reached, still have \"" << e << "\" left in expected.\n"; // LCOV_EXCL_LINE
    }
    REQUIRE(*e == '\0');
}

std::string get_script_path()
{
    return g_script_path;
}

std::string get_version_script_path()
{
    return g_version_script_path;
}

std::string get_default_variables(default_variables_flags_t const flags)
{
#define STRINGIFY_CONTENT(str)  #str
#define STRINGIFY(str)  STRINGIFY_CONTENT(str)
    return

std::string() +

"    V:_csspp_day\n"
"      LIST\n"
"        VARIABLE \"_csspp_day\"\n"
"        STRING \"02\"\n"
"    V:_csspp_e\n"
"      LIST\n"
"        VARIABLE \"_csspp_e\"\n"
"        DECIMAL_NUMBER \"\" D:2.718\n"
"    V:_csspp_hour\n"
"      LIST\n"
"        VARIABLE \"_csspp_hour\"\n"
"        STRING \"14\"\n"
"    V:_csspp_ln10e\n"
"      LIST\n"
"        VARIABLE \"_csspp_ln10e\"\n"
"        DECIMAL_NUMBER \"\" D:2.303\n"
"    V:_csspp_ln2e\n"
"      LIST\n"
"        VARIABLE \"_csspp_ln2e\"\n"
"        DECIMAL_NUMBER \"\" D:0.693\n"
"    V:_csspp_log10e\n"
"      LIST\n"
"        VARIABLE \"_csspp_log10e\"\n"
"        DECIMAL_NUMBER \"\" D:0.434\n"
"    V:_csspp_log2e\n"
"      LIST\n"
"        VARIABLE \"_csspp_log2e\"\n"
"        DECIMAL_NUMBER \"\" D:1.443\n"
"    V:_csspp_major\n"
"      LIST\n"
"        VARIABLE \"_csspp_major\"\n"
"        INTEGER \"\" I:" STRINGIFY(CSSPP_VERSION_MAJOR) "\n"
"    V:_csspp_minor\n"
"      LIST\n"
"        VARIABLE \"_csspp_minor\"\n"
"        INTEGER \"\" I:" STRINGIFY(CSSPP_VERSION_MINOR) "\n"
"    V:_csspp_minute\n"
"      LIST\n"
"        VARIABLE \"_csspp_minute\"\n"
"        STRING \"16\"\n"
"    V:_csspp_month\n"
"      LIST\n"
"        VARIABLE \"_csspp_month\"\n"
"        STRING \"07\"\n"
"    V:_csspp_no_logo\n"
"      LIST\n"
"        VARIABLE \"_csspp_no_logo\"\n"
"        BOOLEAN B:" + ((flags & flag_no_logo_true) != 0 ? "true" : "false") + "\n"
"    V:_csspp_patch\n"
"      LIST\n"
"        VARIABLE \"_csspp_patch\"\n"
"        INTEGER \"\" I:" STRINGIFY(CSSPP_VERSION_PATCH) "\n"
"    V:_csspp_pi\n"
"      LIST\n"
"        VARIABLE \"_csspp_pi\"\n"
"        DECIMAL_NUMBER \"\" D:3.142\n"
"    V:_csspp_pi_rad\n"
"      LIST\n"
"        VARIABLE \"_csspp_pi_rad\"\n"
"        DECIMAL_NUMBER \"rad\" D:3.142\n"
"    V:_csspp_second\n"
"      LIST\n"
"        VARIABLE \"_csspp_second\"\n"
"        STRING \"38\"\n"
"    V:_csspp_sqrt2\n"
"      LIST\n"
"        VARIABLE \"_csspp_sqrt2\"\n"
"        DECIMAL_NUMBER \"\" D:1.414\n"
"    V:_csspp_time\n"
"      LIST\n"
"        VARIABLE \"_csspp_time\"\n"
"        STRING \"14:16:38\"\n"
"    V:_csspp_usdate\n"
"      LIST\n"
"        VARIABLE \"_csspp_usdate\"\n"
"        STRING \"07/02/2015\"\n"
"    V:_csspp_version\n"
"      LIST\n"
"        VARIABLE \"_csspp_version\"\n"
"        STRING \"" CSSPP_VERSION "\"\n"
"    V:_csspp_year\n"
"      LIST\n"
"        VARIABLE \"_csspp_year\"\n"
"        STRING \"2015\"\n"
"    V:adjust_hue\n"
"      LIST\n"
"        FUNCTION \"adjust_hue\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"angle\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"hsla\"\n"
"                FUNCTION \"deg2rad\"\n"
"                  FUNCTION \"hue\"\n"
"                    VARIABLE \"color\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                VARIABLE \"angle\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"saturation\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"lightness\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"alpha\"\n"
"                  VARIABLE \"color\"\n"
"    V:complement\n"
"      LIST\n"
"        FUNCTION \"complement\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"adjust_hue\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                VARIABLE \"_csspp_pi_rad\"\n"
"    V:darken\n"
"      LIST\n"
"        FUNCTION \"darken\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"percent\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"lighten\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                VARIABLE \"percent\"\n"
"    V:deg2rad\n"
"      LIST\n"
"        FUNCTION \"deg2rad\"\n"
"          ARG\n"
"            VARIABLE \"angle\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              VARIABLE \"angle\"\n"
"              WHITESPACE\n"
"              MULTIPLY\n"
"              WHITESPACE\n"
"              VARIABLE \"_csspp_pi_rad\"\n"
"              WHITESPACE\n"
"              DIVIDE\n"
"              WHITESPACE\n"
"              INTEGER \"deg\" I:180\n"
"    V:desaturate\n"
"      LIST\n"
"        FUNCTION \"desaturate\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"percent\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"saturate\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                VARIABLE \"percent\"\n"
"    V:fade_in\n"
"      LIST\n"
"        FUNCTION \"fade_in\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"number\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"opacify\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                VARIABLE \"number\"\n"
"    V:fade_out\n"
"      LIST\n"
"        FUNCTION \"fade_out\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"number\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"transparentize\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                VARIABLE \"number\"\n"
"    V:grayscale\n"
"      LIST\n"
"        FUNCTION \"grayscale\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"saturate\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                FUNCTION \"saturation\"\n"
"                  VARIABLE \"color\"\n"
"    V:invert\n"
"      LIST\n"
"        FUNCTION \"invert\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"rgba\"\n"
"                DECIMAL_NUMBER \"\" D:255\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                FUNCTION \"red\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                DECIMAL_NUMBER \"\" D:255\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                FUNCTION \"green\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                DECIMAL_NUMBER \"\" D:255\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                FUNCTION \"blue\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"alpha\"\n"
"                  VARIABLE \"color\"\n"
"    V:lighten\n"
"      LIST\n"
"        FUNCTION \"lighten\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"percent\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"hsla\"\n"
"                FUNCTION \"hue\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"saturation\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"lightness\"\n"
"                  VARIABLE \"color\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                VARIABLE \"percent\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"alpha\"\n"
"                  VARIABLE \"color\"\n"
"    V:mix\n"
"      LIST\n"
"        FUNCTION \"mix\"\n"
"          ARG\n"
"            VARIABLE \"color1\"\n"
"          ARG\n"
"            VARIABLE \"color2\"\n"
"          ARG\n"
"            VARIABLE \"weight\"\n"
"            DECIMAL_NUMBER \"\" D:0.5\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              VARIABLE \"color1\"\n"
"              WHITESPACE\n"
"              MULTIPLY\n"
"              WHITESPACE\n"
"              VARIABLE \"weight\"\n"
"              WHITESPACE\n"
"              ADD\n"
"              WHITESPACE\n"
"              VARIABLE \"color2\"\n"
"              WHITESPACE\n"
"              MULTIPLY\n"
"              OPEN_PARENTHESIS\n"
"                DECIMAL_NUMBER \"\" D:1\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                VARIABLE \"weight\"\n"
"    V:opacify\n"
"      LIST\n"
"        FUNCTION \"opacify\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"number\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"rgba\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"alpha\"\n"
"                  VARIABLE \"color\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                VARIABLE \"number\"\n"
"    V:opacity\n"
"      LIST\n"
"        FUNCTION \"opacity\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"alpha\"\n"
"                VARIABLE \"color\"\n"
"    V:quote\n"
"      LIST\n"
"        FUNCTION \"quote\"\n"
"          ARG\n"
"            VARIABLE \"identifier\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"string\"\n"
"                VARIABLE \"identifier\"\n"
"    V:remove_unit\n"
"      LIST\n"
"        FUNCTION \"remove_unit\"\n"
"          ARG\n"
"            VARIABLE \"value\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"unit\"\n"
"                VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              EQUAL\n"
"              WHITESPACE\n"
"              STRING \"%\"\n"
"              WHITESPACE\n"
"              CONDITIONAL\n"
"              WHITESPACE\n"
"              FUNCTION \"decimal_number\"\n"
"                VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              COLON\n"
"              WHITESPACE\n"
"              FUNCTION \"type_of\"\n"
"                VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              EQUAL\n"
"              WHITESPACE\n"
"              STRING \"integer\"\n"
"              WHITESPACE\n"
"              CONDITIONAL\n"
"              WHITESPACE\n"
"              VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              DIVIDE\n"
"              WHITESPACE\n"
"              FUNCTION \"integer\"\n"
"                STRING \"1\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                FUNCTION \"unit\"\n"
"                  VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              COLON\n"
"              WHITESPACE\n"
"              VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              DIVIDE\n"
"              WHITESPACE\n"
"              FUNCTION \"decimal_number\"\n"
"                STRING \"1\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                FUNCTION \"unit\"\n"
"                  VARIABLE \"value\"\n"
"    V:saturate\n"
"      LIST\n"
"        FUNCTION \"saturate\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"percent\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"hsla\"\n"
"                FUNCTION \"hue\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"saturation\"\n"
"                  VARIABLE \"color\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                VARIABLE \"percent\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"lightness\"\n"
"                  VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"alpha\"\n"
"                  VARIABLE \"color\"\n"
"    V:set_unit\n"
"      LIST\n"
"        FUNCTION \"set_unit\"\n"
"          ARG\n"
"            VARIABLE \"value\"\n"
"          ARG\n"
"            VARIABLE \"unit\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"string\"\n"
"                VARIABLE \"unit\"\n"
"              WHITESPACE\n"
"              EQUAL\n"
"              WHITESPACE\n"
"              STRING \"%\"\n"
"              WHITESPACE\n"
"              CONDITIONAL\n"
"              WHITESPACE\n"
"              FUNCTION \"percentage\"\n"
"                VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              COLON\n"
"              WHITESPACE\n"
"              FUNCTION \"type_of\"\n"
"                VARIABLE \"value\"\n"
"              WHITESPACE\n"
"              EQUAL\n"
"              WHITESPACE\n"
"              STRING \"integer\"\n"
"              WHITESPACE\n"
"              CONDITIONAL\n"
"              WHITESPACE\n"
"              FUNCTION \"integer\"\n"
"                FUNCTION \"string\"\n"
"                  FUNCTION \"remove_unit\"\n"
"                    VARIABLE \"value\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                FUNCTION \"string\"\n"
"                  VARIABLE \"unit\"\n"
"              WHITESPACE\n"
"              COLON\n"
"              WHITESPACE\n"
"              FUNCTION \"decimal_number\"\n"
"                FUNCTION \"string\"\n"
"                  FUNCTION \"remove_unit\"\n"
"                    VARIABLE \"value\"\n"
"                WHITESPACE\n"
"                ADD\n"
"                WHITESPACE\n"
"                FUNCTION \"string\"\n"
"                  VARIABLE \"unit\"\n"
"    V:transparentize\n"
"      LIST\n"
"        FUNCTION \"transparentize\"\n"
"          ARG\n"
"            VARIABLE \"color\"\n"
"          ARG\n"
"            VARIABLE \"number\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"rgba\"\n"
"                VARIABLE \"color\"\n"
"                COMMA\n"
"                WHITESPACE\n"
"                FUNCTION \"alpha\"\n"
"                  VARIABLE \"color\"\n"
"                WHITESPACE\n"
"                SUBTRACT\n"
"                WHITESPACE\n"
"                VARIABLE \"number\"\n"
"    V:unitless\n"
"      LIST\n"
"        FUNCTION \"unitless\"\n"
"          ARG\n"
"            VARIABLE \"number\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"unit\"\n"
"                VARIABLE \"number\"\n"
"              WHITESPACE\n"
"              EQUAL\n"
"              WHITESPACE\n"
"              STRING \"\"\n"
"    V:unquote\n"
"      LIST\n"
"        FUNCTION \"unquote\"\n"
"          ARG\n"
"            VARIABLE \"string\"\n"
"        OPEN_CURLYBRACKET B:false\n"
"          COMPONENT_VALUE\n"
"            AT_KEYWORD \"return\" I:0\n"
"              FUNCTION \"identifier\"\n"
"                VARIABLE \"string\"\n"

;

#undef STRINGIFY
#undef STRINGIFY_CONTENT
}

std::string get_close_comment(bool token)
{
    if(token)
    {
        return  "  COMMENT \"@preserve -- CSS file parsed by http://csspp.org/ v" CSSPP_VERSION " on 07/02/2015\" I:1\n";
    }
    else
    {
        return "/* @preserve -- CSS file parsed by http://csspp.org/ v" CSSPP_VERSION " on 07/02/2015 */\n";
    }
}

time_t get_now()
{
    return g_now;
}

} // csspp_test namespace

int main(int argc, char *argv[])
{
    // define program name
    csspp_test::g_progname = argv[0];
    char *e(strrchr(csspp_test::g_progname, '/'));
    if(e)
    {
        csspp_test::g_progname = e + 1; // LCOV_EXCL_LINE
    }
    e = strrchr(csspp_test::g_progname, '\\');
    if(e)
    {
        csspp_test::g_progname = e + 1; // LCOV_EXCL_LINE
    }

    unsigned int seed(static_cast<unsigned int>(time(nullptr)));
    bool help(false);
    for(int i(1); i < argc;)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            help = true; // LCOV_EXCL_LINE
            ++i;
        }
        else if(strcmp(argv[i], "--seed") == 0)
        {
            if(i + 1 >= argc) // LCOV_EXCL_LINE
            {
                std::cerr << "error: --seed need to be followed by the actual seed." << std::endl; // LCOV_EXCL_LINE
                exit(1); // LCOV_EXCL_LINE
            }
            seed = atoll(argv[i + 1]); // LCOV_EXCL_LINE
            // remove the --seed and <value>
            argc -= 2; // LCOV_EXCL_LINE
            for(int j(i); j < argc; ++j) // LCOV_EXCL_LINE
            {
                argv[j] = argv[j + 2]; // LCOV_EXCL_LINE
            }
        }
        else if(strcmp(argv[i], "--show-errors") == 0)
        {
            csspp::error::instance().set_verbose(true);
            argc -= 1; // LCOV_EXCL_LINE
            for(int j(i); j < argc; ++j) // LCOV_EXCL_LINE
            {
                argv[j] = argv[j + 1]; // LCOV_EXCL_LINE
            }
        }
        else if(strcmp(argv[i], "--scripts") == 0)
        {
            if(i + 1 >= argc)
            {
                std::cerr << "error: --scripts need to be followed by a path." << std::endl; // LCOV_EXCL_LINE
                exit(1); // LCOV_EXCL_LINE
            }
            csspp_test::g_script_path = argv[i + 1];
            // remove the --scripts and <value>
            argc -= 2; // LCOV_EXCL_LINE
            for(int j(i); j < argc; ++j) // LCOV_EXCL_LINE
            {
                argv[j] = argv[j + 2]; // LCOV_EXCL_LINE
            }
        }
        else if(strcmp(argv[i], "--version-script") == 0)
        {
            if(i + 1 >= argc)
            {
                std::cerr << "error: --version-script need to be followed by a path." << std::endl; // LCOV_EXCL_LINE
                exit(1); // LCOV_EXCL_LINE
            }
            csspp_test::g_version_script_path = argv[i + 1];
            // remove the --version-script and <value>
            argc -= 2; // LCOV_EXCL_LINE
            for(int j(i); j < argc; ++j) // LCOV_EXCL_LINE
            {
                argv[j] = argv[j + 2]; // LCOV_EXCL_LINE
            }
        }
        else if(strcmp(argv[i], "--version") == 0)
        {
            std::cout << CSSPP_VERSION << std::endl;
            exit(0);
        }
        else
        {
            ++i;
        }
    }
    srand(seed);
    std::cout << csspp_test::g_progname << "[" << getpid() << "]" << ": version " << CSSPP_VERSION << ", seed is " << seed << std::endl;

    // unless we get a loop going forever, we should never hit this limit
    csspp::node::limit_nodes_to(1000000);

    if(help)
    {
        std::cout << std::endl // LCOV_EXCL_LINE
                  << "WARNING: at this point we hack the main() to add the following options:" << std::endl // LCOV_EXCL_LINE
                  << "  --scripts <path>          a path to the system scripts to run against the tests" << std::endl // LCOV_EXCL_LINE
                  << "  --seed <seed>             to force the seed at the start of the process to a specific value (i.e. to reproduce the exact same test over and over again)" << std::endl // LCOV_EXCL_LINE
                  << "  --show-errors             request for the errors to always be printed in std::cerr" << std::endl // LCOV_EXCL_LINE
                  << "  --version                 print out the version of this test and exit with 0" << std::endl // LCOV_EXCL_LINE
                  << "  --version-script <path>   a path to the system version script" << std::endl // LCOV_EXCL_LINE
                  << std::endl; // LCOV_EXCL_LINE
    }

    // before running we need to initialize the error tracker
    static_cast<void>(csspp_test::trace_error::instance());

    return Catch::Session().run(argc, argv);
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
