/* parser_numeric_type.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

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
        numeric_type_node->set_flag(Node::flag_t::NODE_TYPE_FLAG_MODULO, true);

        // skip the word 'mod'
        get_token();

        if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
            msg << "missing literal number for a numeric type declaration.";
            return;
        }

        if(f_node->get_type() != Node::node_t::NODE_INT64
        && f_node->get_type() != Node::node_t::NODE_FLOAT64)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
            msg << "invalid numeric type declaration, the modulo must be a literal number.";

            // skip that token because it's useless, and we expect
            // a semi-colon right after that
            get_token();
            return;
        }

        // RESULT OF: use name as mod 123;
        numeric_type_node->append_child(f_node);
        get_token();
        return;
    }

    Node::node_t left_type(f_node->get_type());
    int sign(1);
    if(left_type == Node::node_t::NODE_ADD)
    {
        get_token();
        left_type = f_node->get_type();
    }
    else if(left_type == Node::node_t::NODE_SUBTRACT)
    {
        sign = -1;
        get_token();
        left_type = f_node->get_type();
    }
    if(left_type == Node::node_t::NODE_INT64)
    {
        Int64 i(f_node->get_int64());
        i.set(i.get() * sign);
        f_node->set_int64(i);
    }
    else if(left_type == Node::node_t::NODE_FLOAT64)
    {
        Float64 f(f_node->get_float64());
        f.set(f.get() * sign);
        f_node->set_float64(f);
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must start with a literal number.";
        // TODO: we may want to check whether the next
        //       token is '..' or ';'...
        return;
    }

    Node::pointer_t left_node(f_node);
    numeric_type_node->append_child(f_node);

    // now we expect '..'
    get_token();
    if(f_node->get_type() != Node::node_t::NODE_RANGE)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must use '..' to separate the minimum and maximum boundaries (unexpected '" << f_node->get_type_name() << "').";

        // in case the user put '...' instead of '..'
        if(f_node->get_type() == Node::node_t::NODE_REST)
        {
            get_token();
        }
    }
    else
    {
        get_token();
    }

    Node::node_t right_type(f_node->get_type());
    sign = 1;
    if(right_type == Node::node_t::NODE_ADD)
    {
        get_token();
        right_type = f_node->get_type();
    }
    else if(right_type == Node::node_t::NODE_SUBTRACT)
    {
        sign = -1;
        get_token();
        right_type = f_node->get_type();
    }
    if(right_type == Node::node_t::NODE_INT64)
    {
        Int64 i(f_node->get_int64());
        i.set(i.get() * sign);
        f_node->set_int64(i);
    }
    else if(right_type == Node::node_t::NODE_FLOAT64)
    {
        Float64 f(f_node->get_float64());
        f.set(f.get() * sign);
        f_node->set_float64(f);
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must end with a literal number.";
        if(f_node->get_type() != Node::node_t::NODE_SEMICOLON)
        {
            // avoid an additional error
            get_token();
        }
        return;
    }

    // RESULT OF: use name as 0 .. 100;
    Node::pointer_t right_node(f_node);
    numeric_type_node->append_child(f_node);

    get_token();

    // we verify this after the get_token() to skip the
    // second number so we do not generate yet another error
    if(right_type != left_type)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
        msg << "invalid numeric type declaration, the range must use numbers of the same type on both sides.";
    }
    else if(left_type == Node::node_t::NODE_INT64)
    {
        if(left_node->get_int64().get() > right_node->get_int64().get())
        {
            Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
            msg << "numeric type declaration is empty (only accepts 'null') because left value of range is larger than right value.";
        }
    }
    else
    {
        if(left_node->get_float64().get() > right_node->get_float64().get())
        {
            Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_BAD_NUMERIC_TYPE, f_lexer->get_input()->get_position());
            msg << "numeric type declaration is empty (only accepts 'null') because left value of range is larger than right value.";
        }
    }
}






}
// namespace as2js

// vim: ts=4 sw=4 et
