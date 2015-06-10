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

#include <csspp/error.h>

#include <string>
#include <memory>
#include <vector>

namespace csspp
{

enum class node_type_t
{
    UNKNOWN,

    // basic token
    ADD,                    // for selectors: E + F, F is the next sibling of E
    AT_KEYWORD,
    CDC,
    CDO,
    CLOSE_CURLYBRACKET,
    CLOSE_PARENTHESIS,
    CLOSE_SQUAREBRACKET,
    COLON,                  // for selectors: pseudo-class, E:first-child
    COLUMN,
    COMMA,
    COMMENT,
    DASH_MATCH,             // for selectors: dash match E[land|="en"]
    DECIMAL_NUMBER,
    //DIMENSION, -- DECIMAL_NUMBER and INTEGER with a string are dimensions
    DIVIDE,
    DOLLAR,
    EOF_TOKEN,
    EQUAL,                  // for selectors: exact match E[foo="bar"]
    EXCLAMATION,
    FUNCTION,
    GREATER_THAN,           // for selectors: E > F, F is a child of E
    HASH,
    IDENTIFIER,
    INCLUDE_MATCH,          // for selectors: include match E[foo~="bar"]
    INTEGER,
    MULTIPLY,
    OPEN_CURLYBRACKET,      // holds the children of '{'
    OPEN_PARENTHESIS,       // holds the children of '('
    OPEN_SQUAREBRACKET,     // holds the children of '['
    PERCENT,
    PERIOD,                 // for selectors: E.name, equivalent to E[class~='name']
    PRECEDED,               // for selectors: E ~ F, F is a sibling after E
    PREFIX_MATCH,           // for selectors: prefix match E[foo^="bar"]
    REFERENCE,
    SCOPE,
    SEMICOLON,
    STRING,
    SUBSTRING_MATCH,        // for selectors: substring match E[foo*="bar"]
    SUBTRACT,
    SUFFIX_MATCH,           // for selectors: suffix match E[foo$="bar"]
    UNICODE_RANGE,
    URL,
    VARIABLE,
    WHITESPACE,

    // composed tokens
    CHARSET,                // @charset = @charset <string> ;
    COMPONENT_VALUE,        // "token token token ..." representing a component-value-list
    DECLARATION,            // <id> ':' ...
    FONTFACE,               // @font-face { <declaration-list> }
    KEYFRAME,               // <keyframe-selector> { <declaration-list> }
    KEYFRAMES,              // @keyframes <keyframes-name> { <rule-list> }
    LIST,                   // bare "token token token ..." until better qualified
    MEDIA,                  // @media <media-query-list> { <stylesheet> }

    max_type
};

class node
{
public:
    typedef std::shared_ptr<node>   pointer_t;
    typedef std::vector<pointer_t>  list_t;

                        node(node_type_t const type, position const & pos);

    node_type_t         get_type() const;
    bool                is(node_type_t const type) const;

    position const &    get_position() const;
    std::string const & get_string() const;
    void                set_string(std::string const & str);
    integer_t           get_integer() const;
    void                set_integer(integer_t integer);
    decimal_number_t    get_decimal_number() const;
    void                set_decimal_number(decimal_number_t decimal_number);

    bool                empty() const;
    size_t              size() const;
    void                add_child(pointer_t child);
    void                remove_child(pointer_t child);
    void                remove_child(size_t idx);
    pointer_t           get_child(size_t idx) const;
    pointer_t           get_last_child() const;
    void                take_over_children_of(pointer_t n);

    void                display(std::ostream & out, uint32_t indent) const;

private:
    node_type_t         f_type = node_type_t::UNKNOWN;
    position            f_position;
    integer_t           f_integer = 0;
    decimal_number_t    f_decimal_number = 0.0;
    std::string         f_string;
    list_t              f_children;
};


} // namespace csspp

std::ostream & operator << (std::ostream & out, csspp::node_type_t const type);
std::ostream & operator << (std::ostream & out, csspp::node const & n);

csspp::error & operator << (csspp::error & out, csspp::node_type_t const type);

#endif
// #ifndef CSSPP_NODE_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
