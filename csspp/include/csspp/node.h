#ifndef CSSPP_NODE_H
#define CSSPP_NODE_H
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

#include <string>
#include <memory>

namespace csspp
{

enum class node_type_t
{
    UNKNOWN,

    // basic token
    ADD,
    AT_KEYWORD,
    CDC,
    CDO,
    CLOSE_CURLYBRACKET,
    CLOSE_PARENTHESIS,
    CLOSE_SQUAREBRACKET,
    COLON,
    COLUMN,
    COMMA,
    COMMENT,
    DASH_MATCH,
    DECIMAL_NUMBER,
    //DIMENSION, -- DECIMAL_NUMBER and INTEGER with a string are dimensions
    DIVIDE,
    DOLLAR,
    EOF_TOKEN,
    EQUAL,
    EXCLAMATION,
    FUNCTION,
    GREATER_THAN,
    HASH,
    IDENTIFIER,
    INCLUDE_MATCH,
    INTEGER,
    MULTIPLY,
    OPEN_CURLYBRACKET,
    OPEN_PARENTHESIS,
    OPEN_SQUAREBRACKET,
    PERCENT,
    PERIOD,
    PREFIX_MATCH,
    SCOPE,
    SEMICOLON,
    STRING,
    SUBSTRING_MATCH,
    SUBTRACT,
    SUFFIX_MATCH,
    UNICODE_RANGE,
    URL,
    WHITESPACE,

    // composed tokens
    CHARSET,                // @charset = @charset <string> ;
    FONTFACE,               // @font-face { <declaration-list> }
    KEYFRAME,               // <keyframe-selector> { <declaration-list> }
    KEYFRAMES,              // @keyframes <keyframes-name> { <rule-list> }
    MEDIA,                  // @media <media-query-list> { <stylesheet> }

    max_type
};

class node
{
public:
    typedef std::shared_ptr<node>   pointer_t;

                        node(node_type_t const type, position const & pos);

    node_type_t             get_type() const;
    bool                    is(node_type_t const type) const;

    position const &    get_position() const;
    std::string const & get_string() const;
    void                set_string(std::string const & str);
    integer_t           get_integer() const;
    void                set_integer(integer_t integer);
    decimal_number_t    get_decimal_number() const;
    void                set_decimal_number(decimal_number_t decimal_number);

private:
    node_type_t         f_type = node_type_t::UNKNOWN;
    position            f_position;
    integer_t           f_integer = 0;
    decimal_number_t    f_decimal_number = 0.0;
    std::string         f_string;
};

} // namespace csspp

std::ostream & operator << (std::ostream & out, csspp::node_type_t const type);

#endif
// #ifndef CSSPP_NODE_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
