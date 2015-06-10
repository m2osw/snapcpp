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

#include <algorithm>
#include <iostream>
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
    case node_type_t::DECLARATION:
    case node_type_t::EXCLAMATION:
    case node_type_t::FUNCTION:
    case node_type_t::HASH:
    case node_type_t::IDENTIFIER:
    case node_type_t::INTEGER:
    case node_type_t::STRING:
    case node_type_t::URL:
    case node_type_t::VARIABLE:
        break;

    default:
        {
            std::stringstream ss;
            ss << "trying to access (read/write) the string of a node of type " << type << ", which does not support strings.";
            throw csspp_exception_logic(ss.str());
        }

    }
}

void type_supports_children(node_type_t const type)
{
    switch(type)
    {
    case node_type_t::AT_KEYWORD:
    case node_type_t::COMPONENT_VALUE:
    case node_type_t::DECLARATION:
    case node_type_t::FUNCTION:
    case node_type_t::LIST:
    case node_type_t::OPEN_CURLYBRACKET:
    case node_type_t::OPEN_PARENTHESIS:
    case node_type_t::OPEN_SQUAREBRACKET:
        break;

    default:
        {
            std::stringstream ss;
            ss << "trying to access (read/write) the children of a node of type " << type << ", which does not support children.";
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

bool node::empty() const
{
    type_supports_children(f_type);

    return f_children.empty();
}

size_t node::size() const
{
    type_supports_children(f_type);

    return f_children.size();
}

void node::add_child(pointer_t child)
{
    type_supports_children(f_type);

    // make sure we totally ignore EOF in a child list
    // (this dramatically ease the coding of the parser)
    // also we do not need to save the WHITESPACE tokens
    if(!child->is(node_type_t::EOF_TOKEN))
    {
        f_children.push_back(child);
    }
}

void node::remove_child(pointer_t child)
{
    type_supports_children(f_type);

    auto it(std::find(f_children.begin(), f_children.end(), child));
    if(it == f_children.end())
    {
        throw csspp_exception_logic("remove_child() called with a node which is not a child of this node.");
    }

    f_children.erase(it);
}

void node::remove_child(size_t idx)
{
    type_supports_children(f_type);

    if(idx >= f_children.size())
    {
        throw csspp_exception_overflow("remove_child() called with an index out of range.");
    }

    f_children.erase(f_children.begin() + idx);
}

node::pointer_t node::get_child(size_t idx) const
{
    type_supports_children(f_type);

    if(idx >= f_children.size())
    {
        throw csspp_exception_overflow("get_child() called with an index out of range.");
    }

    return f_children[idx];
}

node::pointer_t node::get_last_child() const
{
    // if empty, get_child() will throw
    return get_child(f_children.size() - 1);
}

void node::take_over_children_of(pointer_t n)
{
    type_supports_children(f_type);
    type_supports_children(n->f_type);

    // children are copied to this node and cleared
    // in the other node (TBD: should this node have
    // an empty list of children to start with?)
    f_children.clear();
    std::swap(f_children, n->f_children);
}

void node::set_variable(std::string const & name, pointer_t value)
{
    f_variables[name] = value;
}

node::pointer_t node::get_variable(std::string const & name)
{
    auto const it(f_variables.find(name));
    if(it == f_variables.end())
    {
        return pointer_t();
    }
    return it->second;
}

void node::display(std::ostream & out, uint32_t indent) const
{
    for(uint32_t i(0); i < indent; ++i)
    {
        out << " ";
    }
    out << f_type;

    switch(f_type)
    {
    case node_type_t::AT_KEYWORD:
    case node_type_t::COMMENT:
    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::DECLARATION:
    case node_type_t::EXCLAMATION:
    case node_type_t::FUNCTION:
    case node_type_t::HASH:
    case node_type_t::IDENTIFIER:
    case node_type_t::INTEGER:
    case node_type_t::STRING:
    case node_type_t::URL:
    case node_type_t::VARIABLE:
        out << " \"" << f_string << "\"";
        break;

    default:
        break;

    }

    switch(f_type)
    {
    case node_type_t::COMMENT:
    case node_type_t::INTEGER:
    case node_type_t::UNICODE_RANGE:
        out << " I:" << f_integer;
        break;

    default:
        break;

    }

    switch(f_type)
    {
    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::PERCENT:
        out << " D:" << f_decimal_number;
        break;

    default:
        break;

    }

    out << "\n";

    switch(f_type)
    {
    case node_type_t::AT_KEYWORD:
    case node_type_t::COMPONENT_VALUE:
    case node_type_t::DECLARATION:
    case node_type_t::EXCLAMATION:
    case node_type_t::FUNCTION:
    case node_type_t::LIST:
    case node_type_t::OPEN_SQUAREBRACKET:
    case node_type_t::OPEN_CURLYBRACKET:
    case node_type_t::OPEN_PARENTHESIS:
        // display the children now
        for(size_t i(0); i < f_children.size(); ++i)
        {
            f_children[i]->display(out, indent + 2);
        }
        break;

    default:
        break;

    }
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

    case csspp::node_type_t::PRECEDED:
        out << "PRECEDED";
        break;

    case csspp::node_type_t::PREFIX_MATCH:
        out << "PREFIX_MATCH";
        break;

    case csspp::node_type_t::REFERENCE:
        out << "REFERENCE";
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

    case csspp::node_type_t::VARIABLE:
        out << "VARIABLE";
        break;

    case csspp::node_type_t::WHITESPACE:
        out << "WHITESPACE";
        break;

    // Grammar related nodes (i.e. composed nodes)
    case csspp::node_type_t::CHARSET:
        out << "CHARSET";
        break;

    case csspp::node_type_t::COMPONENT_VALUE:
        out << "COMPONENT_VALUE";
        break;

    case csspp::node_type_t::DECLARATION:
        out << "DECLARATION";
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

    case csspp::node_type_t::LIST:
        out << "LIST";
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

std::ostream & operator << (std::ostream & out, csspp::node const & n)
{
    n.display(out, 0);
    return out;
}

csspp::error & operator << (csspp::error & out, csspp::node_type_t const type)
{
    std::stringstream ss;
    ss << type;
    out << ss.str();
    return out;
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
