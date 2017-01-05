#ifndef CSSPP_PARSER_H
#define CSSPP_PARSER_H
// CSS Preprocessor
// Copyright (C) 2015-2017  Made to Order Software Corp.
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

#include "csspp/lexer.h"

namespace csspp
{

class parser
{
public:
                            parser(lexer::pointer_t l);

    node::pointer_t         stylesheet();           // <style>...</style>
    node::pointer_t         rule_list();            // file.css
    node::pointer_t         rule();
    node::pointer_t         declaration_list();     // <div style="...">...</div>
    node::pointer_t         component_value_list();
    node::pointer_t         component_value();

    static bool             is_variable_set(node::pointer_t n, bool with_block);
    static bool             is_nested_declaration(node::pointer_t n);
    static bool             argify(node::pointer_t n, node_type_t const separator = node_type_t::COMMA);

private:
    node::pointer_t         next_token();

    node::pointer_t         stylesheet(node::pointer_t n);
    node::pointer_t         rule_list(node::pointer_t n);
    node::pointer_t         rule(node::pointer_t n);
    node::pointer_t         at_rule(node::pointer_t at_keyword);
    node::pointer_t         qualified_rule(node::pointer_t n);
    node::pointer_t         declaration_list(node::pointer_t n);
    node::pointer_t         declaration(node::pointer_t identifier);
    node::pointer_t         component_value_list(node::pointer_t n, int flags);
    node::pointer_t         component_value(node::pointer_t n);
    node::pointer_t         block(node::pointer_t b, node_type_t closing_token);
    node::pointer_t         block_list(node::pointer_t b);

    lexer::pointer_t        f_lexer;
    node::pointer_t         f_last_token;
};

} // namespace csspp
#endif
// #ifndef CSSPP_PARSER_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
