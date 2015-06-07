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

#include "csspp/lexer.h"

#include "csspp/error.h"
#include "csspp/exceptions.h"

#include <cmath>
#include <iostream>
#include <sstream>

namespace csspp
{

namespace
{

// the error instance
error *g_error = nullptr;

} // no name namespace

error::error()
    : f_position("error.css")
{
}

error & error::instance()
{
    // first time, allocate the instance
    if(g_error == nullptr)
    {
        g_error = new error;
    }

    g_error->reset();

    return *g_error;
}

void error::set_error_stream(std::ostream & err_stream)
{
    f_error = &err_stream;
}

error & error::operator << (position const & pos)
{
    f_position = pos;
    return *this;
}

error & error::operator << (error_mode_t mode)
{
    switch(mode)
    {
    case error_mode_t::ERROR_DEC:
        f_message << std::dec;
        break;

    case error_mode_t::ERROR_FATAL:
    case error_mode_t::ERROR_ERROR:
    case error_mode_t::ERROR_WARNING:
        // print the error now
        // (show the page number?)
        {
            std::ostream * out = f_error ? f_error : &std::cerr;
            *out << f_position.get_filename()
                 << "(" << f_position.get_line() << "): " << mode << ": "
                 << f_message.str()
                 << std::endl;
        }
        break;

    case error_mode_t::ERROR_HEX:
        f_message << std::hex;
        break;

    }

    return *this;
}

error & error::operator << (std::string const & msg)
{
    f_message << msg;
    return *this;
}

error & error::operator << (char const * msg)
{
    f_message << msg;
    return *this;
}

error & error::operator << (int32_t value)
{
    f_message << value;
    return *this;
}

error & error::operator << (int64_t value)
{
    f_message << value;
    return *this;
}

error & error::operator << (double value)
{
    f_message << value;
    return *this;
}

void error::reset()
{
    // reset the output message
    f_message.str("");
    f_message << std::dec;
}

} // namespace csspp

std::ostream & operator << (std::ostream & out, csspp::error_mode_t const type)
{
    switch(type)
    {
    case csspp::error_mode_t::ERROR_DEC:
        out << "dec";
        break;

    case csspp::error_mode_t::ERROR_ERROR:
        out << "error";
        break;

    case csspp::error_mode_t::ERROR_FATAL:
        out << "fatal";
        break;

    case csspp::error_mode_t::ERROR_HEX:
        out << "hex";
        break;

    case csspp::error_mode_t::ERROR_WARNING:
        out << "warning";
        break;

    }

    return out;
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
