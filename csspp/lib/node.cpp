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

#include <csspp/node.h>

#include <csspp/exceptions.h>

#include <sstream>

namespace csspp
{

namespace
{

void type_supports_integer(node_type_t const type)
{
    switch(type)
    {
    case node_type_t::COMMENT:
    case node_type_t::INTEGER:
    case node_type_t::UNICODE_RANGE:
        break;

    default:
        {
            std::stringstream ss;
            ss << "trying to access (read/write) the integer of a node of type " << type << ", which does not support integers.";
            throw csspp_exception_logic(ss.str());
        }

    }
}

void type_supports_decimal_number(node_type_t const type)
{
    switch(type)
    {
    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::PERCENT:
        break;

    default:
        {
            std::stringstream ss;
            ss << "trying to access (read/write) the decimal number of a node of type " << type << ", which does not support decimal numbers.";
            throw csspp_exception_logic(ss.str());
        }

    }
}

void type_supports_string(node_type_t const type)
{
    switch(type)
    {
    case node_type_t::AT_KEYWORD:
    case node_type_t::COMMENT:
    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::HASH:
    case node_type_t::FUNCTION:
    case node_type_t::IDENTIFIER:
    case node_type_t::INTEGER:
    case node_type_t::STRING:
    case node_type_t::URL:
        break;

    default:
        {
            std::stringstream ss;
            ss << "trying to access (read/write) the string of a node of type " << type << ", which does not support strings.";
            throw csspp_exception_logic(ss.str());
        }

    }
}

} // no name namespace

node::node(node_type_t const type, position const & pos)
    : f_type(type)
    , f_position(pos)
{
}

node_type_t node::get_type() const
{
    return f_type;
}

bool node::is(node_type_t const type) const
{
    return f_type == type;
}

position const & node::get_position() const
{
    return f_position;
}

std::string const & node::get_string() const
{
    type_supports_string(f_type);
    return f_string;
}

void node::set_string(std::string const & str)
{
    type_supports_string(f_type);
    f_string = str;
}

integer_t node::get_integer() const
{
    type_supports_integer(f_type);
    return f_integer;
}

void node::set_integer(integer_t integer)
{
    type_supports_integer(f_type);
    f_integer = integer;
}

decimal_number_t node::get_decimal_number() const
{
    type_supports_decimal_number(f_type);
    return f_decimal_number;
}

void node::set_decimal_number(decimal_number_t decimal_number)
{
    type_supports_decimal_number(f_type);
    f_decimal_number = decimal_number;
}

} // namespace csspp

std::ostream & operator << (std::ostream & out, csspp::node_type_t const type)
{
    switch(type)
    {
    case csspp::node_type_t::UNKNOWN:
        out << "UNKNOWN";
        break;

    case csspp::node_type_t::ADD:
        out << "ADD";
        break;

    case csspp::node_type_t::AT_KEYWORD:
        out << "AT_KEYWORD";
        break;

    case csspp::node_type_t::CDC:
        out << "CDC";
        break;

    case csspp::node_type_t::CDO:
        out << "CDO";
        break;

    case csspp::node_type_t::CLOSE_CURLYBRACKET:
        out << "CLOSE_CURLYBRACKET";
        break;

    case csspp::node_type_t::CLOSE_PARENTHESIS:
        out << "CLOSE_PARENTHESIS";
        break;

    case csspp::node_type_t::CLOSE_SQUAREBRACKET:
        out << "CLOSE_SQUAREBRACKET";
        break;

    case csspp::node_type_t::COLON:
        out << "COLON";
        break;

    case csspp::node_type_t::COLUMN:
        out << "COLUMN";
        break;

    case csspp::node_type_t::COMMA:
        out << "COMMA";
        break;

    case csspp::node_type_t::COMMENT:
        out << "COMMENT";
        break;

    case csspp::node_type_t::DASH_MATCH:
        out << "DASH_MATCH";
        break;

    case csspp::node_type_t::DECIMAL_NUMBER:
        out << "DECIMAL_NUMBER";
        break;

    case csspp::node_type_t::DIVIDE:
        out << "DIVIDE";
        break;

    case csspp::node_type_t::DOLLAR:
        out << "DOLLAR";
        break;

    case csspp::node_type_t::EOF_TOKEN:
        out << "EOF_TOKEN";
        break;

    case csspp::node_type_t::EQUAL:
        out << "EQUAL";
        break;

    case csspp::node_type_t::EXCLAMATION:
        out << "EXCLAMATION";
        break;

    case csspp::node_type_t::FUNCTION:
        out << "FUNCTION";
        break;

    case csspp::node_type_t::GREATER_THAN:
        out << "GREATER_THAN";
        break;

    case csspp::node_type_t::HASH:
        out << "HASH";
        break;

    case csspp::node_type_t::IDENTIFIER:
        out << "IDENTIFIER";
        break;

    case csspp::node_type_t::INCLUDE_MATCH:
        out << "INCLUDE_MATCH";
        break;

    case csspp::node_type_t::INTEGER:
        out << "INTEGER";
        break;

    case csspp::node_type_t::MULTIPLY:
        out << "MULTIPLY";
        break;

    case csspp::node_type_t::OPEN_CURLYBRACKET:
        out << "OPEN_CURLYBRACKET";
        break;

    case csspp::node_type_t::OPEN_PARENTHESIS:
        out << "OPEN_PARENTHESIS";
        break;

    case csspp::node_type_t::OPEN_SQUAREBRACKET:
        out << "OPEN_SQUAREBRACKET";
        break;

    case csspp::node_type_t::PERCENT:
        out << "PERCENT";
        break;

    case csspp::node_type_t::PERIOD:
        out << "PERIOD";
        break;

    case csspp::node_type_t::PREFIX_MATCH:
        out << "PREFIX_MATCH";
        break;

    case csspp::node_type_t::SCOPE:
        out << "SCOPE";
        break;

    case csspp::node_type_t::SEMICOLON:
        out << "SEMICOLON";
        break;

    case csspp::node_type_t::STRING:
        out << "STRING";
        break;

    case csspp::node_type_t::SUBSTRING_MATCH:
        out << "SUBSTRING_MATCH";
        break;

    case csspp::node_type_t::SUBTRACT:
        out << "SUBTRACT";
        break;

    case csspp::node_type_t::SUFFIX_MATCH:
        out << "SUFFIX_MATCH";
        break;

    case csspp::node_type_t::UNICODE_RANGE:
        out << "UNICODE_RANGE";
        break;

    case csspp::node_type_t::URL:
        out << "URL";
        break;

    case csspp::node_type_t::WHITESPACE:
        out << "WHITESPACE";
        break;

    case csspp::node_type_t::CHARSET:
        out << "CHARSET";
        break;

    case csspp::node_type_t::FONTFACE:
        out << "FONTFACE";
        break;

    case csspp::node_type_t::KEYFRAME:
        out << "KEYFRAME";
        break;

    case csspp::node_type_t::KEYFRAMES:
        out << "KEYFRAMES";
        break;

    case csspp::node_type_t::MEDIA:
        out << "MEDIA";
        break;

    case csspp::node_type_t::max_type:
        out << "max_type";
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
