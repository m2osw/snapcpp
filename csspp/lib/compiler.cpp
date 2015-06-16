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
 *
 * \sa \ref compiler_reference
 */

#include "csspp/compiler.h"

#include "csspp/exceptions.h"
#include "csspp/expression.h"
#include "csspp/nth_child.h"
#include "csspp/parser.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include <unistd.h>

namespace csspp
{

namespace
{

integer_t const g_if_or_else_undefined    = 0;
integer_t const g_if_or_else_false_so_far = 1;
integer_t const g_if_or_else_executed     = 2;

} // no name namespace

class safe_parents_t
{
public:
    safe_parents_t(compiler::compiler_state_t & state, node::pointer_t n)
        : f_state(state)
    {
        f_state.push_parent(n);
    }

    ~safe_parents_t()
    {
        f_state.pop_parent();
    }

private:
    compiler::compiler_state_t &     f_state;
};

class safe_compiler_state_t
{
public:
    safe_compiler_state_t(compiler::compiler_state_t & state)
        : f_state(state)
        , f_state_copy(state)
    {
    }

    ~safe_compiler_state_t()
    {
        f_state = f_state_copy;
    }

private:
    compiler::compiler_state_t &    f_state;
    compiler::compiler_state_t      f_state_copy;
};

void compiler::compiler_state_t::set_root(node::pointer_t root)
{
    f_root = root;
    f_parents.clear();
}

node::pointer_t compiler::compiler_state_t::get_root() const
{
    return f_root;
}

void compiler::compiler_state_t::push_parent(node::pointer_t parent)
{
    f_parents.push_back(parent);
}

void compiler::compiler_state_t::pop_parent()
{
    f_parents.pop_back();
}

bool compiler::compiler_state_t::empty_parents() const
{
    return f_parents.empty();
}

node::pointer_t compiler::compiler_state_t::get_current_parent() const
{
    if(f_parents.empty())
    {
        throw csspp_exception_logic("compiler.cpp:compiler::compiler_state_t::get_current_parent(): no parents available."); // LCOV_EXCL_LINE
    }

    return f_parents.back();
}

node::pointer_t compiler::compiler_state_t::get_previous_parent() const
{
    if(f_parents.size() < 2)
    {
        throw csspp_exception_logic("compiler.cpp:compiler::compiler_state_t::get_current_parent(): no previous parents available."); // LCOV_EXCL_LINE
    }

    // return the parent before last
    return f_parents[f_parents.size() - 2];
}

node::pointer_t compiler::compiler_state_t::find_parent_by_type(node_type_t type) const
{
    size_t pos(f_parents.size());
    while(pos > 0)
    {
        --pos;
        if(f_parents[pos]->is(type))
        {
            return f_parents[pos];
        }
    }

    return node::pointer_t();
}

node::pointer_t compiler::compiler_state_t::find_parent_by_type(node_type_t type, node::pointer_t starting_here) const
{
    size_t pos(f_parents.size());
    while(pos > 0)
    {
        --pos;
        if(f_parents[pos] == starting_here)
        {
            break;
        }
    }

    while(pos > 0)
    {
        --pos;
        if(f_parents[pos]->is(type))
        {
            return f_parents[pos];
        }
    }

    return node::pointer_t();
}

node::pointer_t compiler::compiler_state_t::find_selector() const
{
    node::pointer_t s(find_parent_by_type(node_type_t::OPEN_CURLYBRACKET));
    while(s)
    {
        // is this marked as a selector?
        if(s->get_boolean())
        {
            return s;
        }
        s = find_parent_by_type(node_type_t::OPEN_CURLYBRACKET, s);
    }

    // if nothing else return the root
    return f_root;
}

compiler::compiler(bool validating)
    : f_compiler_validating(validating)
{
    f_paths.push_back("/usr/lib/csspp/scripts");
}

node::pointer_t compiler::get_root() const
{
    return f_state.get_root();
}

void compiler::set_root(node::pointer_t root)
{
    f_state.set_root(root);
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
    // before we compile anything we want to transform all the variables
    // with their verbatim contents; otherwise the compiler would be way
    // more complex for nothing...
    //
    // also for the variables to work appropriately, we immediately handle
    // the @import and @mixins since both may define additional variables.
    //
//std::cerr << "************* COMPILING:\n" << *f_state.get_root() << "-----------------\n";
    mark_selectors(f_state.get_root());
    if(!f_state.empty_parents())
    {
        throw csspp_exception_logic("compiler.cpp: the stack of parents must always be empty before mark_selectors() returns."); // LCOV_EXCL_LINE
    }

    replace_variables(f_state.get_root());
    if(!f_state.empty_parents())
    {
        throw csspp_exception_logic("compiler.cpp: the stack of parents must always be empty before replace_variables() returns."); // LCOV_EXCL_LINE
    }

    compile(f_state.get_root());
    if(!f_state.empty_parents())
    {
        throw csspp_exception_logic("compiler.cpp: the stack of parents must always be empty before compile() returns"); // LCOV_EXCL_LINE
    }
}

void compiler::compile(node::pointer_t n)
{
    safe_parents_t safe_parents(f_state, n);

    switch(n->get_type())
    {
    case node_type_t::LIST:
        // transparent item, just compile all the children
        {
            size_t idx(0);
            while(idx < n->size())
            {
                node::pointer_t child(n->get_child(idx));
                compile(child);

                // the child may replace itself with something else
                // in which case we do not want the ++idx
                if(idx < n->size()
                && n->get_child(idx) == child)
                {
                    ++idx;
                }
            }
            // TODO: remove LIST if it now is empty or has 1 item
        }
        break;

    case node_type_t::COMPONENT_VALUE:
        compile_component_value(n);
        break;

    case node_type_t::AT_KEYWORD:
        compile_at_keyword(n);
        break;

    case node_type_t::COMMENT:
        // passthrough tokens
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
    // there are quite a few cases to handle here:
    //
    //   $variable ':' '{' ... '}'
    //   <field-prefix> ':' '{' ... '}'
    //   <selector-list> '{' ... '}'
    //   $variable ':' ...
    //   <field-name> ':' ...
    //

    if(n->empty())
    {
        // we have a problem, we should already have had an error
        // somewhere? (TBD)
        // I think we can get those if all there was in a component
        // value were variables
        return;
    }

    // $variable ':' '{' ... '}'
    if(parser::is_variable_set(n, true))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable definition was found while compiling (1)."); // LCOV_EXCL_LINE
    }

    // <field-prefix> ':' '{' ... '}'
    if(parser::is_nested_declaration(n))
    {
        compile_declaration(n);
        return;
    }

    // <selector-list> '{' ... '}'
    if(n->get_last_child()->is(node_type_t::OPEN_CURLYBRACKET))
    {
        // this is a selector list followed by a block of
        // definitions and sub-blocks
        compile_qualified_rule(n);
        return;
    }

    // $variable ':' ... ';'
    if(parser::is_variable_set(n, false))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable definition was found while compiling (1)."); // LCOV_EXCL_LINE
    }

    // <field-name> ':' ...
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
        error::instance() << n->get_position()
                << "a qualified rule without selectors is not valid."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    // compile the selectors using a node parser
    // \ref selectors_rules#grammar
    if(!parse_selector(n))
    {
        // an error occurred, forget this entry and move on
        return;
    }

    // compile the block contents
    node::pointer_t brackets(n->get_last_child());
    if(!brackets->empty()
    && brackets->get_child(0)->is(node_type_t::COMPONENT_VALUE))
    {
        safe_parents_t safe_parents(f_state, brackets);
        size_t max_children(brackets->size());
        for(size_t idx(0); idx < max_children; ++idx)
        {
            safe_parents_t safe_sub_parents(f_state, brackets->get_child(idx));
            compile_component_value(brackets->get_child(idx));
        }
    }
    else
    {
        // only one value, this is a component value by itself
        safe_parents_t safe_parents(f_state, brackets);
        compile_component_value(brackets);
    }
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
                << error_mode_t::ERROR_ERROR;
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
                << error_mode_t::ERROR_ERROR;
        return;
    }
    n->remove_child(1);

    // no need to keep the next whitespace if there is one,
    // plus we often do not expect such at the start of a
    // list like we are about to generate.
    if(n->get_child(1)->is(node_type_t::WHITESPACE))
    {
        n->remove_child(1);
    }

    // create a declaration to replace the identifier
    node::pointer_t declaration(new node(node_type_t::DECLARATION, n->get_position()));
    declaration->set_string(identifier->get_string());

    // copy the following children as the children of the declaration
    // (i.e. identifier is element 0, so we copy elements 1 to n)
    size_t const max_children(n->size());
    for(size_t i(1); i < max_children; ++i)
    {
        // since we are removing the children, we always seemingly
        // copy child 1...
        declaration->add_child(n->get_child(1));
        n->remove_child(1);
    }

    // now replace that identifier by its declaration in the parent
    if(n->is(node_type_t::COMPONENT_VALUE))
    {
        // replace the COMPONENT_VALUE instead of the identifier
        // (this happens when a component value has multiple entries)
        f_state.get_previous_parent()->replace_child(n, declaration);
    }
    else
    {
        n->replace_child(identifier, declaration);
    }
}

void compiler::compile_at_keyword(node::pointer_t n)
{
    std::string const at(n->get_string());

    node::pointer_t parent(f_state.get_previous_parent());
    node::pointer_t expr(!n->empty() && !n->get_child(0)->is(node_type_t::OPEN_CURLYBRACKET) ? n->get_child(0) : node::pointer_t());

    if(at == "error")
    {
        parent->remove_child(n);

        error::instance() << n->get_position()
                << (expr ? expr->to_string(0) : std::string("@error reached"))
                << error_mode_t::ERROR_ERROR;
        return;
    }

    if(at == "warning")
    {
        parent->remove_child(n);

        error::instance() << n->get_position()
                << (expr ? expr->to_string(0) : std::string("@warning reached"))
                << error_mode_t::ERROR_WARNING;
        return;
    }

    if(at == "info"
    || at == "message")
    {
        parent->remove_child(n);

        error::instance() << n->get_position()
                << (expr ? expr->to_string(0) : std::string("@message reached"))
                << error_mode_t::ERROR_INFO;
        return;
    }

    if(at == "debug")
    {
        parent->remove_child(n);

        error::instance() << n->get_position()
                << (expr ? expr->to_string(0) : std::string("@debug reached"))
                << error_mode_t::ERROR_DEBUG;
        return;
    }

}

node::pointer_t compiler::compile_expression(node::pointer_t n, bool skip_whitespace, bool list_of_expressions)
{
    // expression: conditional
    expression expr(n, skip_whitespace);
    expr.mark_start();
    expr.next();
    node::pointer_t result;
    if(list_of_expressions)
    {
        // result is a list: a, b, c, ...
        result = expr.conditional();
    }
    else
    {
        result = expr.conditional();
    }
    if(result)
    {
        expr.replace_with_result(result);
    }
    return result;
}

void compiler::replace_import(node::pointer_t parent, node::pointer_t import, node::pointer_t expr, size_t & idx)
{
    static_cast<void>(import);

    //
    // WARNING: we do NOT support the SASS extension of multiple entries
    //          within one @import because it is not CSS 2 or CSS 3
    //          compatible
    //

    // node 'import' is the @import itself
    //
    //   @import string | url() [ media-list ] ';'
    //

    // we only support arguments with one string
    // (@import accepts strings and url() as their first parameter)
    //
    if(expr && expr->size() == 1
    && expr->is(node_type_t::STRING))
    {
        std::string const script_name(expr->get_string());

        // TODO: add code to avoid testing with filenames that represent URIs

        // search the corresponding file
        std::string filename(find_file(script_name));
        if(filename.empty())
        {
            if(script_name.substr(script_name.size() - 5) != ".scss")
            {
                // try again with the "scss" extension
                filename = find_file(script_name + ".scss");
            }
        }

        // if still not found, we ignore
        if(!filename.empty())
        {
            // found an SCSS include, we remove that @import and replace
            // it (see below) with data as loaded from that file
            //
            // idx will not be incremented as a result
            //
            parent->remove_child(idx);

            // position object for this file
            position pos(filename);

            // TODO: do the necessary to avoid recursive @import

            // we found a file, load it and return it
            std::ifstream in;
            in.open(filename);
            if(!in)
            {
                // the script may not really allow reading even though
                // access() just told us otherwise
                error::instance() << pos
                        << "validation script \""
                        << script_name
                        << "\" could not be opened."
                        << error_mode_t::ERROR_ERROR;
            }
            else
            {
                // the file got loaded, parse it and return the root node
                error_happened_t old_count;

                lexer::pointer_t l(new lexer(in, pos));
                parser p(l);
                node::pointer_t list(p.stylesheet());

                if(!old_count.error_happened())
                {
                    // copy valid results at 'idx' which will then be
                    // checked as if it had been part of that script
                    // all along
                    //
                    size_t const max_results(list->size());
                    for(size_t i(0), j(idx); i < max_results; ++i, ++j)
                    {
                        parent->insert_child(j, list->get_child(i));
                    }
                }
            }

            // in this case we managed the entry fully
            return;
        }
    }

    ++idx;
}

void compiler::handle_mixin(node::pointer_t n)
{
    if(n->size() != 2)
    {
        error::instance() << n->get_position()
                << "a @mixin definition expects exactly two parameters: an identifier or function and a {}-block."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    node::pointer_t block(n->get_child(1));
    if(block->is(node_type_t::OPEN_CURLYBRACKET))
    {
        error::instance() << n->get_position()
                << "a @mixin definition expects a {}-block as its second parameter."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    node::pointer_t name(n->get_child(0));
    if(name->is(node_type_t::IDENTIFIER))
    {
        // this is just like a variable
        //
        // search the parents for the node where the variable will be set
        node::pointer_t var_holder(f_state.find_selector());

        // save the variable
        var_holder->set_variable(name->get_string(), block);
    }
    else if(name->is(node_type_t::FUNCTION))
    {
        // this is a function declaration, it includes a list of arguments
        // which we want to check, and if valid we save it in the root node
        argify(name);

        size_t const max_children(name->size());
        for(size_t idx(0); idx < max_children; ++idx)
        {
            node::pointer_t arg(name->get_child(idx));
            if(!arg->is(node_type_t::ARG))
            {
                error::instance() << n->get_position()
                        << "a @mixin list of arguments is expected to be only ARG objects."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
            if(arg->size() != 1)
            {
                error::instance() << n->get_position()
                        << "a @mixin list of arguments is expected to be composed of exactly one identifier per argument."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
            node::pointer_t a(arg->get_child(0));
            if(!a->is(node_type_t::IDENTIFIER))
            {
                error::instance() << n->get_position()
                        << "a @mixin list of arguments is expected to be composed of identifiers only."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
            std::string const arg_name(a->get_string());
            if(arg_name.length() > 3
            && arg_name.substr(arg_name.length() - 3) == "..."
            && idx + 1 != max_children)
            {
                error::instance() << n->get_position()
                        << "only the last identifier of a @mixin list of arguments may end with '...'."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
        }
    }
    else
    {
        error::instance() << n->get_position()
                << "a @mixin expects either an identifier or a function as its first parameter."
                << error_mode_t::ERROR_ERROR;
    }
}

void compiler::mark_selectors(node::pointer_t n)
{
    safe_parents_t safe_parents(f_state, n);

    switch(n->get_type())
    {
    case node_type_t::AT_KEYWORD:
    //case node_type_t::ARG:
    case node_type_t::COMPONENT_VALUE:
    case node_type_t::DECLARATION:
    case node_type_t::LIST:
    case node_type_t::OPEN_CURLYBRACKET:
        {
            // there are the few cases we can have here:
            //
            //   $variable ':' '{' ... '}'
            //   <field-prefix> ':' '{' ... '}'
            //   <selector-list> '{' ... '}' <-- this is the one we're interested in
            //   $variable ':' ...
            //   <field-name> ':' ...
            //

            if(!n->empty()
            && !parser::is_variable_set(n, true)                        // $variable ':' '{' ... '}'
            && !parser::is_nested_declaration(n)                        // <field-prefix> ':' '{' ... '}'
            && n->get_last_child()->is(node_type_t::OPEN_CURLYBRACKET)) // <selector-list> '{' ... '}'
            {
                // this is a selector list followed by a block of
                // definitions and sub-blocks
                n->get_last_child()->set_boolean(true); // accept variables
            }

            // replace all $<var> references with the corresponding value
            for(size_t idx(0); idx < n->size(); ++idx)
            {
                // recursive call to handle all children in the
                // entire tree
                mark_selectors(n->get_child(idx));
            }
        }
        break;

    default:
        // other nodes are not of interest here
        break;

    }
}

void compiler::replace_variables(node::pointer_t n)
{
    safe_parents_t safe_parents(f_state, n);

    switch(n->get_type())
    {
    case node_type_t::AT_KEYWORD:
    case node_type_t::ARG:
    case node_type_t::COMPONENT_VALUE:
    case node_type_t::DECLARATION:
    case node_type_t::FUNCTION:
    case node_type_t::LIST:
    case node_type_t::OPEN_CURLYBRACKET:
    case node_type_t::OPEN_PARENTHESIS:
    case node_type_t::OPEN_SQUAREBRACKET:
        {
            // handle a special case which SETs a variable and cannot
            // get the first $<var> replaced
            bool is_variable_set(n->get_type() == node_type_t::COMPONENT_VALUE
                              && parser::is_variable_set(n, false));

            // replace all $<var> references with the corresponding value
            size_t idx(is_variable_set ? 1 : 0);
            while(idx < n->size())
            {
                node::pointer_t child(n->get_child(idx));
                if(child->is(node_type_t::VARIABLE))
                {
                    n->remove_child(idx);

                    // search for the variable and replace this 'child' with
                    // the contents of the variable
                    node::pointer_t value(get_variable(child));
                    switch(value->get_type())
                    {
                    case node_type_t::LIST:
                    case node_type_t::OPEN_CURLYBRACKET:
                    case node_type_t::OPEN_PARENTHESIS:
                    case node_type_t::OPEN_SQUAREBRACKET:
                        // in this case we insert the children of 'value'
                        // instead of the value itself
                        {
                            size_t const max_children(value->size());
                            for(size_t j(0), i(idx); j < max_children; ++j, ++i)
                            {
                                n->insert_child(i, value->get_child(j));
                            }
                        }
                        break;

                    case node_type_t::WHITESPACE:
                        // whitespaces by themselves do not get re-included,
                        // which may be a big mistake but at this point
                        // it seems wise to do so
                        break;

                    default:
                        n->insert_child(idx, value);
                        break;

                    }
                }
                else
                {
                    // recursive call to handle all children in the
                    // entire tree
                    switch(child->get_type())
                    {
                    case node_type_t::ARG:
                    case node_type_t::COMPONENT_VALUE:
                    case node_type_t::DECLARATION:
                    case node_type_t::FUNCTION:
                    case node_type_t::LIST:
                    case node_type_t::OPEN_CURLYBRACKET:
                    case node_type_t::OPEN_PARENTHESIS:
                    case node_type_t::OPEN_SQUAREBRACKET:
                        replace_variables(child);
                        ++idx;
                        break;

                    case node_type_t::AT_KEYWORD:
                        // handle @import, @mixins, @if, etc.
                        replace_variables(child);
                        replace_at_keyword(n, child, idx);
                        break;

                    default:
                        ++idx;
                        break;

                    }
                }
            }
            // TODO: remove lists that become empty?

            // handle the special case of a variable assignment
            if(is_variable_set)
            {
                // this is enough to get the variable removed
                // from COMPONENT_VALUE
                set_variable(n->get_child(0));
            }
        }
        break;

    default:
        // other nodes are not of interest here
        break;

    }
}

void compiler::set_variable(node::pointer_t n)
{
    // WARNING: 'n' is still the COMPONENT_VALUE and not the $var

    // a variable gets removed from the tree and its current value
    // saved in a parent node that is an OPEN_CURLYBRACKET or the
    // root node if no OPEN_CURLYBRACKET is found in the parents

    f_state.get_previous_parent()->remove_child(n);

    node::pointer_t var(n->get_child(0));
    std::string const & variable_name(var->get_string());

    n->remove_child(0);     // remove the VARIABLE
    if(n->get_child(0)->is(node_type_t::WHITESPACE))
    {
        n->remove_child(0); // remove the WHITESPACE
    }
    if(!n->get_child(0)->is(node_type_t::COLON))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable set is not exactly IDENTIFIER WHITESPACE* ':'."); // LCOV_EXCL_LINE
    }
    n->remove_child(0);     // remove the COLON

    // rename the node from COMPONENT_VALUE to a plain LIST
    node::pointer_t list(new node(node_type_t::LIST, n->get_position()));
    list->take_over_children_of(n);

    // now the value of the variable is 'list'; it will get compiled once in
    // context (i.e. not here)

    // search the parents for the node where the variable will be set
    node::pointer_t var_holder(f_state.find_selector());
    if(!var_holder)
    {
        var_holder = f_state.get_root();
    }

    // save the variable
    var_holder->set_variable(variable_name, list);
}

node::pointer_t compiler::get_variable(node::pointer_t n)
{
    std::string const & variable_name(n->get_string());

    // search the parents for the node where the variable will be set
    node::pointer_t var_holder(f_state.find_parent_by_type(node_type_t::OPEN_CURLYBRACKET));
    while(var_holder)
    {
        // we verify that the variable holder is a selector curly bracket
        // (if not we won't have variables defined in there anyway)
        if(var_holder->get_boolean())
        {
            node::pointer_t value(var_holder->get_variable(variable_name));
            if(value)
            {
                return value;
            }
        }
        var_holder = f_state.find_parent_by_type(node_type_t::OPEN_CURLYBRACKET, var_holder);
    }

    // if not found yet, check the root node too
    {
        node::pointer_t value(f_state.get_root()->get_variable(variable_name));
        if(value)
        {
            return value;
        }
    }

    if(f_empty_on_undefined_variable)
    {
        // returning "empty"...
        return node::pointer_t(new node(node_type_t::WHITESPACE, n->get_position()));
    }

    error::instance() << n->get_position()
            << "variable named \""
            << variable_name
            << "\" is not set."
            << error_mode_t::ERROR_ERROR;

    node::pointer_t fake(new node(node_type_t::IDENTIFIER, n->get_position()));
    fake->set_string("<undefined-variable(\"" + variable_name + "\")>");
    return fake;
}

void compiler::replace_at_keyword(node::pointer_t parent, node::pointer_t n, size_t & idx)
{
    // @<id> [expression] '{' ... '}'
    //
    // Note that the expression is optional.
    //
    // All the @-keyword that are used to control the flow of the
    // SCSS file are to be handled here; these include:
    //
    //  @else       -- changes what happens (i.e. sets a variable)
    //  @if         -- changes what happens (i.e. sets a variable)
    //  @import     -- changes input code
    //  @include    -- same as $var or $var(args)
    //  @mixin      -- changes variables
    //
    std::string const at(n->get_string());

    node::pointer_t expr;
    if(!n->empty()
    && !n->get_child(0)->is(node_type_t::OPEN_CURLYBRACKET))
    {
        if(at == "else"
        && n->get_child(0)->is(node_type_t::IDENTIFIER)
        && n->get_child(0)->get_string() == "if")
        {
            // this is a very special case of the:
            //
            //    @else if expr '{' ... '}'
            //
            // (this is from SASS, if it had been me, I would have used
            // @elseif or @else-if and not @else if ...)
            //
            n->remove_child(0);
            if(!n->empty() && n->get_child(0)->is(node_type_t::WHITESPACE))
            {
                // this should always happen because otherwise we are missing
                // the actual expression!
                n->remove_child(0);
            }
            if(n->size() == 1)
            {
                error::instance() << n->get_position()
                        << "'@else if ...' is missing an expression or a block"
                        << error_mode_t::ERROR_ERROR;
                return;
            }
        }
        expr = compile_expression(n, true, false);
    }

    if(at == "import")
    {
        replace_import(parent, n, expr, idx);
        return;
    }

    if(at == "mixin")
    {
        // mixins are handled like variables or
        // function declarations, so we always
        // remove them
        //
        parent->remove_child(idx);
        handle_mixin(n);
        return;
    }

    if(at == "if")
    {
        // get the position of the @if in its parent so we can insert new
        // data at that position if necessary
        //
        parent->remove_child(idx);
        replace_if(parent, n, expr, idx);
        return;
    }

    if(at == "else")
    {
        // remove the @else from the parent
        parent->remove_child(idx);
        replace_else(parent, n, expr, idx);
        return;
    }

    if(at == "include")
    {
        // this is SASS support, a more explicit way to insert a variable
        // I guess...
        parent->remove_child(idx);

        if(n->empty())
        {
            error::instance() << n->get_position()
                    << "@include is expected to be followed by an IDENTIFIER naming the variable/mixin to include."
                    << error_mode_t::ERROR_ERROR;
            return;
        }

        node::pointer_t id(n->get_child(0));
        if(!id->is(node_type_t::IDENTIFIER))
        {
            error::instance() << n->get_position()
                    << "@include is expected to be followed by an IDENTIFIER naming the variable/mixin to include."
                    << error_mode_t::ERROR_ERROR;
            return;
        }

        // search for the variable and replace this 'child' with
        // the contents of the variable
        node::pointer_t value(get_variable(id));
        switch(value->get_type())
        {
        case node_type_t::LIST:
        case node_type_t::OPEN_CURLYBRACKET:
        case node_type_t::OPEN_PARENTHESIS:
        case node_type_t::OPEN_SQUAREBRACKET:
            // in this case we insert the children of 'value'
            // instead of the value itself
            {
                size_t const max_children(value->size());
                for(size_t j(0), i(idx); j < max_children; ++j, ++i)
                {
                    n->insert_child(i, value->get_child(j));
                }
            }
            break;

        case node_type_t::WHITESPACE:
            // whitespaces by themselves do not get re-included,
            // which may be a big mistake but at this point
            // it seems wise to do so
            break;

        default:
            n->insert_child(idx, value);
            break;

        }
        return;
    }

    // in all other cases the @-keyword is kept as is
    ++idx;
}

void compiler::replace_if(node::pointer_t parent, node::pointer_t n, node::pointer_t expr, size_t idx)
{
    // make sure that we got a valid syntax
    if(n->size() != 2 || !expr)
    {
        error::instance() << n->get_position()
                << "@if is expected to have exactly 2 parameters: an expression and a block. This @if has "
                << static_cast<int>(n->size())
                << " parameters."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    boolean_t const r(expression::boolean(expr));
    if(r == boolean_t::TRUE)
    {
        // TRUE, we need the data which we put in the stream
        // at the position of the @if as if the @if and
        // expression never existed
        node::pointer_t block(n->get_child(1));
        size_t const max_children(block->size());
        for(size_t j(0); j < max_children; ++j, ++idx)
        {
            parent->insert_child(idx, block->get_child(j));
        }
    }

    // we want to mark the next block as valid if it is an
    // '@else' or '@else if' and can possibly be inserted
    if(idx < parent->size())
    {
        node::pointer_t next(parent->get_child(idx));
        if(next->is(node_type_t::AT_KEYWORD)
        && next->get_string() == "else")
        {
            // mark that the @else is at the right place
            // (i.e. an @else with integer == 0 is an error)
            next->set_integer(r == boolean_t::TRUE ? g_if_or_else_executed : g_if_or_else_false_so_far);
        }
    }
}

void compiler::replace_else(node::pointer_t parent, node::pointer_t n, node::pointer_t expr, size_t idx)
{
    // if this '@else' is still marked with 'g_if_or_else_undefined'
    // then there was no '@if' or '@else if' before it which is an error
    //
    int status(n->get_integer());
    if(status == g_if_or_else_undefined)
    {
        error::instance() << n->get_position()
                << "a standalone @else is not legal, it has to be preceeded by a @if ... or @else if ..."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    //
    // when the '@if' or any '@else if' all had a 'false' expression,
    // we are 'true' here; once one of the '@if' / '@else if' is 'true'
    // then we start with 'r = false'
    //
    boolean_t r(status == g_if_or_else_false_so_far ? boolean_t::TRUE : boolean_t::FALSE);
    if(n->size() != 1)
    {
        if(n->size() != 2 || !expr)
        {
            error::instance() << n->get_position()
                    << "'@else { ... }' is expected to have 1 parameter, '@else if ... { ... }' is expected to have 2 parameters. This @else has "
                    << static_cast<int>(n->size())
                    << " parameters."
                    << error_mode_t::ERROR_ERROR;
            return;
        }

        // as long as 'status == g_if_or_else_false_so_far' we have
        // not yet found a match (i.e. the starting '@if' was false
        // and any '@else if' were all false so far) so we check the
        // expression of this very '@else if' to know whether to go
        // on or not; r is TRUE when the status allows us to check
        // the next expression
        if(r == boolean_t::TRUE)
        {
            r = expression::boolean(expr);
        }
    }

    if(r == boolean_t::TRUE)
    {
        status = g_if_or_else_executed;

        // TRUE, we need the data which we put in the stream
        // at the position of the @if as if the @if and
        // expression never existed
        node::pointer_t block(n->get_child(n->size() == 1 ? 0 : 1));
        size_t const max_children(block->size());
        for(size_t j(0); j < max_children; ++j, ++idx)
        {
            parent->insert_child(idx, block->get_child(j));
        }
    }

    // FALSE or INVALID, we remove the block to avoid
    // executing it since we do not know whether it should
    // be executed or not; also we mark the next block as
    // "true" if it is an '@else' or '@else if'
    if(idx < parent->size())
    {
        node::pointer_t next(parent->get_child(idx));
        if(next->is(node_type_t::AT_KEYWORD)
        && next->get_string() == "else")
        {
            if(n->size() == 1)
            {
                error::instance() << n->get_position()
                        << "'@else { ... }' cannot follow another '@else { ... }'. Maybe you are missing an 'if expr'?"
                        << error_mode_t::ERROR_ERROR;
                return;
            }

            // mark that the '@else' is at the right place and whether
            // it may be 'true' (g_if_or_else_false_so_far) or not
            // (g_if_or_else_executed); our status already shows
            // what it can be
            //
            next->set_integer(status);
        }
    }
}

bool compiler::argify(node::pointer_t n)
{
    size_t const max_children(n->size());
    if(max_children > 0)
    {
        node::pointer_t temp(new node(node_type_t::LIST, n->get_position()));
        temp->take_over_children_of(n);

        node::pointer_t arg(new node(node_type_t::ARG, n->get_position()));
        n->add_child(arg);

        for(size_t i(0); i < max_children; ++i)
        {
            node::pointer_t child(temp->get_child(i));
            if(child->is(node_type_t::OPEN_CURLYBRACKET))
            {
                if(i + 1 != max_children)
                {
                    throw csspp_exception_logic("compiler.cpp:compiler::argify(): list that has an OPEN_CURLYBRACKET that is not the last child."); // LCOV_EXCL_LINE
                }
                n->add_child(child);
                break;
            }
            if(child->is(node_type_t::COMMA))
            {
                // make sure to remove any WHITESPACE appearing just
                // before a comma
                while(!arg->empty() && arg->get_last_child()->is(node_type_t::WHITESPACE))
                {
                    arg->remove_child(arg->get_last_child());
                }
                if(arg->empty())
                {
                    if(n->size() == 1)
                    {
                        error::instance() << n->get_position()
                                << "dangling comma at the beginning of a list of arguments or selectors."
                                << error_mode_t::ERROR_ERROR;
                    }
                    else
                    {
                        error::instance() << n->get_position()
                                << "two commas in a row are invalid in a list of arguments or selectors."
                                << error_mode_t::ERROR_ERROR;
                    }
                    return false;
                }
                if(i + 1 == max_children
                || temp->get_child(i + 1)->is(node_type_t::OPEN_CURLYBRACKET))
                {
                    error::instance() << n->get_position()
                            << "dangling comma at the end of a list of arguments or selectors."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
                // move to the next 'arg'
                arg.reset(new node(node_type_t::ARG, n->get_position()));
                n->add_child(arg);
            }
            else if(!child->is(node_type_t::WHITESPACE) || !arg->empty())
            {
                arg->add_child(child);
            }
        }
    }

    return true;
}

bool compiler::selector_attribute_check(node::pointer_t n)
{
    // use a for() as a 'goto exit;' on a 'break'
    for(;;)
    {
        size_t pos(0);
        node::pointer_t term(n->get_child(pos));
        if(term->is(node_type_t::WHITESPACE))
        {
            // I'm keeping this here, although there should be no WHITESPACE
            // at the start of a '[' block
            n->remove_child(term);          // LCOV_EXCL_LINE
            if(pos >= n->size())            // LCOV_EXCL_LINE
            {
                break;                      // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);       // LCOV_EXCL_LINE
        }

        if(!term->is(node_type_t::IDENTIFIER))
        {
            error::instance() << n->get_position()
                    << "an attribute selector expects to first find an identifier."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        ++pos;
        if(pos >= n->size())
        {
            // just IDENTIFIER is valid
            return true;
        }

        term = n->get_child(pos);
        if(term->is(node_type_t::WHITESPACE))
        {
            n->remove_child(pos);
            if(pos >= n->size())
            {
                // just IDENTIFIER is valid, although we should never
                // reach this line because WHITESPACE are removed from
                // the end of lists
                return true;  // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);
        }

        if(!term->is(node_type_t::EQUAL)                // '='
        && !term->is(node_type_t::INCLUDE_MATCH)        // '~='
        && !term->is(node_type_t::PREFIX_MATCH)         // '^='
        && !term->is(node_type_t::SUFFIX_MATCH)         // '$='
        && !term->is(node_type_t::SUBSTRING_MATCH)      // '*='
        && !term->is(node_type_t::DASH_MATCH))          // '|='
        {
            error::instance() << n->get_position()
                    << "expected attribute operator missing, supported operators are '=', '~=', '^=', '$=', '*=', and '|='."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        ++pos;
        if(pos >= n->size())
        {
            break;
        }

        term = n->get_child(pos);
        if(term->is(node_type_t::WHITESPACE))
        {
            n->remove_child(pos);
            if(pos >= n->size())
            {
                // we actually are not expected to ever have a WHITESPACE
                // at the end of a block so we cannot hit this line, but
                // we keep it, just in case we were wrong...
                break; // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);
        }

        if(!term->is(node_type_t::IDENTIFIER)
        && !term->is(node_type_t::STRING)
        && !term->is(node_type_t::INTEGER)
        && !term->is(node_type_t::DECIMAL_NUMBER))
        {
            error::instance() << n->get_position()
                    << "attribute selector value must be an identifier, a string, an integer, or a decimal number, a "
                    << term->get_type() << " is not acceptable."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        ++pos;
        if(pos < n->size())  // <<-- inverted test!
        {
            error::instance() << n->get_position()
                    << "attribute selector cannot be followed by more than one value, found "
                    << n->get_child(pos)->get_type() << " after the value, missing quotes?"
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        return true;
    }

    error::instance() << n->get_position()
            << "the attribute selector is expected to be an IDENTIFIER optionally followed by an operator and a value."
            << error_mode_t::ERROR_ERROR;
    return false;
}

bool compiler::selector_simple_term(node::pointer_t n, size_t & pos)
{
    // test with `pos + 1` since the last item in the list is not a selector,
    // it is the curly bracket block
    if(pos >= n->size())
    {
        throw csspp_exception_logic("compiler.cpp:compiler::selector_term(): selector_simple_term() called when not enough selectors are available."); // LCOV_EXCL_LINE
    }

    node::pointer_t term(n->get_child(pos));
    switch(term->get_type())
    {
    case node_type_t::HASH:
        // valid term as is
        break;

    case node_type_t::IDENTIFIER:
    case node_type_t::MULTIPLY:
        // IDENTIFIER
        // IDENTIFIER '|' IDENTIFIER
        // IDENTIFIER '|' '*'
        // '*'
        // '*' '|' IDENTIFIER
        // '*' '|' '*'
        if(pos + 1 < n->size())
        {
            if(n->get_child(pos + 1)->is(node_type_t::SCOPE))
            {
                if(pos + 2 >= n->size())
                {
                    error::instance() << n->get_position()
                            << "the scope operator (|) requires a right hand side identifier or '*'."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
                pos += 2;
                term = n->get_child(pos);
                if(!term->is(node_type_t::IDENTIFIER)
                && !term->is(node_type_t::MULTIPLY))
                {
                    error::instance() << n->get_position()
                            << "the right hand side of a scope operator (|) must be an identifier or '*'."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
            }
            else if(term->is(node_type_t::MULTIPLY)
                 && (n->get_child(pos + 1)->is(node_type_t::OPEN_SQUAREBRACKET)
                  || n->get_child(pos + 1)->is(node_type_t::PERIOD)))
            {
                // this asterisk is not required, get rid of it
                n->remove_child(term);
                return true; // return immediately to avoid the ++pos
            }
        }
        break;

    case node_type_t::SCOPE:
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a scope selector (|) must be followed by an identifier or '*'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        if(!term->is(node_type_t::IDENTIFIER)
        && !term->is(node_type_t::MULTIPLY))
        {
            error::instance() << n->get_position()
                    << "the right hand side of a scope operator (|) must be an identifier or '*'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        break;

    case node_type_t::COLON:
        ++pos;
        if(pos >= n->size())
        {
            // this is caught by the selector_term() when reading the '::'
            // so we cannot reach this time; keeping just in case though...
            error::instance() << n->get_position()
                    << "a selector list cannot end with a standalone ':'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        switch(term->get_type())
        {
        case node_type_t::IDENTIFIER:
            {
                // ':' IDENTIFIER
                // validate the identifier as only a small number can be used
                set_validation_script("pseudo-classes");
                node::pointer_t str(new node(node_type_t::STRING, term->get_position()));
                str->set_string(term->get_string());
                add_validation_variable("pseudo_name", str);
                if(!run_validation(false))
                {
                    return false;
                }
            }
            break;

        case node_type_t::FUNCTION:
            {
                // ':' FUNCTION component-value-list ')'
                //
                // create a temporary identifier to run the validation
                // checks, because the FUNCTION is a list of nodes!
                node::pointer_t function_name(new node(node_type_t::STRING, term->get_position()));
                function_name->set_string(term->get_string());
                set_validation_script("pseudo-nth-functions");
                add_validation_variable("pseudo_name", function_name);
                if(run_validation(true))
                {
                    // this is a valid nth function, print out its parameters
                    // and reparse as 'An+B'
                    size_t const max_children(term->size());
                    std::string an_b;
                    for(size_t idx(0); idx < max_children; ++idx)
                    {
                        an_b += term->get_child(idx)->to_string(node::g_to_string_flag_show_quotes);
                    }
                    // TODO...
                    nth_child nc;
                    if(nc.parse(an_b))
                    {
                        // success, save the compiled An+B in this object
                        node::pointer_t an_b_node(new node(node_type_t::AN_PLUS_B, term->get_position()));
                        an_b_node->set_integer(nc.get_nth());
                        term->clear();
                        term->add_child(an_b_node);
                    }
                    else
                    {
                        // get the error and display it
                        error::instance() << term->get_position()
                                << nc.get_error()
                                << error_mode_t::ERROR_ERROR;
                        return false;
                    }
                }
                else
                {
                    set_validation_script("pseudo-functions");
                    add_validation_variable("pseudo_name", function_name);
                    if(!run_validation(false))
                    {
                        return false;
                    }
                    // this is a standard function, check the parameters
                    if(term->get_string() == "not")
                    {
                        // :not(:not(...)) is illegal
                        error::instance() << n->get_position()
                                << "the :not() selector does not accept an inner :not()."
                                << error_mode_t::ERROR_ERROR;
                        return false;
                    }
                    else if(term->get_string() == "lang")
                    {
                        // the language must be an identifier with no dashes
                        if(term->size() != 1)
                        {
                            error::instance() << term->get_position()
                                    << "a lang() function selector must have exactly one identifier as its parameter."
                                    << error_mode_t::ERROR_ERROR;
                            return false;
                        }
                        term = term->get_child(0);
                        if(term->is(node_type_t::IDENTIFIER))
                        {
                            std::string lang(term->get_string());
                            std::string country;
                            std::string::size_type char_pos(lang.find('-'));
                            if(char_pos != std::string::npos)
                            {
                                country = lang.substr(char_pos + 1);
                                lang = lang.substr(0, char_pos);
                                char_pos = country.find('-');
                                if(char_pos != std::string::npos)
                                {
                                    // remove whatever other information that
                                    // we will ignore in our validations
                                    country = country.substr(0, char_pos);
                                }
                            }
                            // check the language (mandatory)
                            node::pointer_t language_name(new node(node_type_t::STRING, term->get_position()));
                            language_name->set_string(lang);
                            set_validation_script("languages");
                            add_validation_variable("language_name", language_name);
                            if(!run_validation(false))
                            {
                                return false;
                            }
                            if(!country.empty())
                            {
                                // check the country (optional)
                                node::pointer_t country_name(new node(node_type_t::STRING, term->get_position()));
                                country_name->set_string(country);
                                set_validation_script("countries");
                                add_validation_variable("country_name", country_name);
                                if(!run_validation(false))
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            error::instance() << term->get_position()
                                    << "a lang() function selector expects an identifier as its parameter."
                                    << error_mode_t::ERROR_ERROR;
                            return false;
                        }
                    }
                }
            }
            break;

        default:
            // invalid selector list
            error::instance() << n->get_position()
                    << "a ':' selector must be followed by an identifier or a function, a " << n->get_type() << " was found instead."
                    << error_mode_t::ERROR_ERROR;
            return false;

        }
        break;

    case node_type_t::PERIOD:
        // '.' IDENTIFIER -- class (special attribute check)
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a selector list cannot end with a standalone '.'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        if(!term->is(node_type_t::IDENTIFIER))
        {
            error::instance() << n->get_position()
                    << "a class selector (after a period: '.') must be an identifier."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        break;

    case node_type_t::OPEN_SQUAREBRACKET:
        // '[' WHITESPACE attribute-check WHITESPACE ']' -- attributes check
        ++pos;
        return selector_attribute_check(term);

    case node_type_t::GREATER_THAN:
    case node_type_t::ADD:
    case node_type_t::PRECEDED:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which cannot be used to start a selector expression."
                << error_mode_t::ERROR_ERROR;
        return false;

    case node_type_t::FUNCTION:
        error::instance() << n->get_position()
                << "found function \"" << term->get_string() << "()\", which may be a valid selector token but only if immediately preceeded by a ':' (simple term)."
                << error_mode_t::ERROR_ERROR;
        return false;

    default:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which is not a valid selector token (simple term)."
                << error_mode_t::ERROR_ERROR;
        return false;

    }

    // move on to the next term
    ++pos;

    return true;
}

bool compiler::selector_term(node::pointer_t n, size_t & pos)
{
    if(pos >= n->size())
    {
        throw csspp_exception_logic("compiler.cpp:compiler::selector_term(): selector_term() called when not enough selectors are available."); // LCOV_EXCL_LINE
    }

    node::pointer_t term(n->get_child(pos));
    switch(term->get_type())
    {
    case node_type_t::PLACEHOLDER:
    case node_type_t::REFERENCE:
        // valid complex term as is
        break;

    case node_type_t::COLON:
        // ':' FUNCTION (="not") is a term and has to be managed here
        // '::' IDENTIFIER is a term and not a simple term (it cannot
        //                 appear inside a :not() function.)
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a selector list cannot end with a standalone ':'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        switch(term->get_type())
        {
        case node_type_t::IDENTIFIER:
            --pos;
            return selector_simple_term(n, pos);

        case node_type_t::FUNCTION:
            // ':' FUNCTION component-value-list ')'
            if(term->get_string() == "not")
            {
                // skip FUNCTION
                ++pos;

                // special handling, the :not() is considered to be
                // a complex selector and as such has to be handled
                // right here; the parameters must represent one valid
                // simple term
                //
                // TODO: got to take care of WHITESPACE, plus the
                //       end of the list of children is NOT a {}-block
                //       (argh!)
                size_t sub_pos(0);
                return selector_simple_term(term, sub_pos);
            }
            else
            {
                --pos;
                return selector_simple_term(n, pos);
            }
            break;

        case node_type_t::COLON:
            {
                // '::' IDENTIFIER -- pseudo elements
                ++pos;
                if(pos >= n->size())
                {
                    error::instance() << n->get_position()
                            << "a selector list cannot end with a '::'."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
                term = n->get_child(pos);
                if(!term->is(node_type_t::IDENTIFIER))
                {
                    error::instance() << n->get_position()
                            << "a pseudo element name (defined after a '::' in a list of selectors) must be defined using an identifier."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
                // only a few pseudo element names exist, do a validation
                node::pointer_t pseudo_element(new node(node_type_t::STRING, term->get_position()));
                pseudo_element->set_string(term->get_string());
                set_validation_script("pseudo-elements");
                add_validation_variable("pseudo_name", pseudo_element);
                if(!run_validation(false))
                {
                    return false;
                }
            }
            break;

        default:
            // invalid selector list
            error::instance() << n->get_position()
                    << "a ':' selector must be followed by an identifier or a function, a " << term->get_type() << " was found instead."
                    << error_mode_t::ERROR_ERROR;
            return false;

        }
        break;

    case node_type_t::HASH:
    case node_type_t::IDENTIFIER:
    case node_type_t::MULTIPLY:
    case node_type_t::OPEN_SQUAREBRACKET:
    case node_type_t::PERIOD:
    case node_type_t::SCOPE:
        return selector_simple_term(n, pos);

    case node_type_t::GREATER_THAN:
    case node_type_t::ADD:
    case node_type_t::PRECEDED:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which cannot be used to start a selector expression."
                << error_mode_t::ERROR_ERROR;
        return false;

    case node_type_t::FUNCTION:
        error::instance() << n->get_position()
                << "found function \"" << term->get_string() << "()\", which may be a valid selector token but only if immediately preceeded by a ':' (term)."
                << error_mode_t::ERROR_ERROR;
        return false;

    default:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which is not a valid selector token (term)."
                << error_mode_t::ERROR_ERROR;
        return false;

    }

    // move on to the next term
    ++pos;

    return true;
}

bool compiler::selector_list(node::pointer_t n, size_t & pos)
{
    // we must have a term first
    if(!selector_term(n, pos))
    {
        return false;
    }

    for(;;)
    {
        if(pos >= n->size())
        {
            return true;
        }

        // skip whitespaces between terms
        // this also works for binary operators
        node::pointer_t term(n->get_child(pos));
        if(term->is(node_type_t::WHITESPACE))
        {
            ++pos;

            // end of list too soon?
            if(pos >= n->size())
            {
                // this should not happen since we remove leading/trailing
                // white space tokens
                throw csspp_exception_logic("compiler.cpp: a component value has a WHITESPACE token before the OPEN_CURLYBRACKET."); // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);
        }

        if(term->is(node_type_t::GREATER_THAN)
        || term->is(node_type_t::ADD)
        || term->is(node_type_t::PRECEDED))
        {
            // if we had a WHITESPACE just before the binary operator,
            // remove it as it is not necessary
            if(n->get_child(pos - 1)->is(node_type_t::WHITESPACE))
            {
                n->remove_child(pos - 1);
            }
            else
            {
                ++pos;
            }

            // it is mandatory for these tokens to be followed by another
            // term (i.e. binary operators)
            if(pos >= n->size())
            {
                error::instance() << n->get_position()
                        << "found token " << term->get_type() << ", which is expected to be followed by another selector term."
                        << error_mode_t::ERROR_ERROR;
                return false;
            }

            // we may have a WHITESPACE first, if so skip it
            term = n->get_child(pos);
            if(term->is(node_type_t::WHITESPACE))
            {
                // no need before/after binary operators
                n->remove_child(term);

                // end of list too soon?
                if(pos >= n->size())
                {
                    // this should not happen since we remove leading/trailing
                    // white space tokens
                    throw csspp_exception_logic("compiler.cpp: a component value has a WHITESPACE token before the OPEN_CURLYBRACKET."); // LCOV_EXCL_LINE
                }
            }
        }

        if(!selector_term(n, pos))
        {
            return false;
        }
    }
}

bool compiler::parse_selector(node::pointer_t n)
{
    if(!argify(n))
    {
        return false;
    }

    size_t const max_children(n->size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t arg(n->get_child(idx));
        if(arg->is(node_type_t::OPEN_CURLYBRACKET))
        {
            // this is at the end of the list, so we're done
            break;
        }
        if(!arg->is(node_type_t::ARG))
        {
            throw csspp_exception_logic("compiler.cpp: parse_selector() just called argify() and yet a child is not an ARG."); // LCOV_EXCL_LINE
        }
        size_t pos(0);
        if(!selector_list(arg, pos))
        {
            return false;
        }
    }

    return true;
}

std::string compiler::find_file(std::string const & script_name)
{
    if(f_paths.empty())
    {
        // should this be "." here instead of the default?
        f_paths.push_back("/usr/lib/csspp/scripts");
    }

    for(auto it : f_paths)
    {
        std::string const name(it == "" ? script_name : it + "/" + script_name);
        if(access(name.c_str(), R_OK) == 0)
        {
            return name;
        }
    }

    // in case we cannot find a file
    return std::string();
}

void compiler::set_validation_script(std::string const & script_name)
{
    // try the filename as is first
    std::string filename(find_file(script_name));
    if(filename.empty())
    {
        if(script_name.substr(script_name.size() - 5) != ".scss")
        {
            // try again with the "scss" extension
            filename = find_file(script_name + ".scss");
        }
    }

    if(filename.empty())
    {
        // a validation script should always be available, right?
        position pos(script_name);
        error::instance() << pos
                << "validation script \""
                << script_name
                << "\" was not found."
                << error_mode_t::ERROR_FATAL;
        throw csspp_exception_exit(1);
    }

    node::pointer_t script;

    auto cache(f_validator_scripts.find(filename));
    if(cache == f_validator_scripts.end())
    {
        position pos(filename);

        // the file exists, read it now
        std::ifstream in;
        in.open(filename);
        if(!in)
        {
            // a validation script should always be available, right?
            error::instance() << pos
                    << "validation script \""
                    << script_name
                    << "\" could not be opened."
                    << error_mode_t::ERROR_FATAL;
            throw csspp_exception_exit(1);
        }

        lexer::pointer_t l(new lexer(in, pos));
        parser p(l);
        script = p.stylesheet();

        // TODO: test whether errors occurred while reading the script, if
        //       so then we have to generate a FATAL error here

        // cache the script
        f_validator_scripts[filename] = script;
//std::cerr << "script " << filename << " is:\n" << *script;
    }
    else
    {
        script = cache->second;
    }

    f_current_validation_script = script;
    script->clear_variables();
}

void compiler::add_validation_variable(std::string const & variable_name, node::pointer_t value)
{
    if(!f_current_validation_script)
    {
        throw csspp_exception_logic("compiler.cpp: somehow add_validation_variable() was called without a current validation script set."); // LCOV_EXCL_LINE
    }

    f_current_validation_script->set_variable(variable_name, value);
}

bool compiler::run_validation(bool check_only)
{
    // avoid validation from within a validation script (we probably would
    // get infinite loops anyway)
    if(!f_compiler_validating)
    {
        // save the number of errors so we can test after we ran
        // the compile() function
        error_happened_t old_count;

        safe_compiler_state_t safe_state(f_state);
        f_state.set_root(f_current_validation_script);
        if(check_only)
        {
            // save the current error/warning counters so they do not change
            // on this run
            safe_error_t safe_error;

            // replace the output stream with a memory buffer so the user
            // does not see any of it
            std::stringstream ignore;
            safe_error_stream_t safe_output(ignore);

            // now compile that true/false check
            compile();

            // WARNING: this MUST be here (before the closing curly bracket)
            //          and not after the if() since we restore the error
            //          state from before the compile() call.
            //
            bool const result(!old_count.error_happened());

            // now restore the stream and error counters
            return result;
        }
        else
        {
            compile();

            return !old_count.error_happened();
        }
    }

    return true;
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
