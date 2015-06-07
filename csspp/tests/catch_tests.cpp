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

#include <cstring>

#include <stdlib.h>

namespace csspp_test
{

// static variables
namespace
{

char * g_progname;

trace_error * g_trace_error;

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

void trace_error::expected_error(std::string const & msg)
{
//std::cerr << "require " << msg << "\n";
//std::cerr << "got " << m_error_message.str() << "\n";
    REQUIRE(m_error_message.str() == msg);
    m_error_message.str("");
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
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            help = true; // LCOV_EXCL_LINE
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
        else if(strcmp(argv[i], "--version") == 0)
        {
            std::cout << CSSPP_VERSION << std::endl;
            exit(0);
        }
    }
    srand(seed);
    std::cout << csspp_test::g_progname << "[" << getpid() << "]" << ": version " << CSSPP_VERSION << ", seed is " << seed << std::endl;

    if(help)
    {
        std::cout << std::endl // LCOV_EXCL_LINE
                  << "WARNING: at this point we hack the main() to add the following options:" << std::endl // LCOV_EXCL_LINE
                  << "  --seed <seed>    to force the seed at the start of the process to a specific value (i.e. to reproduce the exact same test over and over again)" << std::endl // LCOV_EXCL_LINE
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
