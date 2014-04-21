/* function.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER FUNCTION  ************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::ParameterList(NodePtr& node, bool& has_out)
{
    has_out = false;

    // accept function stuff(void) { ... } as in C/C++
    if(f_data.f_type == NODE_VOID
    || (f_data.f_type == NODE_IDENTIFIER && f_data.f_str == "Void")) {
        GetToken();
        return;
    }

    node.CreateNode(NODE_PARAMETERS);
    node.SetInputInfo(f_lexer.GetInput());

    // special case which explicitly says that a function definition
    // is not prototyped (vs. an empty list of parameters which is
    // equivalent to a (void)); this means the function accepts
    // parameters, their type & number are just not defined
    if(f_data.f_type == NODE_IDENTIFIER
    && f_data.f_str == "unprototyped") {
        NodePtr param;
        param.CreateNode();
        param.SetInputInfo(f_lexer.GetInput());
        f_data.f_type = NODE_PARAM;
        f_data.f_int.Set(NODE_PARAMETERS_FLAG_UNPROTOTYPED);
        param.SetData(f_data);
        node.AddChild(param);
        GetToken();
        return;
    }

    bool invalid = false;
    for(;;) {
        int flags = 0;

        // get all the attributes for the parameters
        // (const, in, out, named)
        bool more = true;
        do {
            switch(f_data.f_type) {
            case NODE_REST:
                flags |= NODE_PARAMETERS_FLAG_REST;
                GetToken();
                break;

            case NODE_CONST:
                flags |= NODE_PARAMETERS_FLAG_CONST;
                GetToken();
                break;

            case NODE_IN:
                flags |= NODE_PARAMETERS_FLAG_IN;
                GetToken();
                break;

            case NODE_VAR:
                GetToken();
                break;

            case NODE_IDENTIFIER:
                if(f_data.f_str == "out") {
                    flags |= NODE_PARAMETERS_FLAG_OUT;
                    GetToken();
                    has_out = true;
                    break;
                }
                if(f_data.f_str == "named") {
                    flags |= NODE_PARAMETERS_FLAG_NAMED;
                    GetToken();
                    break;
                }
                if(f_data.f_str == "unchecked") {
                    flags |= NODE_PARAMETERS_FLAG_UNCHECKED;
                    GetToken();
                    break;
                }
                /*FALLTHROUGH*/
            default:
                more = false;
                break;

            }
        } while(more); 

        if(flags != 0) {
            invalid = false;
            if((flags & NODE_PARAMETERS_FLAG_OUT) != 0) {
                if((flags & NODE_PARAMETERS_FLAG_REST) != 0) {
f_lexer.ErrMsg(AS_ERR_INVALID_PARAMETERS, "you cannot use the function parameter attribute 'out' with '...'");
                }
                if((flags & NODE_PARAMETERS_FLAG_CONST) != 0) {
f_lexer.ErrMsg(AS_ERR_INVALID_PARAMETERS, "you cannot use the function attributes 'out' and 'const' together");
                }
            }
        }

        if(f_data.f_type == NODE_IDENTIFIER) {
            invalid = false;
            NodePtr param;
            param.CreateNode();
            param.SetInputInfo(f_lexer.GetInput());
            f_data.f_type = NODE_PARAM;
            f_data.f_int.Set(flags);
            param.SetData(f_data);
            node.AddChild(param);
            GetToken();
            if(f_data.f_type == ':') {
                // what about REST? does this mean all
                // the following parameters need to be
                // of that type?
                GetToken();
                NodePtr type;
                ConditionalExpression(type, false);
                param.AddChild(type);
            }
            if(f_data.f_type == '=') {
                // should not accept when REST is set
                GetToken();
                NodePtr initializer;
                initializer.CreateNode(NODE_SET);
                initializer.SetInputInfo(f_lexer.GetInput());
                NodePtr expr;
                ConditionalExpression(expr, false);
                initializer.AddChild(expr);
                param.AddChild(initializer);
            }
        }
        else if((flags & NODE_PARAMETERS_FLAG_REST) != 0) {
            NodePtr param;
            param.CreateNode();
            param.SetInputInfo(f_lexer.GetInput());
            Data flg;
            flg.f_type = NODE_PARAM;
            flg.f_int.Set(flags);
            param.SetData(flg);
            node.AddChild(param);
            invalid = false;
        }

        if(f_data.f_type == ')') {
            return;
        }
        else if(f_data.f_type != ',') {
            if(!invalid) {
                f_lexer.ErrMsg(AS_ERR_INVALID_PARAMETERS, "expected an identifier as the parameter name (not token %d)", f_data.f_type);
            }
            if(f_data.f_type == NODE_EOF
            || f_data.f_type == ';'
            || f_data.f_type == '{'
            || f_data.f_type == '}') {
                return;
            }
            if(invalid) {
                GetToken();
            }
            invalid = true;
        }
        else {
            if((flags & NODE_PARAMETERS_FLAG_REST) != 0) {
                f_lexer.ErrMsg(AS_ERR_INVALID_PARAMETERS, "no other parameter expected after '...'");
            }
            GetToken();
        }
    }
}



void IntParser::Function(NodePtr& node, bool expression)
{
    node.CreateNode(NODE_FUNCTION);
    node.SetInputInfo(f_lexer.GetInput());
    Data& data = node.GetData();

    switch(f_data.f_type) {
    case NODE_IDENTIFIER:
    {
        long flags = 0;
        const char *etter = "";
        if(f_data.f_str == "get") {
            // *** GETTER ***
            flags = NODE_FUNCTION_FLAG_GETTER;
            etter = "->";
        }
        else if(f_data.f_str == "set") {
            // *** SETTER ***
            flags = NODE_FUNCTION_FLAG_SETTER;
            etter = "<-";
        }
        if(flags != 0) {
            GetToken();
            if(f_data.f_type == NODE_IDENTIFIER) {
                data.f_int.Set(flags);
                data.f_str = etter;
                data.f_str += f_data.f_str;
                GetToken();
            }
            else if(f_data.f_type == NODE_STRING) {
                // this is an extension, you can't have
                // a getter or setter which is also an
                // operator overload
                data.f_int.Set(flags);
                data.f_str = etter;
                data.f_str += f_data.f_str;
                if(node.StringToOperator() != NODE_UNKNOWN) {
                    f_lexer.ErrMsg(AS_ERR_INVALID_FUNCTION, "operators cannot be a getter nor a setter function");
                }
                GetToken();
            }
            else if(f_data.f_type == '(') {
                // not a getter or setter when only get() or set()
                if((flags & NODE_FUNCTION_FLAG_GETTER) != 0) {
                    data.f_str = "get";
                }
                else {
                    data.f_str = "set";
                }
                flags = 0;
            }
            else if(!expression) {
                f_lexer.ErrMsg(AS_ERR_INVALID_FUNCTION, "getter and setter functions require a name");
            }
            if(expression && flags != 0) {
                f_lexer.ErrMsg(AS_ERR_INVALID_FUNCTION, "expression functions cannot be getter nor setter functions");
            }
        }
        else {
            // *** STANDARD ***
            data.f_str = f_data.f_str;
            GetToken();
            if(f_data.f_type == NODE_IDENTIFIER) {
                // Ooops? this could be that the user misspelled get or set
                f_lexer.ErrMsg(AS_ERR_INVALID_FUNCTION, "only one name is expected for a function (misspelled get or set?)");
                GetToken();
            }
        }
    }
        break;

    case NODE_STRING:
    {
        // *** OPERATOR OVERLOAD ***
        // (though we just accept any string at this time)
        data.f_str = f_data.f_str;
        if(node.StringToOperator() != NODE_UNKNOWN) {
            data.f_int.Set(NODE_FUNCTION_FLAG_OPERATOR);
        }
        GetToken();
    }
        break;

    // all the operators which can be overloaded as is
    case NODE_LOGICAL_NOT:
    case NODE_MODULO:
    case NODE_BITWISE_AND:
    case NODE_MULTIPLY:
    case NODE_ADD:
    case NODE_SUBTRACT:
    case NODE_DIVIDE:
    case NODE_LESS:
    case NODE_ASSIGNMENT:
    case NODE_GREATER:
    case NODE_BITWISE_XOR:
    case NODE_BITWISE_OR:
    case NODE_BITWISE_NOT:
    case NODE_ASSIGNMENT_ADD:
    case NODE_ASSIGNMENT_BITWISE_AND:
    case NODE_ASSIGNMENT_BITWISE_OR:
    case NODE_ASSIGNMENT_BITWISE_XOR:
    case NODE_ASSIGNMENT_DIVIDE:
    case NODE_ASSIGNMENT_LOGICAL_AND:
    case NODE_ASSIGNMENT_LOGICAL_OR:
    case NODE_ASSIGNMENT_LOGICAL_XOR:
    case NODE_ASSIGNMENT_MAXIMUM:
    case NODE_ASSIGNMENT_MINIMUM:
    case NODE_ASSIGNMENT_MODULO:
    case NODE_ASSIGNMENT_MULTIPLY:
    case NODE_ASSIGNMENT_POWER:
    case NODE_ASSIGNMENT_ROTATE_LEFT:
    case NODE_ASSIGNMENT_ROTATE_RIGHT:
    case NODE_ASSIGNMENT_SHIFT_LEFT:
    case NODE_ASSIGNMENT_SHIFT_RIGHT:
    case NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case NODE_ASSIGNMENT_SUBTRACT:
    case NODE_DECREMENT:
    case NODE_EQUAL:
    case NODE_GREATER_EQUAL:
    case NODE_INCREMENT:
    case NODE_LESS_EQUAL:
    case NODE_LOGICAL_AND:
    case NODE_LOGICAL_OR:
    case NODE_LOGICAL_XOR:
    case NODE_MATCH:
    case NODE_MAXIMUM:
    case NODE_MINIMUM:
    case NODE_NOT_EQUAL:
    case NODE_POST_DECREMENT:
    case NODE_POST_INCREMENT:
    case NODE_POWER:
    case NODE_ROTATE_LEFT:
    case NODE_ROTATE_RIGHT:
    case NODE_SHIFT_LEFT:
    case NODE_SHIFT_RIGHT:
    case NODE_SHIFT_RIGHT_UNSIGNED:
    case NODE_STRICTLY_EQUAL:
    case NODE_STRICTLY_NOT_EQUAL:
    {
        // save the operator type in the node to be able
        // to get the string
        data.f_type = f_data.f_type;
        data.f_str = node.OperatorToString();
        data.f_int.Set(NODE_FUNCTION_FLAG_OPERATOR);
        data.f_type = NODE_FUNCTION;
        GetToken();
    }
        break;

    // this is a complicated one because () can
    // be used for the parameters too
    case '(':
    {
        Data restore = f_data;
        GetToken();
        if(f_data.f_type == ')') {
            Data save = f_data;
            GetToken();
            if(f_data.f_type == '(') {
                // this is taken as the "()" operator!
                data.f_str = "()";
                data.f_int.Set(NODE_FUNCTION_FLAG_OPERATOR);
                data.f_type = NODE_FUNCTION;
            }
            else {
                UngetToken(f_data);
                UngetToken(save);
                f_data = restore;
            }
        }
        else {
            UngetToken(f_data);
            f_data = restore;
        }
    }
        break;

    default:
        if(!expression) {
            f_lexer.ErrMsg(AS_ERR_INVALID_FUNCTION, "function declarations are required to be named");
        }
        break;

    }

    if(f_data.f_type == '(') {
        GetToken();
        if(f_data.f_type != ')') {
            // read params
            NodePtr params;
            bool has_out;
            ParameterList(params, has_out);
            if(has_out) {
                data.f_int.Set(data.f_int.Get() | NODE_FUNCTION_FLAG_OUT);
            }
            if(params.HasNode()) {
                node.AddChild(params);
            }
            else {
                data.f_int.Set(data.f_int.Get() | NODE_FUNCTION_FLAG_NOPARAMS);
            }
            if(f_data.f_type != ')') {
                f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to close the 'function' parameters");
            }
            else {
                GetToken();
            }
        }
        else {
            GetToken();
        }
    }

    if(f_data.f_type == ':') {
        NodePtr expr;
        GetToken();
        if(f_data.f_type == NODE_VOID
        || (f_data.f_type == NODE_IDENTIFIER
                && f_data.f_str == "Void")) {
            // special case of a procedure
            //expr.CreateNode(NODE_VOID);
            //expr.SetInputInfo(f_lexer.GetInput());
            data.f_int.Set(data.f_int.Get() | NODE_FUNCTION_FLAG_VOID);
            GetToken();
        }
        else if(f_data.f_type == NODE_IDENTIFIER
                && f_data.f_str == "Never") {
            data.f_int.Set(data.f_int.Get() | NODE_FUNCTION_FLAG_NEVER);
            GetToken();
        }
        else {
            ConditionalExpression(expr, false);
            node.AddChild(expr);
        }
    }

    if(f_data.f_type == '{') {
        GetToken();
        if(f_data.f_type != '}') {
            // NOTE: by not inserting anything when we have
            // an empty definition, it looks like an abstract
            // definition... we may want to change that at a
            // later time.
            NodePtr directive_list;
            DirectiveList(directive_list);
            node.AddChild(directive_list);
        }
        if(f_data.f_type != '}') {
            f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'}' expected to close the 'function' block");
        }
        else {
            GetToken();
        }
    }
    // empty function (a.k.a abstract or function as a type)
    // such functions are permitted in interfaces!
}



}
// namespace as2js

// vim: ts=4 sw=4 et
