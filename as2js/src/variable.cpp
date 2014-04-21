/* variable.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER VARIABLE  ************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Variable(NodePtr& node, bool const constant)
{
    long    flags;

    if(constant)
    {
        flags = NODE_VAR_FLAG_CONST;
    }
    else
    {
        flags = 0;
    }

    node.CreateNode(NODE_VAR);
    node.SetInputInfo(f_lexer.GetInput());
    for(;;) {
        NodePtr variable;
        variable.CreateNode(NODE_VARIABLE);
        variable.SetInputInfo(f_lexer.GetInput());
        node.AddChild(variable);

        Data& data = variable.GetData();
        data.f_int.Set(flags);

        if(f_data.f_type == NODE_IDENTIFIER) {
            data.f_str = f_data.f_str;
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_INVALID_VARIABLE, "expected an identifier as the variable name");
        }

        if(f_data.f_type == ':') {
            GetToken();
            NodePtr type;
            ConditionalExpression(type, false);
            variable.AddChild(type);
        }

        if(f_data.f_type == '=') {
            GetToken();
            do {
                NodePtr initializer;
                initializer.CreateNode(NODE_SET);
                initializer.SetInputInfo(f_lexer.GetInput());
                NodePtr expr;
                ConditionalExpression(expr, false);
                initializer.AddChild(expr);
                variable.AddChild(initializer);
                // We loop in case we have a list of attributes!
                // This could also be a big syntax error (a missing
                // operator in most cases.) We will report the error
                // later once we know where the variable is being
                // used.
            }
            while((flags & NODE_VAR_FLAG_CONST) != 0
                && f_data.f_type != ','
                && f_data.f_type != ';'
                && f_data.f_type != '{'
                && f_data.f_type != '}'
                && f_data.f_type != ')');
        }

        if(f_data.f_type != ',')
        {
            return;
        }
        GetToken();
    }
}




}
// namespace as

// vim: ts=4 sw=4 et
