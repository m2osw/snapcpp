/* parser_numeric_type.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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
/***  PARSER NUMERIC TYPE  ********************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::numeric_type(Node::pointer_t& numeric_type_node, Node::pointer_t& name)
{
    // TBD: can we really use NODE_TYPE here?
    numeric_type_node = f_lexer->get_new_node(Node::node_t::NODE_TYPE);

    numeric_type_node->append_child(name);

    // we are called with the current token set to NODE_AS, get
    // the following token, it has to be a literal number
    //
    // TODO: support any constant expression
    //
    get_token();
    if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER
    && f_node->get_string() == "mod")
    {
        get_token();
        if(f_node->get_type() != Node::node_t::NODE_INT64
        && f_node->get_type() != Node::node_t::NODE_FLOAT64)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
            msg << "invalid numeric type declaration, the modulo must be a literal number";
            return;
        }

        // use name as mod 123;
        numeric_type_node->append_child(f_node);
        get_token();
        return;
    }

    if(f_node->get_type() != Node::node_t::NODE_INT64
    && f_node->get_type() != Node::node_t::NODE_FLOAT64)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must start with a literal number";
        // TODO: skip till next ';'
        return;
    }

    numeric_type_node->append_child(f_node);

    // now we expect '..'
    get_token();
    if(f_node->get_type() != Node::node_t::NODE_RANGE)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must use '..' to separate the minimum and maximum boundaries";
    }
    else
    {
        get_token();
    }

    if(f_node->get_type() != Node::node_t::NODE_INT64
    && f_node->get_type() != Node::node_t::NODE_FLOAT64)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must end with a literal number";
        // TODO: skip till next ';'
        return;
    }

    // use name as 0 .. 100;
    numeric_type_node->append_child(f_node);

    get_token();
}






}
// namespace as2js

// vim: ts=4 sw=4 et
