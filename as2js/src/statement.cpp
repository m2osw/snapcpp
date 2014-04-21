/* statement.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER BLOCK  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Block(NodePtr& node)
{
    // handle the emptiness right here
    if(f_data.f_type != '}') {
        DirectiveList(node);
    }

    if(f_data.f_type != '}') {
        f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'}' expected to close a block");
    }
    else {
        GetToken();
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER BREAK & CONTINUE  ****************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::BreakContinue(NodePtr& node, node_t type)
{
    node.CreateNode(type);
    node.SetInputInfo(f_lexer.GetInput());

    if(f_data.f_type == NODE_IDENTIFIER) {
        Data& data = node.GetData();
        data.f_str = f_data.f_str;
        GetToken();
    }
    else if(f_data.f_type == NODE_DEFAULT) {
        // default is equivalent to no label
        GetToken();
    }

    if(f_data.f_type != ';') {
        f_lexer.ErrMsg(AS_ERR_INVALID_LABEL, "'break' and 'continue' can be followed by one label only");
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER CASE  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Case(NodePtr& node)
{
    node.CreateNode(NODE_CASE);
    node.SetInputInfo(f_lexer.GetInput());
    NodePtr expr;
    Expression(expr);
    node.AddChild(expr);

    if(f_options != 0
    && f_options->GetOption(AS_OPTION_EXTENDED_STATEMENTS) != 0) {
        // check for 'case <expr> ... <expr>:'
        if(f_data.f_type == NODE_REST || f_data.f_type == NODE_RANGE) {
            GetToken();
            Expression(expr);
            node.AddChild(expr);
        }
    }

    if(f_data.f_type == ':') {
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_CASE_LABEL, "case expression expected to be followed by ':'");
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER CATCH  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Catch(NodePtr& node)
{
    if(f_data.f_type == '(') {
        node.CreateNode(NODE_CATCH);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();
        NodePtr parameters;
        bool unused;
        ParameterList(parameters, unused);
        node.AddChild(parameters);
        // we want exactly ONE parameter
        int count = parameters.GetChildCount();
        if(count == 0) {
            f_lexer.ErrMsg(AS_ERR_INVALID_CATCH, "the 'catch' keyword expects one parameter");
        }
        else if(count > 1) {
            f_lexer.ErrMsg(AS_ERR_INVALID_CATCH, "the 'catch' keyword expects at most one parameter");
        }
        else {
            // There is just one parameter, make sure there
            // isn't an initializer
            bool has_type = false;
            NodePtr& param = parameters.GetChild(0);
            count = param.GetChildCount();
            while(count > 0) {
                --count;
                if(param.GetChild(count).GetData().f_type == NODE_SET) {
                    f_lexer.ErrMsg(AS_ERR_INVALID_CATCH, "the 'catch' parameters can't have an initializer");
                    break;
                }
                has_type = true;
            }
            if(has_type) {
                Data& data = node.GetData();
                data.f_int.Set(NODE_CATCH_FLAG_TYPED);
            }
        }
        if(f_data.f_type == ')') {
            GetToken();
            if(f_data.f_type == '{') {
                GetToken();
                NodePtr directive_list;
                Block(directive_list);
                node.AddChild(directive_list);
            }
            else {
                f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'{' expected after the 'catch' parameter");
            }
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the 'catch' parameter list");
        }
    }
    else {
        f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected after the 'catch' keyword");
    }
}






/**********************************************************************/
/**********************************************************************/
/***  PARSER DEFAULT  *************************************************/
/**********************************************************************/
/**********************************************************************/

// NOTE: if default wasn't a keyword, then it could be used as a
//     label like any user label!
//
//     The fact that it is a keyword allows us to forbid the goto
//     without having to do any extra work.
//
void IntParser::Default(NodePtr& node)
{
    node.CreateNode(NODE_DEFAULT);
    node.SetInputInfo(f_lexer.GetInput());

    // default is just itself!
    if(f_data.f_type == ':') {
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_DEFAULT_LABEL, "default label expected to be followed by ':'");
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER DO  ******************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Do(NodePtr& node)
{
    node.CreateNode(NODE_DO);
    node.SetInputInfo(f_lexer.GetInput());

    NodePtr directive;
    Directive(directive);
    node.AddChild(directive);

    if(f_data.f_type == NODE_WHILE) {
        GetToken();
        if(f_data.f_type == '(') {
            GetToken();
            NodePtr expr;
            Expression(expr);
            node.AddChild(expr);
            if(f_data.f_type != ')') {
                f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the 'while' expression");
            }
            else {
                GetToken();
            }
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected after the 'while' keyword");
        }
    }
    else {
        f_lexer.ErrMsg(AS_ERR_INVALID_DO, "'while' expected after the block of a 'do' keyword");
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER FOR  *****************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::For(NodePtr& node)
{
    bool for_each = f_data.f_type == NODE_IDENTIFIER
            && f_data.f_str == "each";
    if(for_each) {
        GetToken();
    }
    if(f_data.f_type == '(') {
        // NOTE: Avoid IN within the expression by setting
        //     a flag in the lexer. This way it will be
        //     returned as FOR_IN instead.
        //     Not necessary since we can know in which
        //     mode we are anyway.
        //f_lexer.SetForIn(true);

        node.CreateNode(NODE_FOR);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        if(f_data.f_type == NODE_CONST
        || f_data.f_type == NODE_VAR) {
            // *** VARIABLE ***
            NodePtr variables;
            bool constant = f_data.f_type == NODE_CONST;
            if(constant) {
                GetToken();
                if(f_data.f_type == NODE_VAR) {
                    GetToken();
                }
            }
            else {
                GetToken();
            }
            Variable(variables, constant);
            node.AddChild(variables);
        }
        else if(f_data.f_type == ';') {
            // When we have ';' we've got an empty initializer!
            NodePtr empty;
            empty.CreateNode(NODE_EMPTY);
            empty.SetInputInfo(f_lexer.GetInput());
            node.AddChild(empty);
        }
        else /*if(f_data.f_type != ';')*/ {
            // *** EXPRESSION ***
            NodePtr expr;
            Expression(expr);
            if(f_data.f_type != ';') {
                Data& data = expr.GetData();
                if(data.f_type != NODE_IN) {
                    goto bad_forin;
                }
                NodePtr left = expr.GetChild(0);
                NodePtr right = expr.GetChild(1);
                expr.DeleteChild(0);
                expr.DeleteChild(0);
                node.AddChild(left);
                node.AddChild(right);
                goto forin_done;
            }
            else {
                node.AddChild(expr);
            }
        }

        // back to normal -- don't need, really
        //f_lexer.SetForIn(false);

        // This can happen when we return from the
        // Variable() function
        if(f_data.f_type == NODE_IN) {
            // *** IN ***
            GetToken();
            NodePtr expr;
            Expression(expr);
            node.AddChild(expr);
        }
        else if(f_data.f_type == ';') {
            // *** SECOND EXPRESSION ***
            GetToken();
            NodePtr expr;
            if(f_data.f_type == ';') {
                // empty expression
                expr.CreateNode(NODE_EMPTY);
                expr.SetInputInfo(f_lexer.GetInput());
            }
            else {
                Expression(expr);
            }
            node.AddChild(expr);
            if(f_data.f_type == ';') {
                // *** THIRD EXPRESSION ***
                GetToken();
                NodePtr expr;
                if(f_data.f_type == ')') {
                    expr.CreateNode(NODE_EMPTY);
                    expr.SetInputInfo(f_lexer.GetInput());
                }
                else {
                    Expression(expr);
                }
                node.AddChild(expr);
            }
            else {
                f_lexer.ErrMsg(AS_ERR_SEMICOLON_EXPECTED, "';' expected between the last two 'for' expressions");
            }
        }
        else {
bad_forin:
            f_lexer.ErrMsg(AS_ERR_SEMICOLON_EXPECTED, "';' or 'in' expected between the 'for' expressions");
        }

forin_done:
        if(f_data.f_type != ')') {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to close the 'for' expressions");
        }
        else {
            GetToken();
        }

        if(node.GetChildCount() == 2 && for_each) {
            Data& data = node.GetData();
            data.f_int.Set(data.f_int.Get() | NODE_FOR_FLAG_FOREACH);
        }
        else if(for_each) {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'for each()' only available with an enumeration for");
        }

        // *** DIRECTIVES ***
        NodePtr directive;
        Directive(directive);
        node.AddChild(directive);
    }
    else {
        f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected for the 'for' expressions");
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER GOTO  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Goto(NodePtr& node)
{
    if(f_data.f_type == NODE_IDENTIFIER) {
        // save the label
        node.CreateNode(NODE_GOTO);
        node.SetInputInfo(f_lexer.GetInput());
        Data& data = node.GetData();
        data.f_str = f_data.f_str;
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_INVALID_GOTO, "'goto' expects a label as parameter");
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER IF  ******************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::If(NodePtr& node)
{
    if(f_data.f_type == '(') {
        node.CreateNode(NODE_IF);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();
        NodePtr expr;
        Expression(expr);
        node.AddChild(expr);
        if(f_data.f_type == ')') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the 'if' expression");
        }

        // IF part
        NodePtr directive;
        Directive(directive);
        node.AddChild(directive);

        // Note that this is the only place where ELSE is
        // really permitted
        if(f_data.f_type == NODE_ELSE) {
            GetToken();
            // ELSE part
            NodePtr directive;
            Directive(directive);
            node.AddChild(directive);
        }
    }
    else {
        f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected after the 'if' keyword");
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER RETURN  **************************************************/
/**********************************************************************/
/**********************************************************************/



void IntParser::Return(NodePtr& node)
{
    node.CreateNode(NODE_RETURN);
    node.SetInputInfo(f_lexer.GetInput());
    if(f_data.f_type != ';') {
        NodePtr expr;
        Expression(expr);
        node.AddChild(expr);
    }
}


/**********************************************************************/
/**********************************************************************/
/***  PARSER TRY & FINALLY  *******************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::TryFinally(NodePtr& node, node_t type)
{
    if(f_data.f_type == '{') {
        GetToken();
        node.CreateNode(type);
        node.SetInputInfo(f_lexer.GetInput());
        NodePtr block;
        Block(block);
        node.AddChild(block);
    }
    else {
        f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'{' expected after the 'try' keyword");
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER SWITCH  **************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Switch(NodePtr& node)
{
    bool        has_open;

    if(f_data.f_type == '(') {
        node.CreateNode(NODE_SWITCH);
        node.SetInputInfo(f_lexer.GetInput());
        Data& data = node.GetData();
        // a default is important to support ranges properly
        data.f_int.Set(NODE_UNKNOWN);
        GetToken();
        NodePtr expr;
        Expression(expr);
        node.AddChild(expr);
        if(f_data.f_type == ')') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the 'switch' expression");
        }
        if(f_data.f_type == NODE_WITH) {
            GetToken();
            has_open = f_data.f_type == '(';
            if(has_open) {
                GetToken();
            }
            switch(f_data.f_type) {
            // equality
            case NODE_STRICTLY_EQUAL:
            case NODE_EQUAL:
            case NODE_NOT_EQUAL:
            case NODE_STRICTLY_NOT_EQUAL:
            // relational
            case NODE_MATCH:
            case NODE_IN:
            case NODE_IS:
            case NODE_AS:
            case NODE_INSTANCEOF:
            case NODE_LESS:
            case NODE_LESS_EQUAL:
            case NODE_GREATER:
            case NODE_GREATER_EQUAL:
            // so the user can specify the default too
            case NODE_DEFAULT:
                data.f_int.Set(f_data.f_type);
                GetToken();
                break;

            default:
                f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "insupported operator for a 'switch() with()' expression");
                break;

            }
            if(f_data.f_type == ')') {
                GetToken();
                if(!has_open) {
                    f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' was expected to start the 'switch() with()' expression");
                }
            }
            else if(has_open) {
                f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the 'switch() with()' expression");
            }
        }
        NodePtr attr_list;
        Attributes(attr_list);
        long attr_count = attr_list.GetChildCount();
        if(attr_count > 0) {
            node.SetLink(NodePtr::LINK_ATTRIBUTES, attr_list);
        }
        if(f_data.f_type == '{') {
            GetToken();
            NodePtr directive_list;
            Block(directive_list);
            node.AddChild(directive_list);
        }
        else {
            f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'{' expected after the 'switch' expression");
        }
    }
    else {
        f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected after the 'switch' keyword");
    }
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER THROW  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Throw(NodePtr& node)
{
    node.CreateNode(NODE_THROW);
    node.SetInputInfo(f_lexer.GetInput());

    NodePtr expr;
    Expression(expr);
    node.AddChild(expr);
}



/**********************************************************************/
/**********************************************************************/
/***  PARSER WITH & WHILE  ********************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::WithWhile(NodePtr& node, node_t type)
{
    const char *inst = type == NODE_WITH ? "with" : "while";

    if(f_data.f_type == '(') {
        node.CreateNode(type);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();
        NodePtr expr;
        Expression(expr);
        node.AddChild(expr);
        if(f_data.f_type == ')') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the '%s' expression", inst);
        }
        NodePtr directive;
        Directive(directive);
        node.AddChild(directive);
    }
    else
    {
        f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected after the '%s' keyword", inst);
    }
}






}
// namespace as

// vim: ts=4 sw=4 et
