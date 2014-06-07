/* variable.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "as2js/parser.h"
#include    "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER VARIABLE  ************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::variable(Node::pointer_t& node, bool const constant)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_VAR);
    for(;;)
    {
        Node::pointer_t variable_node(f_lexer->get_new_node(Node::node_t::NODE_VARIABLE));
        if(constant)
        {
            variable_node->set_flag(Node::flag_attribute_t::NODE_VAR_FLAG_CONST, true);
        }
        node->append_child(variable_node);

        if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
        {
            get_token();
        }
        else
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_VARIABLE, f_lexer->get_input()->get_position());
            msg << "expected an identifier as the variable name";
        }

        if(f_node->get_type() == Node::node_t::NODE_COLON)
        {
            get_token();
            Node::pointer_t type;
            conditional_expression(type, false);
            variable_node->append_child(type);
        }

        if(f_node->get_type() == Node::node_t::NODE_ASSIGNMENT)
        {
            get_token();
            do
            {
                Node::pointer_t initializer(f_lexer->get_new_node(Node::node_t::NODE_SET));
                Node::pointer_t expr;
                conditional_expression(expr, false);
                initializer->append_child(expr);
                variable_node->append_child(initializer);
                // We loop in case we have a list of attributes!
                // This could also be a big syntax error (a missing
                // operator in most cases.) We will report the error
                // later once we know where the variable is being
                // used.
            }
            while(constant
                && f_node->get_type() != Node::node_t::NODE_COMMA
                && f_node->get_type() != Node::node_t::NODE_SEMICOLON
                && f_node->get_type() != Node::node_t::NODE_OPEN_CURVLY_BRACKET
                && f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET
                && f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS);
        }

        if(f_node->get_type() == Node::node_t::NODE_COMMA)
        {
            return;
        }
        get_token();
    }
}




}
// namespace as2js

// vim: ts=4 sw=4 et
