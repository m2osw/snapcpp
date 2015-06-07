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
 * \brief Test the csspp.cpp file.
 *
 * This test runs a battery of tests agains the csspp.cpp
 * implementation to ensure full coverage.
 */

#include "catch_tests.h"

#include "csspp/exceptions.h"
#include "csspp/lexer.h"
#include "csspp/unicode_range.h"

#include <sstream>

#include <string.h>

namespace
{


} // no name namespace


TEST_CASE("Version string", "[csspp]")
{
    // we expect the test suite to be compiled with the exact same version
    REQUIRE(csspp::csspp_library_version() == std::string(CSSPP_VERSION));

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
