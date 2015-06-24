#ifndef CSSPP_EXPRESSION_H
#define CSSPP_EXPRESSION_H
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

namespace csspp
{

class expression
{
public:
                        expression(node::pointer_t n, bool skip_whitespace);
    node::pointer_t     compile_list();
    node::pointer_t     compile();
    bool                end_of_nodes();
    void                mark_start();
    node::pointer_t     replace_with_result(node::pointer_t result);
    void                next();
    node::pointer_t     look_ahead() const;
    node::pointer_t     current() const;
    node::pointer_t     conditional();
    node::pointer_t     argument_list();

    static boolean_t    boolean(node::pointer_t n);

private:
    typedef std::map<std::string, node::pointer_t>  variable_vector_t;

    node::pointer_t     expression_list();
    node::pointer_t     assignment();
    node::pointer_t     logical_or();
    node::pointer_t     logical_and();
    node_type_t         equality_operator(node::pointer_t n);
    node::pointer_t     equality();
    node::pointer_t     relational();
    node::pointer_t     additive();
    node_type_t         multiplicative_operator(node::pointer_t n);
    node::pointer_t     multiply(node_type_t op, node::pointer_t lhs, node::pointer_t rhs);
    node::pointer_t     multiplicative();
    node::pointer_t     apply_power(node::pointer_t lhs, node::pointer_t rhs);
    node::pointer_t     power();
    node::pointer_t     post();
    node::pointer_t     unary();
    node::pointer_t     excecute_function(node::pointer_t func);

    node::pointer_t     f_node;
    size_t              f_pos = 0;
    size_t              f_start = static_cast<size_t>(-1);
    node::pointer_t     f_current;
    variable_vector_t   f_variables;
    bool                f_skip_whitespace = false;
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
