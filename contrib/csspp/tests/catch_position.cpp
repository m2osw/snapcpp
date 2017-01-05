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

namespace
{


} // no name namespace


TEST_CASE("Position defaults", "[position] [defaults]")
{
    {
        csspp::position pos("pos.css");

        REQUIRE(pos.get_filename() == "pos.css");
        REQUIRE(pos.get_line() == 1);
        REQUIRE(pos.get_page() == 1);
        REQUIRE(pos.get_total_line() == 1);

        csspp::position other("other.css");

        REQUIRE(other.get_filename() == "other.css");
        REQUIRE(other.get_line() == 1);
        REQUIRE(other.get_page() == 1);
        REQUIRE(other.get_total_line() == 1);

        // copy works as expected?
        other = pos;

        REQUIRE(pos.get_filename() == "pos.css");
        REQUIRE(pos.get_line() == 1);
        REQUIRE(pos.get_page() == 1);
        REQUIRE(pos.get_total_line() == 1);

        REQUIRE(other.get_filename() == "pos.css");
        REQUIRE(other.get_line() == 1);
        REQUIRE(other.get_page() == 1);
        REQUIRE(other.get_total_line() == 1);
    }

    // no error left over
    REQUIRE_ERRORS("");
}

TEST_CASE("Position counters", "[position] [count]")
{
    // simple check to verify there is no interaction between
    // a copy and the original
    {
        csspp::position pos("pos.css");

        REQUIRE(pos.get_filename() == "pos.css");
        REQUIRE(pos.get_line() == 1);
        REQUIRE(pos.get_page() == 1);
        REQUIRE(pos.get_total_line() == 1);

        csspp::position other("other.css");

        REQUIRE(other.get_filename() == "other.css");
        REQUIRE(other.get_line() == 1);
        REQUIRE(other.get_page() == 1);
        REQUIRE(other.get_total_line() == 1);

        // copy works as expected?
        other = pos;

        REQUIRE(pos.get_filename() == "pos.css");
        REQUIRE(pos.get_line() == 1);
        REQUIRE(pos.get_page() == 1);
        REQUIRE(pos.get_total_line() == 1);

        REQUIRE(other.get_filename() == "pos.css"); // filename changed!
        REQUIRE(other.get_line() == 1);
        REQUIRE(other.get_page() == 1);
        REQUIRE(other.get_total_line() == 1);

        // increment does not affect another position
        other.next_line();

        REQUIRE(pos.get_filename() == "pos.css");
        REQUIRE(pos.get_line() == 1);
        REQUIRE(pos.get_page() == 1);
        REQUIRE(pos.get_total_line() == 1);

        REQUIRE(other.get_filename() == "pos.css");
        REQUIRE(other.get_line() == 2);
        REQUIRE(other.get_page() == 1);
        REQUIRE(other.get_total_line() == 2);

        // increment does not affect another position
        other.next_page();

        REQUIRE(pos.get_filename() == "pos.css");
        REQUIRE(pos.get_line() == 1);
        REQUIRE(pos.get_page() == 1);
        REQUIRE(pos.get_total_line() == 1);

        REQUIRE(other.get_filename() == "pos.css");
        REQUIRE(other.get_line() == 1);
        REQUIRE(other.get_page() == 2);
        REQUIRE(other.get_total_line() == 2);
    }

    // loop and increment line/page counters
    {
        csspp::position pos("counters.css");
        int line(1);
        int page(1);
        int total_line(1);

        for(int i(0); i < 1000; ++i)
        {
            if(rand() & 1)
            {
                pos.next_line();
                ++line;
                ++total_line;
            }
            else
            {
                pos.next_page();
                line = 1;
                ++page;
                //++total_line; -- should this happen?
            }

            REQUIRE(pos.get_filename() == "counters.css");
            REQUIRE(pos.get_line() == line);
            REQUIRE(pos.get_page() == page);
            REQUIRE(pos.get_total_line() == total_line);
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
