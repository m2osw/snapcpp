#ifndef CSSPP_ERROR_H
#define CSSPP_ERROR_H
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

#include "csspp/position.h"

#include <istream>
#include <sstream>

namespace csspp
{

enum class error_mode_t
{
    ERROR_DEC,
    ERROR_ERROR,
    ERROR_FATAL,
    ERROR_HEX,
    ERROR_WARNING
};

class error
{
public:
                            error();

    static error &          instance();
    void                    set_error_stream(std::ostream & err_stream);

    error &                 operator << (position const & pos);
    error &                 operator << (error_mode_t mode);
    error &                 operator << (std::string const & msg);
    error &                 operator << (char const * msg);
    error &                 operator << (int32_t value);
    error &                 operator << (int64_t value);
    error &                 operator << (double value);

private:
    void                    reset();

    position                f_position;
    std::stringstream       f_message;
    std::ostream *          f_error = nullptr;
};

} // namespace csspp

std::ostream & operator << (std::ostream & out, csspp::error_mode_t const type);

#endif
// #ifndef CSSPP_LEXER_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
