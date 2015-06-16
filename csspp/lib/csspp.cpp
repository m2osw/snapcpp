// CSS Preprocessor
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

#include "csspp/csspp.h"

#include "csspp/exceptions.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace csspp
{

namespace
{

int g_precision = 3;

} // no name namespace

char const * csspp_library_version()
{
    return CSSPP_VERSION;
}

int get_precision()
{
    return g_precision;
}

void set_precision(int precision)
{
    if(precision < 0 || precision > 10)
    {
        throw csspp_exception_overflow("precision must be between 0 and 10, " + std::to_string(precision) + " is out of bounds.");
    }

    g_precision = precision;
}

std::string decimal_number_to_string(decimal_number_t d)
{
    // the std::setprecision() is a total number of digits when we
    // want a specific number of digits after the decimal point so
    // we use the following algorithm for it:
    std::stringstream ss;

    // make sure to round the value up first
    if(d >= 0)
    {
        ss << d + 5.0 / pow(10, g_precision + 1);
    }
    else
    {
        ss << d - 5.0 / pow(10, g_precision + 1);
    }

    std::string out(ss.str());

    // check wether the number of digits after the decimal point is too large
    std::string::size_type end(out.find('.') + g_precision + 1);
    if(end != std::string::npos
    && out.length() > end)
    {
        // remove anything extra
        out = out.substr(0, end);
    }
    if(end != std::string::npos
    && end > 0)
    {
        while(out.back() == '0')
        {
            out.erase(out.end() - 1);
        }
        if(out.back() == '.')
        {
            out.erase(out.end() - 1);
        }
    }

    return out;
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et

