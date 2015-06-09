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

/** \file
 * \brief Implementation of the CSS Preprocessor parser.
 *
 * The CSS Preprocessor parser follows the CSS 3 grammar which allows for
 * the syntax we seek to support: a syntax similar to SASS which allows
 * for selectors, blocks with fields, and embedded blocks.
 *
 * For example, we can write
 *
 * \code
 *      div {
 *          color: #000;
 *
 *          a {
 *              color: #00f;
 *          }
 *      }
 * \endcode
 *
 * and the CSS Preprocessor transforms that data in:
 *
 * \code
 * 	div{color:#000}
 * 	div a{color:#00f}
 * \endcode
 *
 * \sa \ref parser_rules
 */

#include "csspp/parser.h"

#include "csspp/error.h"
#include "csspp/exceptions.h"

#include <iostream>

namespace csspp
{

parser::parser(lexer::pointer_t l)
    : f_lexer(l)
{
}

node::pointer_t parser::stylesheet()
{
    return stylesheet(next_token());
}

node::pointer_t parser::rule_list()
{
    return rule_list(next_token());
}

node::pointer_t parser::rule()
{
    return rule(next_token());
}

node::pointer_t parser::declaration_list()
{
    return declaration_list(next_token());
}

node::pointer_t parser::component_value_list()
{
    return component_value_list(next_token());
}

node::pointer_t parser::component_value()
{
    return component_value(next_token());
}

node::pointer_t parser::next_token()
{
    f_last_token = f_lexer->next_token();
    return f_last_token;
}

node::pointer_t parser::stylesheet(node::pointer_t n)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    for(; !n->is(node_type_t::EOF_TOKEN); n = next_token())
    {
        // completely ignore the CDO and CDC, if the "assembler"
        // wants to output them, it will do so, but otherwise it
        // is just completely ignored
        //
        // also white spaces at this level are pretty much useless
        //
        if(n->is(node_type_t::CDO)
        || n->is(node_type_t::CDC)
        || n->is(node_type_t::WHITESPACE))
        {
            continue;
        }
        if(n->is(node_type_t::AT_KEYWORD))
        {
            result->add_child(at_rule(n));
        }
        else
        {
            // anything else is a qualified rule
            result->add_child(qualified_rule(n));
        }
    }

    return result;
}

node::pointer_t parser::rule_list(node::pointer_t n)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    for(; !n->is(node_type_t::EOF_TOKEN); n = next_token())
    {
        result->add_child(rule(n));
    }

    return result;
}

node::pointer_t parser::rule(node::pointer_t n)
{
    if(n->is(node_type_t::CDO)
    || n->is(node_type_t::CDC))
    {
        error::instance() << n->get_position()
                          << "HTML comment delimiters (<!-- and -->) are not allowed in this CSS document."
                          << error_mode_t::ERROR_ERROR;
        return node::pointer_t(new node(node_type_t::EOF_TOKEN, n->get_position()));
    }

    if(n->is(node_type_t::WHITESPACE))
    {
        return node::pointer_t(new node(node_type_t::EOF_TOKEN, n->get_position()));
    }

    if(n->is(node_type_t::AT_KEYWORD))
    {
        return at_rule(n);
    }

    // anything else is a qualified rule
    return qualified_rule(n);
}

node::pointer_t parser::at_rule(node::pointer_t at_keyword)
{
    // the '@' was already eaten, it will be our result
    node::pointer_t n(component_value_list(at_keyword));

    if(n->empty())
    {
        error::instance() << at_keyword->get_position()
                          << "At '@' command cannot be empty (missing block) unless ended by a semicolon (;)."
                          << error_mode_t::ERROR_ERROR;
    }
    else
    {
        node::pointer_t last_child(n->get_last_child());
        if(!last_child->is(node_type_t::OPEN_CURLYBRACKET)
        && !last_child->is(node_type_t::SEMICOLON))
        {
            error::instance() << at_keyword->get_position()
                              << "At '@' command must end with a block or a ';'."
                              << error_mode_t::ERROR_ERROR;
        }
        at_keyword->take_over_children_of(n);
    }

    return at_keyword;
}

node::pointer_t parser::qualified_rule(node::pointer_t n)
{
    // a qualified rule is a component value list that
    // ends with a block
    node::pointer_t result(component_value_list(n));

    if(result->empty())
    {
        error::instance() << n->get_position()
                          << "A qualified rule cannot be empty; you are missing a { ... } block."
                          << error_mode_t::ERROR_ERROR;
    }
    else
    {
        node::pointer_t last_child(result->get_last_child());
        if(!last_child->is(node_type_t::OPEN_CURLYBRACKET))
        {
            error::instance() << n->get_position()
                              << "A qualified rule must end with a { ... } block."
                              << error_mode_t::ERROR_ERROR;
        }
    }

    return result;
}

node::pointer_t parser::declaration_list(node::pointer_t n)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    for(;;)
    {
        if(n->is(node_type_t::WHITESPACE))
        {
            n = next_token();
        }

        if(n->is(node_type_t::IDENTIFIER))
        {
            result->add_child(declaration(n));
            if(!f_last_token->is(node_type_t::SEMICOLON))
            {
                break;
            }
            // skip the ';'
            n = next_token();
        }
        else if(n->is(node_type_t::AT_KEYWORD))
        {
            result->add_child(at_rule(n));
            n = f_last_token;
        }
        else
        {
            break;
        }
    }

    return result;
}

node::pointer_t parser::declaration(node::pointer_t identifier)
{
    node::pointer_t result(new node(node_type_t::DECLARATION, identifier->get_position()));
    result->set_string(identifier->get_string());

    node::pointer_t n(next_token());

    // allow white spaces
    if(n->is(node_type_t::WHITESPACE))
    {
        n = next_token();
    }

    // here we must have a ':'
    if(n->is(node_type_t::COLON))
    {
        // skip the colon, no need to keep it around
        n = next_token();
    }
    else
    {
        error::instance() << f_last_token->get_position()
                          << "':' missing in your declaration starting with \""
                          << identifier->get_string()
                          << "\"."
                          << error_mode_t::ERROR_ERROR;
    }

    if(!n->is(node_type_t::EXCLAMATION))
    {
        // a component value
        result->add_child(component_value(n));
        n = f_last_token;
    }

    if(n->is(node_type_t::EXCLAMATION))
    {
        node::pointer_t exclamation(next_token());
        if(exclamation->is(node_type_t::WHITESPACE))
        {
            exclamation = next_token();
        }
        if(exclamation->is(node_type_t::IDENTIFIER))
        {
            n->set_string(exclamation->get_string());
            result->add_child(n);

            // TBD: should we check that the identifier is either
            //      "important" or "global" at this point?

            // read the next token and if it is a space, skip it
            n = next_token();
            if(n->is(node_type_t::WHITESPACE))
            {
                next_token();
            }
        }
        else
        {
            error::instance() << f_last_token->get_position()
                              << "A '!' must be followed by an identifier, got a "
                              << exclamation->get_type()
                              << " instead."
                              << error_mode_t::ERROR_ERROR;
        }
    }

    return result;
}

node::pointer_t parser::component_value_list(node::pointer_t n)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    for(;; n = next_token())
    {
        // this test is rather ugly... also it kinda breaks the
        // so called 'preserved tokens'
        //
        if(n->is(node_type_t::EOF_TOKEN)
        || n->is(node_type_t::CLOSE_PARENTHESIS)
        || n->is(node_type_t::CLOSE_SQUAREBRACKET)
        || n->is(node_type_t::CLOSE_CURLYBRACKET)
        || n->is(node_type_t::AT_KEYWORD)
        || n->is(node_type_t::EXCLAMATION)
        || n->is(node_type_t::SEMICOLON)
        || n->is(node_type_t::CDO)
        || n->is(node_type_t::CDC))
        {
            break;
        }
        result->add_child(component_value(n));
    }

    return result;
}

node::pointer_t parser::component_value(node::pointer_t n)
{
    if(n->is(node_type_t::OPEN_CURLYBRACKET))
    {
        // parse a block up to '}'
        return block(n, node_type_t::CLOSE_CURLYBRACKET);
    }

    if(n->is(node_type_t::OPEN_SQUAREBRACKET))
    {
        // parse a block up to ']'
        return block(n, node_type_t::CLOSE_SQUAREBRACKET);
    }

    if(n->is(node_type_t::OPEN_PARENTHESIS))
    {
        // parse a block up to ')'
        return block(n, node_type_t::CLOSE_PARENTHESIS);
    }

    if(n->is(node_type_t::FUNCTION))
    {
        // parse a block up to ')'
        return block(n, node_type_t::CLOSE_PARENTHESIS);
    }

    // n is the token we keep
    return n;
}

node::pointer_t parser::block(node::pointer_t b, node_type_t closing_token)
{
    b->add_child(component_value_list(next_token()));
    if(f_last_token->is(closing_token))
    {
        // skip that closing token
        next_token();
    }
    else
    {
        error::instance() << b->get_position()
                          << "Block expected to end with "
                          << closing_token
                          << " but got "
                          << f_last_token->get_type()
                          << " instead."
                          << error_mode_t::ERROR_ERROR;
    }

    return b;
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
