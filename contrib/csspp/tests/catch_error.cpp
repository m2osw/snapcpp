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
        case csspp::error_mode_t::ERROR_DEBUG:
            REQUIRE(name == "debug");
            break;

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

        case csspp::error_mode_t::ERROR_INFO:
            REQUIRE(name == "info");
            break;

        case csspp::error_mode_t::ERROR_WARNING:
            REQUIRE(name == "warning");
            break;

        }

        e = static_cast<csspp::error_mode_t>(static_cast<int>(e) + 1);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Error messages", "[error] [output]")
{
    csspp::error_count_t error_count(csspp::error::instance().get_error_count());
    csspp::error_count_t warning_count(csspp::error::instance().get_warning_count());

    csspp::position p("test.css");

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing errors: "
                                 << 123
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                 << "."
                                 << csspp::error_mode_t::ERROR_FATAL;
        REQUIRE_ERRORS("test.css(1): fatal: testing errors: 123 U+7b.\n");
        ++error_count;
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        int64_t cs(83);
        csspp::error::instance() << p << std::string("testing errors:")
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << cs
                                 << " (" << csspp::error_mode_t::ERROR_DEC << 133
                                 << ")."
                                 << csspp::error_mode_t::ERROR_ERROR;
        REQUIRE_ERRORS("test.css(1): error: testing errors: U+53 (133).\n");
        ++error_count;
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::safe_error_t safe_error;

        {
            csspp::error_happened_t happened;

            csspp::error::instance() << p << "testing warnings:"
                                     << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                     << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                                     << "."
                                     << csspp::error_mode_t::ERROR_WARNING;
            REQUIRE_ERRORS("test.css(1): warning: testing warnings: U+7b decimal: 123.25.\n");
            ++warning_count;
            REQUIRE(error_count == csspp::error::instance().get_error_count());
            REQUIRE(warning_count == csspp::error::instance().get_warning_count());

            REQUIRE_FALSE(happened.error_happened());
            REQUIRE(happened.warning_happened());
        }

        {
            csspp::error_happened_t happened;

            csspp::error::instance().set_count_warnings_as_errors(true);
            csspp::error::instance() << p << "testing warnings:"
                                     << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                     << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                                     << "."
                                     << csspp::error_mode_t::ERROR_WARNING;
            REQUIRE_ERRORS("test.css(1): warning: testing warnings: U+7b decimal: 123.25.\n");
            ++error_count;
            REQUIRE(error_count == csspp::error::instance().get_error_count());
            REQUIRE(warning_count == csspp::error::instance().get_warning_count());
            csspp::error::instance().set_count_warnings_as_errors(false);

            REQUIRE(happened.error_happened());
            REQUIRE_FALSE(happened.warning_happened());
        }

        {
            csspp::error_happened_t happened;

            csspp::error::instance() << p << "testing warnings:"
                                     << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                     << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                                     << "."
                                     << csspp::error_mode_t::ERROR_WARNING;
            REQUIRE_ERRORS("test.css(1): warning: testing warnings: U+7b decimal: 123.25.\n");
            ++warning_count;
            REQUIRE(error_count == csspp::error::instance().get_error_count());
            REQUIRE(warning_count == csspp::error::instance().get_warning_count());

            REQUIRE_FALSE(happened.error_happened());
            REQUIRE(happened.warning_happened());
        }
    }
    // the safe_error restores the counters to what they were before the '{'
    --error_count;
    warning_count -= 2;
    REQUIRE(error_count == csspp::error::instance().get_error_count());
    REQUIRE(warning_count == csspp::error::instance().get_warning_count());

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing info:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 120
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 213.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_INFO;
        REQUIRE_ERRORS("test.css(1): info: testing info: U+78 decimal: 213.25.\n");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing debug:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 112
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 13.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_DEBUG;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance().set_show_debug(true);
        csspp::error::instance() << p << "testing debug:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 112
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 13.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_DEBUG;
        REQUIRE_ERRORS("test.css(1): debug: testing debug: U+70 decimal: 13.25.\n");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());
        csspp::error::instance().set_show_debug(false);

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing debug:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 112
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 13.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_DEBUG;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance().set_verbose(true);
        csspp::error::instance() << p << "verbose message to debug the compiler."
                                 << csspp::error_mode_t::ERROR_INFO;
        REQUIRE_ERRORS("test.css(1): info: verbose message to debug the compiler.\n");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());

        csspp::error::instance().set_verbose(false);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Error messages when hidden", "[error] [output] [hidden]")
{
    csspp::error_count_t error_count(csspp::error::instance().get_error_count());
    csspp::error_count_t warning_count(csspp::error::instance().get_warning_count());

    csspp::error::instance().set_hide_all(true);

    csspp::position p("test.css");

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing errors: "
                                 << 123
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                 << "."
                                 << csspp::error_mode_t::ERROR_FATAL;
        REQUIRE_ERRORS("test.css(1): fatal: testing errors: 123 U+7b.\n");
        ++error_count;
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        int64_t cs(83);
        csspp::error::instance() << p << std::string("testing errors:")
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << cs
                                 << " (" << csspp::error_mode_t::ERROR_DEC << 133
                                 << ")."
                                 << csspp::error_mode_t::ERROR_ERROR;
        REQUIRE_ERRORS("test.css(1): error: testing errors: U+53 (133).\n");
        ++error_count;
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::safe_error_t safe_error;

        {
            csspp::error_happened_t happened;

            csspp::error::instance() << p << "testing warnings:"
                                     << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                     << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                                     << "."
                                     << csspp::error_mode_t::ERROR_WARNING;
            REQUIRE_ERRORS("");
            REQUIRE(error_count == csspp::error::instance().get_error_count());
            REQUIRE(warning_count == csspp::error::instance().get_warning_count());

            REQUIRE_FALSE(happened.error_happened());
            REQUIRE_FALSE(happened.warning_happened());
        }

        {
            csspp::error_happened_t happened;

            csspp::error::instance().set_count_warnings_as_errors(true);
            csspp::error::instance() << p << "testing warnings:"
                                     << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                     << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                                     << "."
                                     << csspp::error_mode_t::ERROR_WARNING;
            REQUIRE_ERRORS("test.css(1): warning: testing warnings: U+7b decimal: 123.25.\n");
            ++error_count;
            REQUIRE(error_count == csspp::error::instance().get_error_count());
            REQUIRE(warning_count == csspp::error::instance().get_warning_count());
            csspp::error::instance().set_count_warnings_as_errors(false);

            REQUIRE(happened.error_happened());
            REQUIRE_FALSE(happened.warning_happened());
        }

        {
            csspp::error_happened_t happened;

            csspp::error::instance() << p << "testing warnings:"
                                     << " U+" << csspp::error_mode_t::ERROR_HEX << 123
                                     << " decimal: " << csspp::error_mode_t::ERROR_DEC << 123.25
                                     << "."
                                     << csspp::error_mode_t::ERROR_WARNING;
            REQUIRE_ERRORS("");
            REQUIRE(error_count == csspp::error::instance().get_error_count());
            REQUIRE(warning_count == csspp::error::instance().get_warning_count());

            REQUIRE_FALSE(happened.error_happened());
            REQUIRE_FALSE(happened.warning_happened());
        }
    }
    // the safe_error restores the counters to what they were before the '{'
    --error_count;
    warning_count -= 0;
    REQUIRE(error_count == csspp::error::instance().get_error_count());
    REQUIRE(warning_count == csspp::error::instance().get_warning_count());

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing info:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 120
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 213.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_INFO;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing debug:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 112
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 13.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_DEBUG;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance().set_show_debug(true);
        csspp::error::instance() << p << "testing debug:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 112
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 13.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_DEBUG;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());
        csspp::error::instance().set_show_debug(false);

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance() << p << "testing debug:"
                                 << " U+" << csspp::error_mode_t::ERROR_HEX << 112
                                 << " decimal: " << csspp::error_mode_t::ERROR_DEC << 13.25
                                 << "."
                                 << csspp::error_mode_t::ERROR_DEBUG;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());
    }

    {
        csspp::error_happened_t happened;

        csspp::error::instance().set_verbose(true);
        csspp::error::instance() << p << "verbose message to debug the compiler."
                                 << csspp::error_mode_t::ERROR_INFO;
        REQUIRE_ERRORS("");
        REQUIRE(error_count == csspp::error::instance().get_error_count());
        REQUIRE(warning_count == csspp::error::instance().get_warning_count());

        REQUIRE_FALSE(happened.error_happened());
        REQUIRE_FALSE(happened.warning_happened());

        csspp::error::instance().set_verbose(false);
    }

    csspp::error::instance().set_hide_all(false);

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Error stream", "[error] [stream]")
{
    {
        std::stringstream ss;
        std::ostream & errout(csspp::error::instance().get_error_stream());
        REQUIRE(&errout != &ss);
        {
            csspp::safe_error_stream_t safe_stream(ss);
            REQUIRE(&csspp::error::instance().get_error_stream() == &ss);
        }
        REQUIRE(&csspp::error::instance().get_error_stream() != &ss);
        REQUIRE(&csspp::error::instance().get_error_stream() == &errout);
    }
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
