#ifndef CSSPP_LEXER_H
#define CSSPP_LEXER_H
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

#include "csspp/node.h"

#include <istream>

namespace csspp
{

class lexer
{
public:
                            lexer(std::istream & in, position const & pos);

    node::pointer_t         next_token();

    wide_char_t             mbtowc(char const * mb);
    void                    wctomb(wide_char_t const wc, char * mb, size_t max_length);
    std::string             wctomb(wide_char_t const wc);

private:
    static size_t const     UNGETSIZ = 16;

    wide_char_t             getc();
    void                    ungetc(wide_char_t c);
    static bool constexpr   is_space(wide_char_t c);
    static bool constexpr   is_non_printable(wide_char_t c);
    static bool constexpr   is_identifier(wide_char_t c);
    static bool constexpr   is_start_identifier(wide_char_t c);
    static bool constexpr   is_digit(wide_char_t c);
    static bool constexpr   is_hex(wide_char_t c);
    static bool constexpr   is_hash_character(wide_char_t c);
    static int              hex_to_dec(wide_char_t c);

    wide_char_t             escape();
    node::pointer_t         identifier(wide_char_t c);
    node::pointer_t         number(wide_char_t c);
    std::string             string(wide_char_t const quote);
    node::pointer_t         comment(bool c_comment);
    node::pointer_t         unicode_range(wide_char_t c);
    node::pointer_t         hash();

    std::istream &          f_in;
    position                f_position;
    position                f_start_position;
    wide_char_t             f_ungetc[UNGETSIZ];
    size_t                  f_ungetc_pos = 0;
};

} // namespace csspp
#endif
// #ifndef CSSPP_LEXER_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
