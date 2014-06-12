/* parser_statement.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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
/***  PARSER BLOCK  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::block(Node::pointer_t& node)
{
    // handle the emptiness right here
    if(f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        directive_list(node);
    }

    if(f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'}' expected to close a block";
    }
    else
    {
        // skip the '}'
        get_token();
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER BREAK & CONTINUE  ****************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Read a break or continue statement.
 *
 * The statement is a break or continue optionally followed by a label
 * (an identifier) or the default keyword (a special label meaning
 * use the default behavior.)
 *
 * Then we expect a semi-colon.
 *
 * The label is saved in the break or continue statement as the string
 * of the break or continue node.
 *
 * \code
 *     // A break by itself or the default break
 *     break;
 *     break default;
 *    
 *     // A break with a label
 *     break label;
 * \endcode
 *
 * \param[out] node  The node to be created.
 * \param[in] type  The type of node (break or continue).
 */
void Parser::break_continue(Node::pointer_t& node, Node::node_t type)
{
    node = f_lexer->get_new_node(type);

    if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
    {
        node->set_string(f_node->get_string());
        get_token();
    }
    else if(f_node->get_type() == Node::node_t::NODE_DEFAULT)
    {
        // default is equivalent to no label
        get_token();
    }

    if(f_node->get_type() != Node::node_t::NODE_SEMICOLON)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_LABEL, f_lexer->get_input()->get_position());
        msg << "'break' and 'continue' can be followed by one label only";
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER CASE  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::case_directive(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_CASE);
    Node::pointer_t expr;
    expression(expr);
    node->append_child(expr);

    if(f_options
    && f_options->get_option(Options::option_t::OPTION_EXTENDED_STATEMENTS) != 0)
    {
        // check for 'case <expr> ... <expr>:'
        if(f_node->get_type() == Node::node_t::NODE_REST
        || f_node->get_type() == Node::node_t::NODE_RANGE)
        {
            get_token();
            Node::pointer_t expr_to;
            expression(expr_to);
            node->append_child(expr_to);
        }
    }

    if(f_node->get_type() == Node::node_t::NODE_COLON)
    {
        get_token();
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CASE_LABEL, f_lexer->get_input()->get_position());
        msg << "case expression expected to be followed by ':'";
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER CATCH  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::catch_directive(Node::pointer_t& node)
{
    if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
    {
        node = f_lexer->get_new_node(Node::node_t::NODE_CATCH);
        get_token();
        Node::pointer_t parameters;
        bool unused;
        parameter_list(parameters, unused);
        node->append_child(parameters);
        // we want exactly ONE parameter
        size_t const count(parameters->get_children_size());
        if(count != 1)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CATCH, f_lexer->get_input()->get_position());
            msg << "the 'catch' keyword expects exactly one parameter";
        }
        else
        {
            // There is just one parameter, make sure there
            // is no initializer
            bool has_type = false;
            Node::pointer_t param(parameters->get_child(0));
            size_t idx(param->get_children_size());
            while(idx > 0)
            {
                --idx;
                if(param->get_child(idx)->get_type() == Node::node_t::NODE_SET)
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CATCH, f_lexer->get_input()->get_position());
                    msg << "'catch' parameters do not support initializers";
                    break;
                }
                has_type = true;
            }
            if(has_type)
            {
                node->set_flag(Node::flag_t::NODE_CATCH_FLAG_TYPED, true);
            }
        }
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
            if(f_node->get_type() == Node::node_t::NODE_OPEN_CURVLY_BRACKET)
            {
                get_token();
                Node::pointer_t one_block;
                block(one_block);
                node->append_child(one_block);
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "'{' expected after the 'catch' parameter";
            }
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "')' expected to end the 'catch' parameter list";
        }
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'(' expected after the 'catch' keyword";
    }
}






/**********************************************************************/
/**********************************************************************/
/***  PARSER DEFAULT  *************************************************/
/**********************************************************************/
/**********************************************************************/

// NOTE: if default wasn't a keyword, then it could be used as a
//       label like any user label!
//
//       The fact that it is a keyword allows us to forbid default with
//       the goto instruction without having to do any extra work.
//
void Parser::default_directive(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_DEFAULT);

    // default is just itself!
    if(f_node->get_type() == Node::node_t::NODE_COLON)
    {
        get_token();
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DEFAULT_LABEL, f_lexer->get_input()->get_position());
        msg << "default label expected to be followed by ':'";
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER DO  ******************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::do_directive(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_DO);

    Node::pointer_t one_directive;
    directive(one_directive);
    node->append_child(one_directive);

    if(f_node->get_type() == Node::node_t::NODE_WHILE)
    {
        get_token();
        if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
        {
            get_token();
            Node::pointer_t expr;
            expression(expr);
            node->append_child(expr);
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "')' expected to end the 'while' expression";
            }
            else
            {
                get_token();
            }
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'(' expected after the 'while' keyword";
        }
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_DO, f_lexer->get_input()->get_position());
        msg << "'while' expected after the block of a 'do' keyword";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER FOR  *****************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::for_directive(Node::pointer_t& node)
{
    // for each(...)
    bool const for_each(f_node->get_type() == Node::node_t::NODE_IDENTIFIER
                     && f_node->get_string() == "each");
    if(for_each)
    {
        get_token(); // skip the 'each' "keyword"
    }
    if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
    {
        node = f_lexer->get_new_node(Node::node_t::NODE_FOR);

        bool got_for_in(false);
        get_token(); // skip the '('
        if(f_node->get_type() == Node::node_t::NODE_CONST
        || f_node->get_type() == Node::node_t::NODE_VAR)
        {
            // *** VARIABLE ***
            bool const constant(f_node->get_type() == Node::node_t::NODE_CONST);
            if(constant)
            {
                get_token(); // skip the 'const'
                if(f_node->get_type() == Node::node_t::NODE_VAR)
                {
                    // allow just 'const' or 'const var'
                    get_token(); // skip the 'var'
                }
            }
            else
            {
                get_token(); // skip the 'var'
            }
            Node::pointer_t variables;
            variable(variables, constant);
            node->append_child(variables);
        }
        else if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
        {
            // When we have ';' we have got an empty initializer!
            Node::pointer_t empty(f_lexer->get_new_node(Node::node_t::NODE_EMPTY));
            node->append_child(empty);
        }
        else /*if(f_node->get_type() != Node::node_t::NODE_SEMICOLON)*/
        {
            // *** EXPRESSION ***
            Node::pointer_t expr;
            expression(expr);
            if(f_node->get_type() != Node::node_t::NODE_SEMICOLON)
            {
                if(f_node->get_type() == Node::node_t::NODE_IN)
                {
                    Node::pointer_t left(expr->get_child(0));
                    Node::pointer_t right(expr->get_child(1));
                    expr->delete_child(0);
                    expr->delete_child(0);
                    node->append_child(left);
                    node->append_child(right);
                    got_for_in = true;
                }
            }
            else
            {
                node->append_child(expr);
            }
        }

        if(!got_for_in)
        {
            // This can happen when we return from the
            // variable() function
            if(f_node->get_type() == Node::node_t::NODE_IN)
            {
                // *** IN ***
                get_token();
                Node::pointer_t expr;
                expression(expr);
                node->append_child(expr);
            }
            else if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
            {
                // *** SECOND EXPRESSION ***
                get_token();
                Node::pointer_t expr;
                if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
                {
                    // empty expression
                    expr = f_lexer->get_new_node(Node::node_t::NODE_EMPTY);
                }
                else
                {
                    expression(expr);
                }
                node->append_child(expr);
                if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
                {
                    // *** THIRD EXPRESSION ***
                    get_token();
                    Node::pointer_t thrid_expr;
                    if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
                    {
                        thrid_expr = f_lexer->get_new_node(Node::node_t::NODE_EMPTY);
                    }
                    else
                    {
                        expression(thrid_expr);
                    }
                    node->append_child(thrid_expr);
                }
                else
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_input()->get_position());
                    msg << "';' expected between the last two 'for' expressions";
                }
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_input()->get_position());
                msg << "';' or 'in' expected between the 'for' expressions";
            }
        }

        if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "')' expected to close the 'for' expressions";
        }
        else
        {
            get_token();
        }

        if(node->get_children_size() == 2 && for_each)
        {
            node->set_flag(Node::flag_t::NODE_FOR_FLAG_FOREACH, true);
        }
        else if(for_each)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'for each()' only available with an enumeration for";
        }

        // *** DIRECTIVES ***
        Node::pointer_t one_directive;
        directive(one_directive);
        node->append_child(one_directive);
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'(' expected following the 'for' directive";
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER GOTO  ****************************************************/
/**********************************************************************/
/**********************************************************************/

// although JavaScript does not support a goto directive, we support it
// in the parser; however, the compiler will reject it if you try to
// output the result to JavaScript for browsers.
void Parser::goto_directive(Node::pointer_t& node)
{
    if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
    {
        // save the label
        node = f_node;
        get_token();
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_GOTO, f_lexer->get_input()->get_position());
        msg << "'goto' expects a label as parameter";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER IF  ******************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::if_directive(Node::pointer_t& node)
{
    if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
    {
        node = f_lexer->get_new_node(Node::node_t::NODE_IF);
        get_token();
        Node::pointer_t expr;
        expression(expr);
        node->append_child(expr);
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "')' expected to end the 'if' expression";
        }

        // IF part
        Node::pointer_t one_directive;
        directive(one_directive);
        node->append_child(one_directive);

        // Note that this is the only place where ELSE is permitted!
        if(f_node->get_type() == Node::node_t::NODE_ELSE)
        {
            get_token();
            // ELSE part
            Node::pointer_t else_directive;
            directive(else_directive);
            node->append_child(else_directive);
        }
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'(' expected after the 'if' keyword";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER RETURN  **************************************************/
/**********************************************************************/
/**********************************************************************/



void Parser::return_directive(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_RETURN);
    if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
    {
        Node::pointer_t expr;
        expression(expr);
        node->append_child(expr);
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER TRY & FINALLY  *******************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::try_finally(Node::pointer_t& node, Node::node_t type)
{
    if(f_node->get_type() == Node::node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();
        node = f_lexer->get_new_node(type);
        Node::pointer_t one_block;
        block(one_block);
        node->append_child(one_block);
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'{' expected after the '" << (type == Node::node_t::NODE_TRY ? "try" : "finally") << "' keyword";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER SWITCH  **************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::switch_directive(Node::pointer_t& node)
{
    bool has_open;

    if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
    {
        node = f_lexer->get_new_node(Node::node_t::NODE_SWITCH);

        // a default comparison is important to support ranges properly
        //node->set_switch_operator(Node::node_t::NODE_UNKNOWN); -- this is the default

        get_token();
        Node::pointer_t expr;
        expression(expr);
        node->append_child(expr);
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "')' expected to end the 'switch' expression";
        }
        if(f_node->get_type() == Node::node_t::NODE_WITH)
        {
            get_token();
            has_open = f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS;
            if(has_open)
            {
                get_token();
            }
            switch(f_node->get_type())
            {
            // equality
            case Node::node_t::NODE_STRICTLY_EQUAL:
            case Node::node_t::NODE_EQUAL:
            case Node::node_t::NODE_NOT_EQUAL:
            case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
            // relational
            case Node::node_t::NODE_MATCH:
            case Node::node_t::NODE_IN:
            case Node::node_t::NODE_IS:
            case Node::node_t::NODE_AS:
            case Node::node_t::NODE_INSTANCEOF:
            case Node::node_t::NODE_LESS:
            case Node::node_t::NODE_LESS_EQUAL:
            case Node::node_t::NODE_GREATER:
            case Node::node_t::NODE_GREATER_EQUAL:
            // so the user can specify the default too
            case Node::node_t::NODE_DEFAULT:
                node->set_switch_operator(f_node->get_type());
                get_token();
                break;

            default:
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "insupported operator for a 'switch() with()' expression";
            }
                break;

            }
            if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                get_token();
                if(!has_open)
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
                    msg << "'(' was expected to start the 'switch() with()' expression";
                }
            }
            else if(has_open)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "')' expected to end the 'switch() with()' expression";
            }
        }
        Node::pointer_t attr_list;
        attributes(attr_list);
        if(attr_list->get_children_size() > 0)
        {
            node->set_link(Node::link_t::LINK_ATTRIBUTES, attr_list);
        }
        if(f_node->get_type() == Node::node_t::NODE_OPEN_CURVLY_BRACKET)
        {
            get_token();
            Node::pointer_t one_block;
            block(one_block);
            node->append_child(one_block);
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'{' expected after the 'switch' expression";
        }
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'(' expected after the 'switch' keyword";
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER THROW  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::throw_directive(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_THROW);

    Node::pointer_t expr;
    expression(expr);
    node->append_child(expr);
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER WITH & WHILE  ********************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::with_while(Node::pointer_t& node, Node::node_t type)
{
    char const *inst = type == Node::node_t::NODE_WITH ? "with" : "while";

    if(type == Node::node_t::NODE_WITH
    && (!f_options || f_options->get_option(Options::option_t::OPTION_ALLOW_WITH) != 0))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "')' expected to end the '" << inst << "' expression";
    }

    if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
    {
        node = f_lexer->get_new_node(type);
        get_token();
        Node::pointer_t expr;
        expression(expr);
        node->append_child(expr);
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "')' expected to end the '" << inst << "' expression";
        }
        Node::pointer_t one_directive;
        directive(one_directive);
        node->append_child(one_directive);
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
        msg << "'(' expected after the '" << inst << "' keyword";
    }
}






}
// namespace as2js

// vim: ts=4 sw=4 et
