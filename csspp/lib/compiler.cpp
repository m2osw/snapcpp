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
 * \brief Implementation of the CSS Preprocessor compiler.
 *
 * The CSS Preprocessor compiler applies the script rules and transform
 * the tree of nodes so it can be output as standard CSS.
 */

#include "csspp/compiler.h"

#include "csspp/error.h"
#include "csspp/exceptions.h"
#include "csspp/parser.h"
//#include "csspp/unicode_range.h"

//#include <cmath>
#include <iostream>

namespace csspp
{

namespace
{

class safe_parents_t
{
public:
    safe_parents_t(node_vector_t & parents, node::pointer_t n)
        : f_parents(parents)
    {
        f_parents.push_back(n);
    }

    ~safe_parents_t()
    {
        f_parents.pop_back();
    }

private:
    node_vector_t &     f_parents;
};

} // no name namespace

compiler::compiler()
{
    f_paths.push_back("/usr/lib/csspp/scripts");
}

node::pointer_t compiler::get_root() const
{
    return f_root;
}

void compiler::set_root(node::pointer_t root)
{
    f_root = root;
}

void compiler::clear_paths()
{
    f_paths.clear();
}

void compiler::add_path(std::string const & path)
{
    f_paths.push_back(path);
}

void compiler::compile()
{
    compile(f_root);

    if(!f_parents.empty())
    {
        throw csspp_exception_logic("compiler.cpp: the stack of parents must always be empty before compile() returns"); // LCOV_EXCL_LINE
    }
}

void compiler::compile(node::pointer_t n)
{
    safe_parents_t safe_parents(f_parents, n);

    switch(n->get_type())
    {
    case node_type_t::LIST:
        // transparent item, just compile all the children
        {
            size_t const max_children(n->size());
            for(size_t idx(0); idx < max_children; ++idx)
            {
                compile(n->get_child(idx));
            }
            // TODO: remove LIST if it now is empty or has 1 item
        }
        break;

    case node_type_t::COMPONENT_VALUE:
        {
            compile_component_value(n);
        }
        break;

    default:
        {
            std::stringstream ss;
            ss << "unexpected token (type: " << n->get_type() << ") in compile().";
            throw csspp_exception_unexpected_token(ss.str());
        }

    }
}

void compiler::compile_component_value(node::pointer_t n)
{
    // there are 3 types of component values we understand; we test
    // them in the following order to make sure we know exactly what
    // type we are dealing with:
    //
    //   1. <selector-list> '{' ... '}'
    //   2. $variable ':' ...
    //   3. <field-name> ':' ...
    //

    if(n->empty())
    {
        // we have a problem, we should already have had an error
        // somewhere? (TBD)
        return;
    }

    // 1. <selector-list> '{' ... '}'
    if(n->get_last_child()->is(node_type_t::OPEN_CURLYBRACKET))
    {
        // this is a selector list followed by a block of
        // definitions and sub-blocks
        compile_qualified_rule(n);
        return;
    }

    // 2. <variable> ':' ...
    if(parser::is_variable_set(n))
    {
        compile_variable(n);
        return;
    }

    // 3. <field-name> ':' ...
    compile_declaration(n);
}

void compiler::compile_qualified_rule(node::pointer_t n)
{
    // so here we have a list of selectors, that means we can verify
    // that said list is valid (i.e. binary operators are used properly,
    // only valid operators were used, etc.)

    // any selectors?
    if(n->size() <= 1)
    {
        return;
    }
}

void compiler::compile_variable(node::pointer_t n)
{
    // a variable gets removed from the tree and its current value
    // saved in a parent node that is an OPEN_CURLYBRACKET or the
    // root node if no OPEN_CURLYBRACKET is found in the parents

    f_parents.back()->remove_child(n);

    std::string const & variable_name(n->get_string());

    n->remove_child(0);
    if(n->get_child(0)->is(node_type_t::WHITESPACE))
    {
        n->remove_child(0);
    }
    if(!n->get_child(0)->is(node_type_t::COLON))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable set is not exactly IDENTIFIER WHITESPACE* ':'."); // LCOV_EXCL_LINE
    }
    n->remove_child(0);

    // now the value of the variable is 'n'; it will get compiled once in
    // context (i.e. not here)

    // search the parents for the node where the variable will be set
    size_t pos(f_parents.size());
    while(pos > 0)
    {
        --pos;
        if(f_parents[pos]->is(node_type_t::OPEN_CURLYBRACKET))
        {
            break;
        }
    }

    // save the variable
    f_parents[pos]->set_variable(variable_name, n);
}

void compiler::compile_declaration(node::pointer_t n)
{
    // first make sure we have a declaration
    // (i.e. IDENTIFIER WHITESPACE ':' ...)
    //
    node::pointer_t identifier(n->get_child(0));
    if(!identifier->is(node_type_t::IDENTIFIER))
    {
        error::instance() << n->get_position()
                << "expected an identifier to start a declaration value; got a: " << n->get_type() << " instead."
                << error_mode_t::ERROR_FATAL;
        return;
    }

    // the WHITESPACE is optional, if present, remove it
    node::pointer_t next(n->get_child(1));
    if(next->is(node_type_t::WHITESPACE))
    {
        n->remove_child(1);
        next = n->get_child(1);
    }

    // now we must have a COLON, also remove that COLON
    if(!next->is(node_type_t::COLON))
    {
        error::instance() << n->get_position()
                << "expected a ':' after the identifier of this declaration value; got a: " << n->get_type() << " instead."
                << error_mode_t::ERROR_FATAL;
        return;
    }
    n->remove_child(1);

    // create a declaration to replace the identifier
    node::pointer_t declaration(new node(node_type_t::DECLARATION, n->get_position()));
    declaration->set_string(identifier->get_string());

    // copy the following children as the children of the declaration
    // (i.e. identifier is element 0, so we copy elements 1 to n)
    size_t const max_children(n->size());
    for(size_t i(1); i < max_children; ++i)
    {
        declaration->add_child(n->get_child(i));
    }

    // now replace that identifier by its declaration in the parent
    f_parents.back()->replace_child(identifier, declaration);
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
