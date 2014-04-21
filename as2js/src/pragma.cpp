/* pragma.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

/*

Copyright (c) 2005-2009 Made to Order Software Corp.

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


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER PRAGMA  **************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Pragma(void)
{
    while(f_data.f_type == NODE_IDENTIFIER)
    {
        String name = f_data.f_str;
        Data argument;
        bool prima = false;
        GetToken();
        if(f_data.f_type == '(')
        {
            // has zero or one argument
            GetToken();
            // accept an empty argument '()'
            if(f_data.f_type != ')')
            {
                bool negative = false;
                if(f_data.f_type == '-')
                {
                    GetToken();
                    negative = true;
                }
                switch(f_data.f_type)
                {
                case NODE_FALSE:
                case NODE_STRING:
                case NODE_TRUE:
                    if(negative)
                    {
                        negative = false;
                        f_lexer.ErrMsg(AS_ERR_BAD_PRAGMA, "invalid negative argument for a pragma");
                    }
                    argument = f_data;
                    GetToken();
                    break;

                case NODE_FLOAT64:
                    argument = f_data;
                    if(negative)
                    {
                        argument.f_float.Set(-argument.f_float.Get());
                    }
                    GetToken();
                    break;

                case NODE_INT64:
                    argument = f_data;
                    if(negative)
                    {
                        argument.f_int.Set(-argument.f_int.Get());
                    }
                    GetToken();
                    break;

                case ')':
                    f_lexer.ErrMsg(AS_ERR_BAD_PRAGMA, "a pragma argument can't just be '-'");
                    break;

                default:
                    f_lexer.ErrMsg(AS_ERR_BAD_PRAGMA, "invalid argument type for a pragma");
                    break;

                }
            }
            if(f_data.f_type != ')')
            {
                f_lexer.ErrMsg(AS_ERR_BAD_PRAGMA, "invalid argument for a pragma");
            }
            else
            {
                GetToken();
            }
        }
        if(f_data.f_type == '?')
        {
            prima = true;
            GetToken();
        }

        // Check out this pragma. We have the following
        // info about each pragma:
        //
        //    name        The pragma name
        //    argument    The pragma argument (unknown by default)
        //    prima        True if pragma name followed by '?'
        //
        // NOTE: pragmas that we don't recognize are simply
        // being ignored.
        //
        long value = 1;
        option_t option = AS_OPTION_UNKNOWN;
        if(name == "extended_operators")
        {
            option = AS_OPTION_EXTENDED_OPERATORS;
        }
        else if(name == "no_extended_operators")
        {
            option = AS_OPTION_EXTENDED_OPERATORS;
            value = 0;
        }
        else if(name == "extended_escape_sequences")
        {
            option = AS_OPTION_EXTENDED_ESCAPE_SEQUENCES;
        }
        else if(name == "no_extended_escape_sequences")
        {
            option = AS_OPTION_EXTENDED_ESCAPE_SEQUENCES;
            value = 0;
        }
        else if(name == "octal")
        {
            option = AS_OPTION_OCTAL;
        }
        else if(name == "no_octal")
        {
            option = AS_OPTION_OCTAL;
            value = 0;
        }
        else if(name == "strict")
        {
            option = AS_OPTION_STRICT;
        }
        else if(name == "not_strict")
        {
            option = AS_OPTION_STRICT;
            value = 0;
        }
        else if(name == "trace_to_object")
        {
            option = AS_OPTION_TRACE_TO_OBJECT;
        }
        else if(name == "no_trace_to_object")
        {
            option = AS_OPTION_TRACE_TO_OBJECT;
            value = 0;
        }
        else if(name == "trace")
        {
            option = AS_OPTION_TRACE;
        }
        else if(name == "no_trace")
        {
            option = AS_OPTION_TRACE;
            value = 0;
        }
        if(option != AS_OPTION_UNKNOWN)
        {
            Pragma_Option(option, prima, argument, value);
        }
    }
}



void IntParser::Pragma_Option(option_t option, bool prima, const Data& argument, long value)
{
    // did we get any option object?
    if(f_options == 0)
    {
        return;
    }

    if(prima)
    {
        if(f_options->GetOption(option) != value)
        {
            f_lexer.ErrMsg(AS_ERR_PRAGMA_FAILED, "prima pragma failed");
        }
        return;
    }

    switch(argument.f_type)
    {
    case NODE_UNKNOWN:
        f_options->SetOption(option, value);
        break;

    case NODE_TRUE:
        f_options->SetOption(option, 1);
        break;

    case NODE_INT64:
        f_options->SetOption(option, argument.f_int.Get() != 0);
        break;

    case NODE_FLOAT64:
        f_options->SetOption(option, argument.f_float.Get() != 0.0f);
        break;

    case NODE_STRING:
        f_lexer.ErrMsg(AS_ERR_INCOMPATIBLE_PRAGMA_ARGUMENT, "incompatible pragma argument");
        break;

    default: // NODE_FALSE
        f_options->SetOption(option, 0);
        break;

    }
}





}
// namespace as

// vim: ts=4 sw=4 et
