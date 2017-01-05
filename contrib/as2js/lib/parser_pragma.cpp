/* parser_pragma.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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
/***  PARSER PRAGMA  **************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::pragma()
{
    while(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
    {
        String const name(f_node->get_string());
        Node::pointer_t argument;
        get_token();
        if(f_node->get_type() == Node::node_t::NODE_OPEN_PARENTHESIS)
        {
            // has zero or one argument
            get_token();
            // accept an empty argument '()'
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                bool const negative(f_node->get_type() == Node::node_t::NODE_SUBTRACT);
                if(negative)
                {
                    // skip the '-' sign
                    get_token();
                }
                // TODO: add support for 'positive'?
                switch(f_node->get_type())
                {
                case Node::node_t::NODE_FALSE:
                case Node::node_t::NODE_STRING:
                case Node::node_t::NODE_TRUE:
                    if(negative)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_input()->get_position());
                        msg << "invalid negative argument for a pragma.";
                    }
                    argument = f_node;
                    get_token();
                    break;

                case Node::node_t::NODE_FLOAT64:
                    argument = f_node;
                    if(negative)
                    {
                        argument->set_float64(-argument->get_float64().get());
                    }
                    get_token();
                    break;

                case Node::node_t::NODE_INT64:
                    argument = f_node;
                    if(negative)
                    {
                        argument->set_int64(-argument->get_int64().get());
                    }
                    get_token();
                    break;

                case Node::node_t::NODE_CLOSE_PARENTHESIS:
                    if(negative)
                    {
                        // we cannot negate "nothingness"
                        // (i.e. use blah(-); is not valid)
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_input()->get_position());
                        msg << "a pragma argument cannot just be '-'.";
                    }
                    break;

                default:
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_input()->get_position());
                    msg << "invalid argument type for a pragma.";
                }
                    break;

                }
            }
            if(f_node->get_type() != Node::node_t::NODE_CLOSE_PARENTHESIS)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_input()->get_position());
                msg << "invalid argument for a pragma.";
            }
            else
            {
                get_token();
            }
        }
        bool const prima(f_node->get_type() == Node::node_t::NODE_CONDITIONAL);
        if(prima)
        {
            // skip the '?'
            get_token();
        }

        // Check out this pragma. We have the following
        // info about each pragma:
        //
        //    name        The pragma name
        //    argument    The pragma argument (unknown by default)
        //    prima       True if pragma name followed by '?'
        //
        // NOTE: pragmas that we do not recognize are simply
        //       being ignored.
        //
        Options::option_value_t value(1);
        Options::option_t option = Options::option_t::OPTION_UNKNOWN;
        if(name == "allow_with")
        {
            option = Options::option_t::OPTION_ALLOW_WITH;
        }
        else if(name == "no_allow_with")
        {
            option = Options::option_t::OPTION_ALLOW_WITH;
            value = 0;
        }
        else if(name == "binary")
        {
            option = Options::option_t::OPTION_BINARY;
        }
        else if(name == "no_binary")
        {
            option = Options::option_t::OPTION_BINARY;
            value = 0;
        }
        else if(name == "coverage")
        {
            option = Options::option_t::OPTION_COVERAGE;
        }
        else if(name == "no_coverage")
        {
            option = Options::option_t::OPTION_COVERAGE;
            value = 0;
        }
        else if(name == "debug")
        {
            option = Options::option_t::OPTION_DEBUG;
        }
        else if(name == "no_debug")
        {
            option = Options::option_t::OPTION_DEBUG;
            value = 0;
        }
        else if(name == "extended_escape_sequences")
        {
            option = Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES;
        }
        else if(name == "no_extended_escape_sequences")
        {
            option = Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES;
            value = 0;
        }
        else if(name == "extended_operators")
        {
            option = Options::option_t::OPTION_EXTENDED_OPERATORS;
        }
        else if(name == "no_extended_operators")
        {
            option = Options::option_t::OPTION_EXTENDED_OPERATORS;
            value = 0;
        }
        else if(name == "extended_statements")
        {
            option = Options::option_t::OPTION_EXTENDED_STATEMENTS;
        }
        else if(name == "no_extended_statements")
        {
            option = Options::option_t::OPTION_EXTENDED_STATEMENTS;
            value = 0;
        }
        else if(name == "octal")
        {
            option = Options::option_t::OPTION_OCTAL;
        }
        else if(name == "no_octal")
        {
            option = Options::option_t::OPTION_OCTAL;
            value = 0;
        }
        else if(name == "strict")
        {
            option = Options::option_t::OPTION_STRICT;
        }
        else if(name == "no_strict")
        {
            option = Options::option_t::OPTION_STRICT;
            value = 0;
        }
        else if(name == "trace")
        {
            option = Options::option_t::OPTION_TRACE;
        }
        else if(name == "no_trace")
        {
            option = Options::option_t::OPTION_TRACE;
            value = 0;
        }
        else if(name == "unsafe_math")
        {
            option = Options::option_t::OPTION_UNSAFE_MATH;
        }
        else if(name == "no_unsafe_math")
        {
            option = Options::option_t::OPTION_UNSAFE_MATH;
            value = 0;
        }
        if(option != Options::option_t::OPTION_UNKNOWN)
        {
            pragma_option(option, prima, argument, value);
        }

        if(f_node->get_type() == Node::node_t::NODE_COMMA)
        {
            get_token();
        }
        else if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_input()->get_position());
            msg << "pragmas must be separated by commas.";
        }
        else if(f_node->get_type() != Node::node_t::NODE_SEMICOLON)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_BAD_PRAGMA, f_lexer->get_input()->get_position());
            msg << "pragmas must be separated by commas and ended by a semicolon.";
            // no need for a break since the while() will exit already
        }
    }
}



void Parser::pragma_option(Options::option_t option, bool prima, Node::pointer_t& argument, Options::option_value_t value)
{
    // user overloaded the value?
    // if argument is a null pointer, then keep the input value as is
    if(argument) switch(argument->get_type())
    {
    case Node::node_t::NODE_TRUE:
        value = 1;
        break;

    case Node::node_t::NODE_INT64:
        value = argument->get_int64().get();
        break;

    case Node::node_t::NODE_FLOAT64:
        // should we round up instead of using floor()?
        value = static_cast<Options::option_value_t>(argument->get_float64().get());
        break;

    case Node::node_t::NODE_STRING:
    {
        // TBD: we could try to convert the string, but is that really
        //      necessary?
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INCOMPATIBLE_PRAGMA_ARGUMENT, f_lexer->get_input()->get_position());
        msg << "incompatible pragma argument.";
    }
        break;

    default: // Node::node_t::NODE_FALSE
        value = 0;
        break;

    }

    if(prima)
    {
        if(f_options->get_option(option) != value)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_PRAGMA_FAILED, f_lexer->get_input()->get_position());
            msg << "prima pragma failed.";
        }
        return;
    }

    f_options->set_option(option, value);
}





}
// namespace as2js

// vim: ts=4 sw=4 et
