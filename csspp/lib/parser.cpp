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

namespace
{

int const component_value_flag_return_on_semi_colon  = 0x0001;
int const component_value_flag_return_on_exclamation = 0x0002;

} // no name namespace

parser::parser(lexer::pointer_t l)
    : f_lexer(l)
{
    next_token();
}

node::pointer_t parser::stylesheet()
{
    return stylesheet(f_last_token);
}

node::pointer_t parser::rule_list()
{
    return rule_list(f_last_token);
}

node::pointer_t parser::rule()
{
    return rule(f_last_token);
}

node::pointer_t parser::declaration_list()
{
    return declaration_list(f_last_token);
}

node::pointer_t parser::component_value_list()
{
    return component_value_list(f_last_token, component_value_flag_return_on_semi_colon);
}

node::pointer_t parser::component_value()
{
    return component_value(f_last_token);
}

node::pointer_t parser::next_token()
{
    f_last_token = f_lexer->next_token();
//std::cerr << "*** TOKEN: " << *f_last_token;
    return f_last_token;
}

node::pointer_t parser::stylesheet(node::pointer_t n)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    for(; !n->is(node_type_t::EOF_TOKEN); n = f_last_token)
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
            next_token();
            continue;
        }

        if(n->is(node_type_t::CLOSE_CURLYBRACKET)
        || n->is(node_type_t::CLOSE_SQUAREBRACKET)
        || n->is(node_type_t::CLOSE_PARENTHESIS))
        {
            error::instance() << n->get_position()
                              << "Unexpected closing block of type: " << n->get_type() << "."
                              << error_mode_t::ERROR_ERROR;
            break;
        }

        if(n->is(node_type_t::COMMENT))
        {
            result->add_child(n);
            next_token();
        }
        else if(n->is(node_type_t::AT_KEYWORD))
        {
            result->add_child(at_rule(n));
        }
        else
        {
            // anything else is a qualified rule
            result->add_child(qualified_rule(n));
        }
    }

    // we always return the LIST because it starts with @import (or rather
    // is just one @import) or $var then it needs to be replaced and we
    // could not do that if those were root nodes
    return result;
}

node::pointer_t parser::rule_list(node::pointer_t n)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    for(node::pointer_t q; (!q || !q->is(node_type_t::EOF_TOKEN)) && !n->is(node_type_t::EOF_TOKEN); n = f_last_token)
    {
        q = rule(n);
        result->add_child(q);
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

    if(n->is(node_type_t::CLOSE_CURLYBRACKET)
    || n->is(node_type_t::CLOSE_SQUAREBRACKET)
    || n->is(node_type_t::CLOSE_PARENTHESIS))
    {
        error::instance() << n->get_position()
                          << "Unexpected closing block of type: " << n->get_type() << "."
                          << error_mode_t::ERROR_ERROR;
        return node::pointer_t(new node(node_type_t::EOF_TOKEN, n->get_position()));
    }

    if(n->is(node_type_t::WHITESPACE))
    {
        // skip potential whitespaces
        n = next_token();
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
    node::pointer_t n(component_value_list(next_token(), component_value_flag_return_on_semi_colon));

    if(n->empty())
    {
        error::instance() << at_keyword->get_position()
                          << "At '@' command cannot be empty (missing block) unless ended by a semicolon (;)."
                          << error_mode_t::ERROR_ERROR;
    }
    else
    {
        node::pointer_t last_child(n->get_last_child());
        if(f_last_token->is(node_type_t::SEMICOLON))
        {
            next_token();
        }
        else if(!last_child->is(node_type_t::OPEN_CURLYBRACKET))
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
    if(n->is(node_type_t::EOF_TOKEN))
    {
        return n;
    }
    if(n->is(node_type_t::SEMICOLON))
    {
        // skip the ';' (i.e. ';' in 'foo { blah: 123 };')
        next_token();

        // it is an error, we just make it clear what error it is because
        // by default it would otherwise come out as "invalid qualified rule"
        // which is rather hard to understand here...
        error::instance() << n->get_position()
                          << "A qualified rule cannot end a { ... } block with a ';'."
                          << error_mode_t::ERROR_ERROR;
        return node::pointer_t(new node(node_type_t::EOF_TOKEN, n->get_position()));
    }

    // a qualified rule is a component value list that
    // ends with a block
    node::pointer_t result(component_value_list(n, 0));

    if(result->empty())
    {
        // I have not been able to reach these lines, somehow...
        error::instance() << n->get_position()
                          << "A qualified rule cannot be empty; you are missing a { ... } block."
                          << error_mode_t::ERROR_ERROR;
    }
    else
    {
        node::pointer_t last_child(result->get_last_child());
        if(!is_variable_set(result, false)
        && !last_child->is(node_type_t::OPEN_CURLYBRACKET))
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
                // the EOF_TOKEN below generates an error if we
                // do not remove those spaces ahead of time
                if(f_last_token->is(node_type_t::WHITESPACE))
                {
                    next_token();
                }
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

    if(!f_last_token->is(node_type_t::EOF_TOKEN))
    {
        error::instance() << f_last_token->get_position()
                          << "the end of the stream was not reached in this declaration, we stopped on a "
                          << f_last_token->get_type()
                          << "."
                          << error_mode_t::ERROR_ERROR;
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
        error::instance() << n->get_position()
                          << "':' missing in your declaration starting with \""
                          << identifier->get_string()
                          << "\"."
                          << error_mode_t::ERROR_ERROR;
    }

    if(!n->is(node_type_t::EXCLAMATION))
    {
        // a component value
        result->add_child(component_value_list(n, component_value_flag_return_on_semi_colon | component_value_flag_return_on_exclamation));
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
            error::instance() << exclamation->get_position()
                              << "A '!' must be followed by an identifier, got a "
                              << exclamation->get_type()
                              << " instead."
                              << error_mode_t::ERROR_ERROR;
        }
    }

    return result;
}

node::pointer_t parser::component_value_list(node::pointer_t n, int flags)
{
    node::pointer_t result(new node(node_type_t::LIST, n->get_position()));

    node::pointer_t list(new node(node_type_t::COMPONENT_VALUE, n->get_position()));
    result->add_child(list);
    for(;; n = f_last_token)
    {
        // this test is rather ugly... also it kinda breaks the
        // so called 'preserved tokens'
        //
        if(n->is(node_type_t::EOF_TOKEN)
        || n->is(node_type_t::CLOSE_PARENTHESIS)
        || n->is(node_type_t::CLOSE_SQUAREBRACKET)
        || n->is(node_type_t::CLOSE_CURLYBRACKET)
        || ((flags & component_value_flag_return_on_semi_colon) && n->is(node_type_t::SEMICOLON)) // declarations handle the semi-colon differently
        || ((flags & component_value_flag_return_on_exclamation) != 0 && n->is(node_type_t::EXCLAMATION))
        || n->is(node_type_t::CDO)
        || n->is(node_type_t::CDC))
        {
            break;
        }

        if(n->is(node_type_t::AT_KEYWORD))
        {
            list->add_child(at_rule(n));
            continue;
        }

        if(n->is(node_type_t::SEMICOLON))
        {
            next_token();

            // remove leading and trailing whitespace, no need really
            while(!list->empty() && list->get_child(0)->is(node_type_t::WHITESPACE))
            {
                list->remove_child(0);
            }
            while(!list->empty() && list->get_last_child()->is(node_type_t::WHITESPACE))
            {
                list->remove_child(list->size() - 1);
            }

            // variables are viewed as a terminator string when ended by a
            // semicolon; a qualified rule normally requires a block to
            // end, but we have a special case to allow definition of
            // variables anywhere
            if(is_variable_set(list, false))
            {
                break;
            }

            if(!list->empty())
            {
                // move to a new sub-list
                list.reset(new node(node_type_t::COMPONENT_VALUE, n->get_position()));
                result->add_child(list);
            }
            continue;
        }

        // remove trailing whitespace before a block, no need
        if((n->is(node_type_t::OPEN_CURLYBRACKET)
         || n->is(node_type_t::OPEN_SQUAREBRACKET)
         || n->is(node_type_t::OPEN_PARENTHESIS))
        && !list->empty()
        && list->get_last_child()->is(node_type_t::WHITESPACE))
        {
            list->remove_child(list->size() - 1);
        }

        if(n->is(node_type_t::OPEN_CURLYBRACKET))
        {
            // in this special case, we read the {}-block and return
            // (i.e. end of an @-rule, etc.)
            //
            // however, to support the full SASS syntax we need to
            // support two special cases:
            //
            //    $var: { some-value: here; };
            //    font: { family: strange; style: italic };
            //
            // For those special entries, we must avoid returning
            // when we find a block (darn! this grammar...)
            //
            // Note that the second test is done after we read the block
            // since the presence of the block is checked in case of the
            // nested declaration.
            //
            list->add_child(component_value(n));

            // remove leading and trailing whitespace, no need really
            // (to make sure the tests below work as expected)
            if(!list->empty() && list->get_child(0)->is(node_type_t::WHITESPACE))
            {
                list->remove_child(0);
            }

            // return or that were sub-definitions?
            if(!is_variable_set(list, true)
            && !is_nested_declaration(list))
            {
                break;
            }

            while(f_last_token->is(node_type_t::WHITESPACE))
            {
                next_token();
            }

            if(!f_last_token->is(node_type_t::SEMICOLON))
            {
                // blocks defining a variable or a nested declaration
                // must be followed by a semi-colon or we have an error
                error::instance() << list->get_child(0)->get_position()
                                  << "Variable set to a block and a nested property block must end with a semicolon (;) after said block."
                                  << error_mode_t::ERROR_ERROR;
            }
        }
        else
        {
            list->add_child(component_value(n));
        }
    }

    // remove leading and trailing whitespace, no need really
    if(!list->empty() && list->get_child(0)->is(node_type_t::WHITESPACE))
    {
        list->remove_child(0);
    }
    if(!list->empty() && list->get_last_child()->is(node_type_t::WHITESPACE))
    {
        list->remove_child(list->size() - 1);
    }

    if(list->empty())
    {
        result->remove_child(list);
    }

    if(result->size() == 1)
    {
        result = result->get_last_child();
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

    if(n->is(node_type_t::OPEN_PARENTHESIS)
    || n->is(node_type_t::FUNCTION)
    || n->is(node_type_t::VARIABLE_FUNCTION))
    {
        // parse a block up to ')'
        return block(n, node_type_t::CLOSE_PARENTHESIS);
    }

    next_token();

    // n is the token we keep
    return n;
}

node::pointer_t parser::block(node::pointer_t b, node_type_t closing_token)
{
    node::pointer_t children(component_value_list(next_token(), 0));
    b->take_over_children_of(children);
    if(f_last_token->is(node_type_t::WHITESPACE))
    {
        next_token();
    }
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

bool parser::is_variable_set(node::pointer_t n, bool with_block)
{
    // a variable set is at least 3 tokens:
    //    $var:<value>
    if(n->size() < 3
    || (!n->get_child(0)->is(node_type_t::VARIABLE)
     && !n->get_child(0)->is(node_type_t::VARIABLE_FUNCTION)))
    {
        return false;
    }

    size_t pos(n->get_child(1)->is(node_type_t::WHITESPACE) ? 2 : 1);
    if(!n->get_child(pos)->is(node_type_t::COLON))
    {
        return false;
    }

    if(!with_block)
    {
        // in this case the shorthand is enough: $var ':'
        return true;
    }

    // WARNING: from here the size needs to be checked since the list may
    //          be smaller than what we are looking for in it

    // in this case we need to have: $var ':' '{'
    ++pos;
    if(pos < n->size() && n->get_child(pos)->is(node_type_t::WHITESPACE))
    {
        ++pos;
    }

    return pos < n->size() && n->get_child(pos)->is(node_type_t::OPEN_CURLYBRACKET);
}

bool parser::is_nested_declaration(node::pointer_t n)
{
    // a declaration with a sub-block
    //    field: '{' ... '}' ';'
    if(n->size() < 3
    || !n->get_child(0)->is(node_type_t::IDENTIFIER))
    {
        return false;
    }

    size_t pos(n->get_child(1)->is(node_type_t::WHITESPACE) ? 2 : 1);
    if(!n->get_child(pos)->is(node_type_t::COLON))
    {
        return false;
    }
    ++pos;
    if(n->get_child(pos)->is(node_type_t::WHITESPACE))
    {
        // although we test this special case, there isn't a way to
        // reach this line without actually building a tree of nodes
        // by hand and adding a WHITESPACE "at the wrong place" which
        // we dot not currently test (TODO)
        ++pos; // LCOV_EXCL_LINE
    }
    return n->get_child(pos)->is(node_type_t::OPEN_CURLYBRACKET);
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
