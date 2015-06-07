#ifndef CATCH_TESTS_H
#define CATCH_TESTS_H
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
 * \brief Common header for all our catch tests.
 *
 * csspp comes with a unit test suite. This header defines things
 * that all the tests access, such as the catch.hpp header file.
 */

#include <csspp/unicode_range.h>

#include <catch.hpp>

namespace csspp_test
{

class trace_error
{
public:
                            trace_error();

    static trace_error &    instance();

    void                    expected_error(std::string const & msg);

private:
    std::stringstream       m_error_message;
};

class our_unicode_range_t
{
public:
                            our_unicode_range_t(csspp::wide_char_t start, csspp::wide_char_t end);

    void                    set_start(csspp::wide_char_t start);
    void                    set_end(csspp::wide_char_t end);
    void                    set_range(csspp::range_value_t range);

    csspp::wide_char_t      get_start() const;
    csspp::wide_char_t      get_end() const;
    csspp::range_value_t    get_range() const;

private:
    csspp::wide_char_t      f_start;
    csspp::wide_char_t      f_end;
};

} // csspp_test namespace
#endif
// #ifndef CSSPP_TESTS_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
