/* parser_expression.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include "as2js/parser.h"
#include "as2js/exceptions.h"
#include "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER EXPRESSION  **********************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::expression(Node::pointer_t& node)
{
    list_expression(node, false, false);

    if(!node)
    {
        // should not happen, if it does, we have got a really bad internal error
        throw exception_internal_error("expression() cannot return a null node pointer"); // LCOV_EXCL_LINE
    }
}


void Parser::list_expression(Node::pointer_t& node, bool rest, bool empty)
{
    if(node)
    {
        // should not happen, if it does, we have got a really bad internal error
        throw exception_internal_error("list_expression() called with a non-null node pointer"); // LCOV_EXCL_LINE
    }

    int has_rest(0);
    if(empty && f_node->get_type() == Node::node_t::NODE_COMMA)
    {
        // empty at the start of the array
        node = f_lexer->get_new_node(Node::node_t::NODE_EMPTY);
    }
    else if(rest && f_node->get_type() == Node::node_t::NODE_REST)
    {
        // the '...' in a function call is used to mean pass
        // my own rest down to the callee
        node = f_lexer->get_new_node(Node::node_t::NODE_REST);
        get_token();
        has_rest = 1;
        // note: we expect ')' here but we
        // let the user put ',' <expr> still
        // and err in case it happens
    }
    else if(rest && f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
    {
        // identifiers ':' -> named parameter
        Node::pointer_t save(f_node);
        // skip the identifier
        get_token();
        if(f_node->get_type() == Node::node_t::NODE_COLON)
        {
            // skip the ':'
            get_token();
            node = f_lexer->get_new_node(Node::node_t::NODE_NAME);
            node->set_string(save->get_string());
            if(f_node->get_type() == Node::node_t::NODE_REST)
            {
                // the '...' in a function call is used to mean pass
                // my own rest down to the callee
                Node::pointer_t rest_of_args(f_lexer->get_new_node(Node::node_t::NODE_REST));
                node->append_child(rest_of_args);
                get_token();
                has_rest = 1;
                // note: we expect ')' here but we
                // let the user put ',' <expr> still
                // and err in case it happens
            }
            else
            {
                Node::pointer_t value;
                assignment_expression(value);
                node->append_child(value);
            }
        }
        else
        {
            unget_token(f_node);
            f_node = save;
            assignment_expression(node);
        }
    }
    else
    {
        assignment_expression(node);
    }

    if(f_node->get_type() == Node::node_t::NODE_COMMA)
    {
        Node::pointer_t first_item(node);

        node = f_lexer->get_new_node(Node::node_t::NODE_LIST);
        node->append_child(first_item);

        while(f_node->get_type() == Node::node_t::NODE_COMMA)
        {
            get_token();
            if(has_rest == 1)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_REST, f_lexer->get_input()->get_position());
                msg << "'...' was expected to be the last expression in this function call.";
                has_rest = 2;
            }
            if(empty && f_node->get_type() == Node::node_t::NODE_COMMA)
            {
                // empty inside the array
                Node::pointer_t empty_node(f_lexer->get_new_node(Node::node_t::NODE_EMPTY));
                node->append_child(empty_node);
            }
            else if(empty && f_node->get_type() == Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                // empty at the end of the array
                Node::pointer_t empty_node(f_lexer->get_new_node(Node::node_t::NODE_EMPTY));
                node->append_child(empty_node);
            }
            else if(rest && f_node->get_type() == Node::node_t::NODE_REST)
            {
                // the '...' in a function call is used to mean pass
                // my own rest down to the callee
                Node::pointer_t rest_node(f_lexer->get_new_node(Node::node_t::NODE_REST));
                node->append_child(rest_node);
                get_token();
                if(has_rest == 0)
                {
                    has_rest = 1;
                }
                // note: we expect ')' here but we
                // let the user put ',' <expr> still
                // and err in case it happens
            }
            else if(rest && f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
            {
                Node::pointer_t item;

                // identifiers ':' -> named parameter
                Node::pointer_t save(f_node);
                get_token();
                if(f_node->get_type() == Node::node_t::NODE_COLON)
                {
                    get_token();
                    item = f_lexer->get_new_node(Node::node_t::NODE_NAME);
                    item->set_string(save->get_string());

                    if(f_node->get_type() == Node::node_t::NODE_REST)
                    {
                        // the '...' in a function call is used to mean pass
                        // my own rest down to the callee
                        Node::pointer_t rest_of_args(f_lexer->get_new_node(Node::node_t::NODE_REST));
                        item->append_child(rest_of_args);
                        get_token();
                        if(has_rest == 0)
                        {
                            has_rest = 1;
                        }
                        // note: we expect ')' here but we
                        // let the user put ',' <expr> still
                        // and err in case it happens
                    }
                    else
                    {
                        Node::pointer_t value;
                        assignment_expression(value);
                        item->append_child(value);
                    }
                    node->append_child(item);
                }
                else
                {
                    unget_token(f_node);
                    f_node = save;
                    assignment_expression(item);
                    node->append_child(item);
                }
            }
            else
            {
                Node::pointer_t item;
                assignment_expression(item);
                node->append_child(item);
            }
        }

        // TODO: check that the list ends with a NODE_REST
    }
}


void Parser::assignment_expression(Node::pointer_t& node)
{
    conditional_expression(node, true);

    // TODO: check that the result is a postfix expression
    switch(f_node->get_type())
    {
    case Node::node_t::NODE_ASSIGNMENT:
    case Node::node_t::NODE_ASSIGNMENT_ADD:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::node_t::NODE_ASSIGNMENT_DIVIDE:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::node_t::NODE_ASSIGNMENT_MODULO:
    case Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
        break;

    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MINIMUM:
    case Node::node_t::NODE_ASSIGNMENT_POWER:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
        if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        break;

    default:
        return;

    }

    Node::pointer_t left(node);

    node = f_node;

    get_token();
    Node::pointer_t right;
    assignment_expression(right);

    node->append_child(left);
    node->append_child(right);
}


void Parser::conditional_expression(Node::pointer_t& node, bool const assignment)
{
    min_max_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_CONDITIONAL)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t left;
        // not like C/C++, not a list expression here
        if(assignment)
        {
            assignment_expression(left);
        }
        else
        {
            conditional_expression(left, false);
        }
        node->append_child(left);

        if(f_node->get_type() == Node::node_t::NODE_COLON)
        {
            get_token();
            Node::pointer_t right;
            if(assignment)
            {
                assignment_expression(right);
            }
            else
            {
                conditional_expression(right, false);
            }
            node->append_child(right);
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CONDITIONAL, f_lexer->get_input()->get_position());
            msg << "invalid use of the conditional operator, ':' was expected.";
        }
    }
}



void Parser::min_max_expression(Node::pointer_t& node)
{
    logical_or_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_MINIMUM
    || f_node->get_type() == Node::node_t::NODE_MAXIMUM)
    {
        if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        logical_or_expression(right);
        node->append_child(right);
    }
}


void Parser::logical_or_expression(Node::pointer_t& node)
{
    logical_xor_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_LOGICAL_OR)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        logical_xor_expression(right);
        node->append_child(right);
    }
}


void Parser::logical_xor_expression(Node::pointer_t& node)
{
    logical_and_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_LOGICAL_XOR)
    {
        if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '^^' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        logical_and_expression(right);
        node->append_child(right);
    }
}


void Parser::logical_and_expression(Node::pointer_t& node)
{
    bitwise_or_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_LOGICAL_AND)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        bitwise_or_expression(right);
        node->append_child(right);
    }
}



void Parser::bitwise_or_expression(Node::pointer_t& node)
{
    bitwise_xor_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_BITWISE_OR)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        bitwise_xor_expression(right);
        node->append_child(right);
    }
}


void Parser::bitwise_xor_expression(Node::pointer_t& node)
{
    bitwise_and_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_BITWISE_XOR)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        bitwise_and_expression(right);
        node->append_child(right);
    }
}


void Parser::bitwise_and_expression(Node::pointer_t& node)
{
    equality_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_BITWISE_AND)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        equality_expression(right);
        node->append_child(right);
    }
}


void Parser::equality_expression(Node::pointer_t& node)
{
    relational_expression(node);

    Node::node_t type(f_node->get_type());
    while(type == Node::node_t::NODE_EQUAL
       || type == Node::node_t::NODE_NOT_EQUAL
       || type == Node::node_t::NODE_STRICTLY_EQUAL
       || type == Node::node_t::NODE_STRICTLY_NOT_EQUAL
       || type == Node::node_t::NODE_COMPARE
       || type == Node::node_t::NODE_SMART_MATCH)
    {
        if((type == Node::node_t::NODE_COMPARE || type == Node::node_t::NODE_SMART_MATCH)
        && !has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        relational_expression(right);
        node->append_child(right);

        type = f_node->get_type();
    }
}


void Parser::relational_expression(Node::pointer_t& node)
{
    shift_expression(node);

    while(f_node->get_type() == Node::node_t::NODE_LESS
       || f_node->get_type() == Node::node_t::NODE_GREATER
       || f_node->get_type() == Node::node_t::NODE_LESS_EQUAL
       || f_node->get_type() == Node::node_t::NODE_GREATER_EQUAL
       || f_node->get_type() == Node::node_t::NODE_IS
       || f_node->get_type() == Node::node_t::NODE_AS
       || f_node->get_type() == Node::node_t::NODE_IN
       || f_node->get_type() == Node::node_t::NODE_INSTANCEOF)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        shift_expression(right);
        node->append_child(right);

        // with IN we accept a range (optional)
        if(node->get_type() == Node::node_t::NODE_IN
        && (f_node->get_type() == Node::node_t::NODE_RANGE || f_node->get_type() == Node::node_t::NODE_REST))
        {
            if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
                msg << "the 'x in min .. max' operator is only available when extended operators are authorized (use extended_operators;).";
            }

            get_token();
            Node::pointer_t end;
            shift_expression(end);
            node->append_child(end);
        }
    }
}


void Parser::shift_expression(Node::pointer_t& node)
{
    additive_expression(node);

    Node::node_t type(f_node->get_type());
    while(type == Node::node_t::NODE_SHIFT_LEFT
       || type == Node::node_t::NODE_SHIFT_RIGHT
       || type == Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED
       || type == Node::node_t::NODE_ROTATE_LEFT
       || type == Node::node_t::NODE_ROTATE_RIGHT)
    {
        if((type == Node::node_t::NODE_ROTATE_LEFT || type == Node::node_t::NODE_ROTATE_RIGHT)
        && !has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }

        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        additive_expression(right);
        node->append_child(right);

        type = f_node->get_type();
    }
}


void Parser::additive_expression(Node::pointer_t& node)
{
    multiplicative_expression(node);

    while(f_node->get_type() == Node::node_t::NODE_ADD
       || f_node->get_type() == Node::node_t::NODE_SUBTRACT)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        multiplicative_expression(right);
        node->append_child(right);
    }
}


void Parser::multiplicative_expression(Node::pointer_t& node)
{
    match_expression(node);

    while(f_node->get_type() == Node::node_t::NODE_MULTIPLY
       || f_node->get_type() == Node::node_t::NODE_DIVIDE
       || f_node->get_type() == Node::node_t::NODE_MODULO)
    {
        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        match_expression(right);
        node->append_child(right);
    }
}


void Parser::match_expression(Node::pointer_t& node)
{
    power_expression(node);

    while(f_node->get_type() == Node::node_t::NODE_MATCH
       || f_node->get_type() == Node::node_t::NODE_NOT_MATCH)
    {
        if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }

        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        power_expression(right);
        node->append_child(right);
    }
}


void Parser::power_expression(Node::pointer_t& node)
{
    unary_expression(node);

    if(f_node->get_type() == Node::node_t::NODE_POWER)
    {
        if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '**' operator is only available when extended operators are authorized (use extended_operators;).";
        }

        f_node->append_child(node);
        node = f_node;

        get_token();
        Node::pointer_t right;
        power_expression(right); // right to left
        node->append_child(right);
    }
}



void Parser::unary_expression(Node::pointer_t& node)
{
    if(node)
    {
        throw exception_internal_error("unary_expression() called with a non-null node pointer");  // LCOV_EXCL_LINE
    }

    switch(f_node->get_type())
    {
    case Node::node_t::NODE_DELETE:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_DECREMENT:
    {
        node = f_node;
        get_token();
        Node::pointer_t postfix;
        postfix_expression(postfix);
        node->append_child(postfix);
    }
        break;

    case Node::node_t::NODE_VOID:
    case Node::node_t::NODE_TYPEOF:
    case Node::node_t::NODE_ADD: // +<value>
    case Node::node_t::NODE_SUBTRACT: // -<value>
    case Node::node_t::NODE_BITWISE_NOT:
    case Node::node_t::NODE_LOGICAL_NOT:
    {
        node = f_node;
        get_token();
        Node::pointer_t unary;
        unary_expression(unary);
        node->append_child(unary);
    }
        break;

    case Node::node_t::NODE_SMART_MATCH:
    {
        // we support the ~~ for Smart Match, but if found as a unary
        // operator the user had to mean '~' and '~' separated as in:
        //     a = ~ ~ b
        // so here we generate two bitwise not (DO NOT OPTIMIZE, if one
        // writes a = ~~b it is NOT the same as a = b because JavaScript
        // forces a conversion of b to a 32 bit integer when applying the
        // bitwise not operator.)
        //
        node = f_lexer->get_new_node(Node::node_t::NODE_BITWISE_NOT);
        Node::pointer_t child(f_lexer->get_new_node(Node::node_t::NODE_BITWISE_NOT));
        node->append_child(child);
        get_token();
        Node::pointer_t unary;
        unary_expression(unary);
        child->append_child(unary);
    }
        break;

    case Node::node_t::NODE_NOT_MATCH:
    {
        // we support the !~ for Not Match, but if found as a unary
        // operator the user had to mean '!' and '~' separated as in:
        //     a = ! ~ b
        // so here we generate two not (DO NOT OPTIMIZE, if one
        // writes a = !~b it is NOT the same as a = b because JavaScript
        // forces a conversion of b to a 32 bit integer when applying the
        // bitwise not operator.)
        //
        node = f_lexer->get_new_node(Node::node_t::NODE_LOGICAL_NOT);
        Node::pointer_t child(f_lexer->get_new_node(Node::node_t::NODE_BITWISE_NOT));
        node->append_child(child);
        get_token();
        Node::pointer_t unary;
        unary_expression(unary);
        child->append_child(unary);
    }
        break;

    default:
        postfix_expression(node);
        break;

    }
}


void Parser::postfix_expression(Node::pointer_t& node)
{
    primary_expression(node);

    for(;;)
    {
        switch(f_node->get_type())
        {
        case Node::node_t::NODE_MEMBER:
        {
            f_node->append_child(node);
            node = f_node;

            get_token();
            Node::pointer_t right;
            primary_expression(right);
            node->append_child(right);
        }
            break;

        case Node::node_t::NODE_SCOPE:
        {
            // TBD: I do not think that we need a scope operator at all
            //      since we can use the '.' (MEMBER) operator in all cases
            //      I can currently think of (and in JavaScript you are
            //      expected to do so anyway!) therefore I only authorize
            //      it as an extension at the moment
            if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
                msg << "the '::' operator is only available when extended operators are authorized (use extended_operators;).";
            }

            f_node->append_child(node);
            node = f_node;

            get_token();
            if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
            {
                node->append_child(f_node);
                get_token();
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_SCOPE, f_lexer->get_input()->get_position());
                msg << "scope operator '::' is expected to be followed by an identifier.";
            }
            // don't repeat scope (it seems)
            return;
        }
            break;

        case Node::node_t::NODE_INCREMENT:
        {
            Node::pointer_t decrement(f_lexer->get_new_node(Node::node_t::NODE_POST_INCREMENT));
            decrement->append_child(node);
            node = decrement;
            get_token();
        }
            break;

        case Node::node_t::NODE_DECREMENT:
        {
            Node::pointer_t decrement(f_lexer->get_new_node(Node::node_t::NODE_POST_DECREMENT));
            decrement->append_child(node);
            node = decrement;
            get_token();
        }
            break;

        case Node::node_t::NODE_OPEN_PARENTHESIS:        // function call arguments
        {
            Node::pointer_t left(node);
            node = f_lexer->get_new_node(Node::node_t::NODE_CALL);
            node->append_child(left);

            get_token();

            // any arguments?
            Node::pointer_t right;
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                Node::pointer_t list;
                list_expression(list, true, false);
                if(list->get_type() == Node::node_t::NODE_LIST)
                {
                    // already a list, use it as is
                    right = list;
                }
                else
                {
                    // not a list, so put it in a one
                    right = f_lexer->get_new_node(Node::node_t::NODE_LIST);
                    right->append_child(list);
                }
            }
            else
            {
                // an empty list!
                right = f_lexer->get_new_node(Node::node_t::NODE_LIST);
            }
            node->append_child(right);

            if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                get_token();
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "')' expected to end the list of arguments.";
            }
        }
            break;

        case Node::node_t::NODE_OPEN_SQUARE_BRACKET:        // array/property access
        {
            Node::pointer_t array(f_lexer->get_new_node(Node::node_t::NODE_ARRAY));
            array->append_child(node);
            node = array;

            get_token();

            // any arguments?
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                Node::pointer_t right;
                list_expression(right, false, false);
                node->append_child(right);
            }

            if(f_node->get_type() == Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                get_token();
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SQUARE_BRACKETS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "']' expected to end the list of element references or declarations.";
            }
        }
            break;

        default:
            return;

        }
    }
}


void Parser::primary_expression(Node::pointer_t& node)
{
    switch(f_node->get_type())
    {
    case Node::node_t::NODE_FALSE:
    case Node::node_t::NODE_FLOAT64:
    case Node::node_t::NODE_IDENTIFIER:
    case Node::node_t::NODE_INT64:
    case Node::node_t::NODE_NULL:
    case Node::node_t::NODE_REGULAR_EXPRESSION:
    case Node::node_t::NODE_STRING:
    case Node::node_t::NODE_THIS:
    case Node::node_t::NODE_TRUE:
    case Node::node_t::NODE_UNDEFINED:
    case Node::node_t::NODE_SUPER:
        node = f_node;
        get_token();
        break;

    case Node::node_t::NODE_PRIVATE:
    case Node::node_t::NODE_PROTECTED:
    case Node::node_t::NODE_PUBLIC:
        if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
            msg << "the '" << f_node->get_type_name() << "' operator is only available when extended operators are authorized (use extended_operators;).";
        }
        node = f_node;
        get_token();
        break;

    case Node::node_t::NODE_NEW:
    {
        node = f_node;
        get_token();
        Node::pointer_t object_name;
        postfix_expression(object_name);
        node->append_child(object_name);
    }
        break;

    case Node::node_t::NODE_OPEN_PARENTHESIS:        // grouped expressions
    {
        get_token();
        list_expression(node, false, false);

        // NOTE: the following is important in different cases
        //       such as (a).field which is dynamic (i.e. we get the
        //       content of variable a as the name of the object to
        //       access and thus it is not equivalent to a.field)
        if(node->get_type() == Node::node_t::NODE_IDENTIFIER)
        {
            node->to_videntifier();
        }
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "')' expected to match the '('.";
        }
    }
        break;

    case Node::node_t::NODE_OPEN_SQUARE_BRACKET: // array declaration
    {
        node = f_lexer->get_new_node(Node::node_t::NODE_ARRAY_LITERAL);
        get_token();

        Node::pointer_t elements;
        list_expression(elements, false, true);
        node->append_child(elements);
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SQUARE_BRACKETS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "']' expected to match the '[' of this array.";
        }
    }
        break;

    case Node::node_t::NODE_OPEN_CURVLY_BRACKET: // object declaration
    {
        get_token();
        object_literal_expression(node);
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'}' expected to match the '{' of this object literal.";
        }
    }
        break;

    case Node::node_t::NODE_FUNCTION:
    {
        get_token();
        function(node, true);
    }
        break;

    default:
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, f_lexer->get_input()->get_position());
        msg << "unexpected token '" << f_node->get_type_name() << "' found in an expression.";

        // callers expect to receive a node... give them something
        node = f_lexer->get_new_node(Node::node_t::NODE_FALSE);
        break;

    }
}



void Parser::object_literal_expression(Node::pointer_t& node)
{
    Node::pointer_t name;
    Node::node_t    type;

    node = f_lexer->get_new_node(Node::node_t::NODE_OBJECT_LITERAL);
    for(;;)
    {
        name = f_lexer->get_new_node(Node::node_t::NODE_NAME);
        type = f_node->get_type();
        switch(type)
        {
        case Node::node_t::NODE_OPEN_PARENTHESIS: // (<expr>)::<name> only
        {
            get_token();  // we MUST skip the '(' otherwise the '::' is eaten from within
            Node::pointer_t expr;
            expression(expr);
            if(expr->get_type() == Node::node_t::NODE_IDENTIFIER)
            {
                // an identifier becomes a VIDENTIFIER to remain dynamic.
                expr->to_videntifier();
            }
            name->append_child(expr);
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, f_lexer->get_input()->get_position());
                msg << "')' is expected to close a dynamically named object field.";
            }
            else
            {
                get_token();
            }
        }
            goto and_scope;

        case Node::node_t::NODE_IDENTIFIER:     // <name> or <namespace>::<name>
            // NOTE: an IDENTIFIER here remains NODE_IDENTIFIER
            //       so it does not look like the previous expression
            //       (i.e. an expression literal can be just an
            //       identifier but it will be marked as
            //       NODE_VIDENTIFIER instead)
            name->set_string(f_node->get_string());
            /*FALLTHROUGH*/
        case Node::node_t::NODE_PRIVATE:        // private::<name> only
        case Node::node_t::NODE_PROTECTED:      // protected::<name> only
        case Node::node_t::NODE_PUBLIC:         // public::<name> only
            get_token();
and_scope:
            if(f_node->get_type() == Node::node_t::NODE_SCOPE)
            {
                // TBD: I do not think that we need a scope operator at all
                //      since we can use the '.' (MEMBER) operator in all cases
                //      I can currently think of (and in JavaScript you are
                //      expected to do so anyway!) therefore I only authorize
                //      it as an extension at the moment
                if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_lexer->get_input()->get_position());
                    msg << "the '::' operator is only available when extended operators are authorized (use extended_operators;).";
                }

                get_token();
                if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
                {
                    name->append_child(f_node);
                    get_token();
                }
                else
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_SCOPE, f_lexer->get_input()->get_position());
                    msg << "'::' is expected to always be followed by an identifier.";
                }
            }
            else if(type != Node::node_t::NODE_IDENTIFIER)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD_NAME, f_lexer->get_input()->get_position());
                msg << "'public', 'protected', or 'private' or a dynamic scope cannot be used as a field name, '::' was expected.";
            }
            break;

        case Node::node_t::NODE_INT64:
        case Node::node_t::NODE_FLOAT64:
        case Node::node_t::NODE_STRING:
            name = f_node;
            get_token();
            break;

        default:
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FIELD, f_lexer->get_input()->get_position());
            msg << "the name of a field was expected.";
            break;

        }

        if(f_node->get_type() == Node::node_t::NODE_COLON)
        {
            get_token();
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COLON_EXPECTED, f_lexer->get_input()->get_position());
            msg << "':' expected after the name of a field.";

            // if we have a closing brace here, the programmer
            // tried to end his list improperly; we just
            // accept that one silently! (like in C/C++)
            if(f_node->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET
            || f_node->get_type() == Node::node_t::NODE_SEMICOLON)
            {
                // this is probably the end...
                return;
            }

            // if we have a comma here, the programmer
            // just forgot a few things...
            if(f_node->get_type() == Node::node_t::NODE_COMMA)
            {
                get_token();
                // we accept a comma at the end here too!
                if(f_node->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET
                || f_node->get_type() == Node::node_t::NODE_SEMICOLON)
                {
                    return;
                }
                continue;
            }
        }

        // add the name only now so we have a mostly
        // valid tree from here on
        node->append_child(name);

        Node::pointer_t set(f_lexer->get_new_node(Node::node_t::NODE_SET));
        Node::pointer_t value;
        assignment_expression(value);
        set->append_child(value);
        node->append_child(set);

        // got to the end?
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            return;
        }

        if(f_node->get_type() != Node::node_t::NODE_COMMA)
        {
            if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, f_lexer->get_input()->get_position());
                msg << "'}' expected before the ';' to end an object literal.";
                return;
            }
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, f_lexer->get_input()->get_position());
            msg << "',' or '}' expected after the value of a field.";
        }
        else
        {
            get_token();
        }
    }
    /*NOTREACHED*/
}





}
// namespace as2js

// vim: ts=4 sw=4 et
