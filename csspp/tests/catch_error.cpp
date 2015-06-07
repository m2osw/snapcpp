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
 * \brief Test the error.cpp file.
 *
 * This test runs a battery of tests agains the error.cpp
 * implementation to ensure full coverage.
 */

#include "catch_tests.h"

#include "csspp/error.h"
#include "csspp/exceptions.h"
#include "csspp/lexer.h"
#include "csspp/unicode_range.h"

#include <sstream>

#include <string.h>

namespace
{


} // no name namespace


TEST_CASE("Error names", "[error]")
{
    csspp::error_mode_t e(csspp::error_mode_t::ERROR_DEC);
    while(e <= csspp::error_mode_t::ERROR_WARNING)
    {
        std::stringstream ss;
        ss << e;
        std::string const name(ss.str());

        switch(e)
        {
        case csspp::error_mode_t::ERROR_DEC:
            REQUIRE(name == "dec");
            break;

        case csspp::error_mode_t::ERROR_ERROR:
            REQUIRE(name == "error");
            break;

        case csspp::error_mode_t::ERROR_FATAL:
            REQUIRE(name == "fatal");
            break;

        case csspp::error_mode_t::ERROR_HEX:
            REQUIRE(name == "hex");
            break;

        case csspp::error_mode_t::ERROR_WARNING:
            REQUIRE(name == "warning");
            break;

        }

        e = static_cast<csspp::error_mode_t>(static_cast<int>(e) + 1);
    }

    // no error left over
    csspp_test::trace_error::instance().expected_error("");
}

TEST_CASE("Error messages", "[error]")
{
    csspp::position p("test.css");

    csspp::error::instance() << p << "testing errors: "
                             << 123
                             << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                             << "."
                             << csspp::error_mode_t::ERROR_FATAL;
    csspp_test::trace_error::instance().expected_error("test.css(1): fatal: testing errors: 123 U+7b.\n");

    int64_t cs(123);
    csspp::error::instance() << p << std::string("testing errors:")
                             << " U+" << csspp::error_mode_t::ERROR_HEX << cs
                             << " (" << csspp::error_mode_t::ERROR_DEC << 123
                             << ")."
                             << csspp::error_mode_t::ERROR_ERROR;
    csspp_test::trace_error::instance().expected_error("test.css(1): error: testing errors: U+7b (123).\n");

    csspp::error::instance() << p << "testing warnings:"
                             << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                             << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                             << "."
                             << csspp::error_mode_t::ERROR_WARNING;
    csspp_test::trace_error::instance().expected_error("test.css(1): warning: testing warnings: U+7b decimal: 123.25.\n");

    // no error left over
    csspp_test::trace_error::instance().expected_error("");
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
