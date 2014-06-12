/* parser_function.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include "as2js/parser.h"
#include "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER FUNCTION  ************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::parameter_list(Node::pointer_t& node, bool& has_out)
{
    has_out = false;

    // accept function stuff(void) { ... } as in C/C++
    // XXX: determine whether the identifier should be checked case insensitively
    //      (because just "Void" seems strange to me now)
    if(f_node->get_type() == Node::node_t::NODE_VOID
    || (f_node->get_type() == Node::node_t::NODE_IDENTIFIER && f_node->get_string() == "Void"))
    {
        get_token();
        return;
    }

    node = f_lexer->get_new_node(Node::node_t::NODE_PARAMETERS);

    // special case which explicitly says that a function definition
    // is not prototyped (vs. an empty list of parameters which is
    // equivalent to a (void)); this means the function accepts
    // parameters, their type & number are just not defined
    if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER
    && f_node->get_string() == "unprototyped")
    {
        Node::pointer_t param(f_lexer->get_new_node(Node::node_t::NODE_PARAM));
        param->set_flag(Node::flag_t::NODE_PARAM_FLAG_UNPROTOTYPED, true);
        node->append_child(param);
        get_token();
        return;
    }

    bool invalid(false);
    for(;;)
    {
        Node::pointer_t param(f_lexer->get_new_node(Node::node_t::NODE_PARAM));

        // get all the attributes for the parameters
        // (var, const, in, out, named, unchecked, ...)
        bool more(true);
        do
        {
            // TODO: it seems that any one flag should only be accepted
            //       once, 'var' first, and '...' last.
            switch(f_node->get_type())
            {
            case Node::node_t::NODE_REST:
                param->set_flag(Node::flag_t::NODE_PARAM_FLAG_REST, true);
                invalid = false;
                get_token();
                break;

            case Node::node_t::NODE_CONST:
                param->set_flag(Node::flag_t::NODE_PARAM_FLAG_CONST, true);
                invalid = false;
                get_token();
                break;

            case Node::node_t::NODE_IN:
                param->set_flag(Node::flag_t::NODE_PARAM_FLAG_IN, true);
                invalid = false;
                get_token();
                break;

            case Node::node_t::NODE_VAR:
                // TBD: should this be forced first?
                invalid = false;
                get_token();
                break;

            case Node::node_t::NODE_IDENTIFIER:
                if(f_node->get_string() == "out")
                {
                    param->set_flag(Node::flag_t::NODE_PARAM_FLAG_OUT, true);
                    invalid = false;
                    get_token();
                    has_out = true; // for caller to know
                    break;
                }
                if(f_node->get_string() == "named")
                {
                    param->set_flag(Node::flag_t::NODE_PARAM_FLAG_NAMED, true);
                    invalid = false;
                    get_token();
                    break;
                }
                if(f_node->get_string() == "unchecked")
                {
                    param->set_flag(Node::flag_t::NODE_PARAM_FLAG_UNCHECKED, true);
                    invalid = false;
                    get_token();
                    break;
                }
                /*FALLTHROUGH*/
            default:
                more = false;
                break;

            }
        }
        while(more); 

        if(has_out)
        {
            if(param->get_flag(Node::flag_t::NODE_PARAM_FLAG_REST))
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_input()->get_position());
                msg << "you cannot use the function parameter attribute 'out' with '...'";
            }
            if(param->get_flag(Node::flag_t::NODE_PARAM_FLAG_CONST))
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_input()->get_position());
                msg << "you cannot use the function attributes 'out' and 'const' together";
            }
        }

        if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
        {
            param->set_string(f_node->get_string());
            node->append_child(param);
            invalid = false;
            get_token();
            if(f_node->get_type() == Node::node_t::NODE_COLON)
            {
                // TBD: what about REST? does this mean all
                //      the following parameters need to be
                //      of that type?
                get_token();
                Node::pointer_t type;
                conditional_expression(type, false);
                param->append_child(type);
            }
            if(f_node->get_type() == Node::node_t::NODE_ASSIGNMENT)
            {
                // cannot accept when REST is set
                if(f_node->get_flag(Node::flag_t::NODE_PARAM_FLAG_REST))
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_input()->get_position());
                    msg << "you cannot assign a default value to '...'";
                }
                else
                {
                    // initializer
                    get_token();
                    Node::pointer_t initializer(f_lexer->get_new_node(Node::node_t::NODE_SET));
                    Node::pointer_t expr;
                    conditional_expression(expr, false);
                    initializer->append_child(expr);
                    param->append_child(initializer);
                }
            }
        }
        else if(f_node->get_flag(Node::flag_t::NODE_PARAM_FLAG_REST))
        {
            node->append_child(param);
        }

        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            return;
        }
        else if(f_node->get_type() != Node::node_t::NODE_COMMA)
        {
            if(!invalid)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_input()->get_position());
                msg << "expected an identifier as the parameter name (not token " << f_node->get_type_name() << ")";
            }
            switch(f_node->get_type())
            {
            case Node::node_t::NODE_EOF:
            case Node::node_t::NODE_SEMICOLON:
            case Node::node_t::NODE_OPEN_CURVLY_BRACKET:
            case Node::node_t::NODE_CLOSE_CURVLY_BRACKET:
                return;

            default:
                // continue, just ignore that token
                break;

            }
            if(invalid)
            {
                get_token();
            }
            invalid = true;
        }
        else
        {
            if(f_node->get_flag(Node::flag_t::NODE_PARAM_FLAG_REST))
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_PARAMETERS, f_lexer->get_input()->get_position());
                msg << "no other parameters expected after '...'";
            }
            get_token();
        }
    }
}



void Parser::function(Node::pointer_t& node, bool const expression_function)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_FUNCTION);

    switch(f_node->get_type())
    {
    case Node::node_t::NODE_IDENTIFIER:
    {
        String etter;
        if(f_node->get_string() == "get")
        {
            // *** GETTER ***
            node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_GETTER, true);
            etter = "->";
        }
        else if(f_node->get_string() == "set")
        {
            // *** SETTER ***
            node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_SETTER, true);
            etter = "<-";
        }
        if(!etter.empty())
        {
            // *** one of GETTER/SETTER ***
            get_token();
            if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
            {
                node->set_string(String(etter + f_node->get_string()));
                get_token();
            }
            else if(f_node->get_type() == Node::node_t::NODE_STRING)
            {
                // this is an extension, you can't have
                // a getter or setter which is also an
                // operator overload though...
                node->set_string(etter + f_node->get_string());
                if(Node::string_to_operator(node->get_string()) != Node::node_t::NODE_UNKNOWN)
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_input()->get_position());
                    msg << "operator override cannot be marked as a getter nor a setter function";
                }
                get_token();
            }
            else if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
            {
                // not a getter or setter when only get() or set()
                if(f_node->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_GETTER))
                {
                    node->set_string("get");
                }
                else
                {
                    node->set_string("set");
                }
                f_node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_GETTER, false);
                f_node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_SETTER, false);
                etter = "";
            }
            else if(!expression_function)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_input()->get_position());
                msg << "getter and setter functions require a name";
            }
            if(expression_function && !etter.empty())
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_input()->get_position());
                msg << "expression functions cannot be getter nor setter functions";
            }
        }
        else
        {
            // *** STANDARD ***
            node->set_string(f_node->get_string());
            get_token();
            if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
            {
                // Ooops? this could be that the user misspelled get or set
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_input()->get_position());
                msg << "only one name is expected for a function (misspelled get or set?)";
                get_token();
            }
        }
    }
        break;

    case Node::node_t::NODE_STRING:
    {
        // *** OPERATOR OVERLOAD ***
        // (though we just accept any string at this time)
        node->set_string(f_node->get_string());
        if(Node::string_to_operator(node->get_string()) != Node::node_t::NODE_UNKNOWN)
        {
            node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
        }
        get_token();
    }
        break;

    // all the operators which can be overloaded as is
    case Node::node_t::NODE_LOGICAL_NOT:
    case Node::node_t::NODE_MODULO:
    case Node::node_t::NODE_BITWISE_AND:
    case Node::node_t::NODE_MULTIPLY:
    case Node::node_t::NODE_ADD:
    case Node::node_t::NODE_SUBTRACT:
    case Node::node_t::NODE_DIVIDE:
    case Node::node_t::NODE_LESS:
    case Node::node_t::NODE_ASSIGNMENT:
    case Node::node_t::NODE_GREATER:
    case Node::node_t::NODE_BITWISE_XOR:
    case Node::node_t::NODE_BITWISE_OR:
    case Node::node_t::NODE_BITWISE_NOT:
    case Node::node_t::NODE_ASSIGNMENT_ADD:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::node_t::NODE_ASSIGNMENT_DIVIDE:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MINIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MODULO:
    case Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
    case Node::node_t::NODE_ASSIGNMENT_POWER:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
    case Node::node_t::NODE_DECREMENT:
    case Node::node_t::NODE_EQUAL:
    case Node::node_t::NODE_GREATER_EQUAL:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_LESS_EQUAL:
    case Node::node_t::NODE_LOGICAL_AND:
    case Node::node_t::NODE_LOGICAL_OR:
    case Node::node_t::NODE_LOGICAL_XOR:
    case Node::node_t::NODE_MATCH:
    case Node::node_t::NODE_MAXIMUM:
    case Node::node_t::NODE_MINIMUM:
    case Node::node_t::NODE_NOT_EQUAL:
    case Node::node_t::NODE_POST_DECREMENT:
    case Node::node_t::NODE_POST_INCREMENT:
    case Node::node_t::NODE_POWER:
    case Node::node_t::NODE_ROTATE_LEFT:
    case Node::node_t::NODE_ROTATE_RIGHT:
    case Node::node_t::NODE_SHIFT_LEFT:
    case Node::node_t::NODE_SHIFT_RIGHT:
    case Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_STRICTLY_EQUAL:
    case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
    {
        // save the operator type in the node to be able
        // to get the string
        node->set_string(Node::operator_to_string(f_node->get_type()));
        node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
        get_token();
    }
        break;

    // this is a complicated one because () can
    // be used for the parameters too
    case Node::node_t::NODE_OPEN_PARENTHESIS:
    {
        Node::pointer_t restore(f_node);
        get_token();
        if(f_node->get_type() == Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            Node::pointer_t save(f_node);
            get_token();
            if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
            {
                // at this point...
                // this is taken as the "()" operator!
                node->set_string("()");
                node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR, true);
            }
            else
            {
                unget_token(f_node);
                unget_token(save);
                f_node = restore;
            }
        }
        else
        {
            unget_token(f_node);
            f_node = restore;
        }
    }
        break;

    default:
        if(!expression_function)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_FUNCTION, f_lexer->get_input()->get_position());
            msg << "function declarations are required to be named";
        }
        break;

    }

    if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
    {
        get_token();
        if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
        {
            // read params
            Node::pointer_t params;
            bool has_out;
            parameter_list(params, has_out);
            if(has_out)
            {
                node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_OUT, true);
            }
            if(params)
            {
                node->append_child(params);
            }
            else
            {
                node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_NOPARAMS, true);
            }
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PARENTHESIS_EXPECTED, f_lexer->get_input()->get_position());
                msg << "')' expected to close the 'function' parameters";
            }
            else
            {
                get_token();
            }
        }
        else
        {
            get_token();
        }
    }

    // return type specified?
    if(f_node->get_type() == Node::node_t::NODE_COLON)
    {
        get_token();
        if(f_node->get_type() == Node::node_t::NODE_VOID
        || (f_node->get_type() == Node::node_t::NODE_IDENTIFIER && f_node->get_string() == "Void"))
        {
            // special case of a procedure instead of a function
            node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_VOID, true);
            get_token();
        }
        else if(f_node->get_type() == Node::node_t::NODE_VOID && f_node->get_string() == "Never")
        {
            // function is not expected to return
            node->set_flag(Node::flag_t::NODE_FUNCTION_FLAG_NEVER, true);
            get_token();
        }
        else
        {
            // normal type definition
            Node::pointer_t expr;
            conditional_expression(expr, false);
            node->append_child(expr);
        }
    }

    if(f_node->get_type() == Node::node_t::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();
        if(f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            Node::pointer_t statements;
            directive_list(statements);
            node->append_child(statements);
        }
        // else ... nothing?!
        // NOTE: by not inserting anything when we have
        //       an empty definition, it looks like an abstract
        //       definition... we may want to change that at a
        //       later time.
        if(f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRAKETS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'}' expected to close the 'function' block";
        }
        else
        {
            get_token();
        }
    }
    // empty function (a.k.a abstract or function as a type)
    // such functions are permitted in interfaces!
}



}
// namespace as2js

// vim: ts=4 sw=4 et
