/* class.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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

#include    "as2js/parser.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER CLASS  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Class(NodePtr& node, node_t type)
{
    if(f_data.f_type != NODE_IDENTIFIER) {
        f_lexer.ErrMsg(AS_ERR_INVALID_CLASS, "the name of the class is expected after the keyword 'class'");
        return;
    }

    node.CreateNode(type);
    node.SetInputInfo(f_lexer.GetInput());

    // *** NAME ***
    Data& data = node.GetData();
    data.f_str = f_data.f_str;
    GetToken();

    // *** INHERITANCE ***
    while(f_data.f_type == NODE_EXTENDS
            || f_data.f_type == NODE_IMPLEMENTS) {
        NodePtr inherits;
        inherits.CreateNode(f_data.f_type);
        inherits.SetInputInfo(f_lexer.GetInput());
        node.AddChild(inherits);

        GetToken();

        NodePtr expr;
        Expression(expr);
        inherits.AddChild(expr);
        // TODO: EXTENDS and IMPLEMENTS don't accept assignments.
        // TODO: EXTENDS doesn't accept lists.
        //     We need to test for that here.
    }
    // TODO: note that we only can accept one EXTENDS and
    //     one IMPLEMENTS in that order. We need to check
    //     that here. [that's according to the spec. is
    //     that really important?]

    if(f_data.f_type == '{') {
        GetToken();

        // *** DECLARATION ***
        if(f_data.f_type != '}') {
            NodePtr directive_list;
            DirectiveList(directive_list);
            node.AddChild(directive_list);
        }

        if(f_data.f_type == '}') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'}' expected to close the 'class' definition");
        }
    }
    else if(f_data.f_type != ';') {    // accept empty class definitions (for typedef's and forward declaration)
        f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'{' expected to start the 'class' definition");
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER ENUM  ****************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Enum(NodePtr& node)
{
    node.CreateNode(NODE_ENUM);
    node.SetInputInfo(f_lexer.GetInput());

    // enumerations can be unamed
    if(f_data.f_type == NODE_IDENTIFIER) {
        Data& data = node.GetData();
        data.f_str = f_data.f_str;
        GetToken();
    }

    // in case the name was not specified, we can still have a type (?)
    if(f_data.f_type == ':') {
        NodePtr type;
        Expression(type);
        node.AddChild(type);
    }

    if(f_data.f_type != '{') {
        if(f_data.f_type == ';') {
            // empty enumeration
            return;
        }
        f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'{' expected to start the 'enum' definition");
        return;
    }

    GetToken();

    Data previous;
    previous.f_type = NODE_NULL;
    while(f_data.f_type != '}' && f_data.f_type != NODE_EOF) {
        if(f_data.f_type == ',') {
            // TODO: should we warn here?
            GetToken();
            continue;
        }
        String current_name = "null";
        NodePtr entry;
        entry.CreateNode(NODE_VARIABLE);
        entry.SetInputInfo(f_lexer.GetInput());
        node.AddChild(entry);
        if(f_data.f_type == NODE_IDENTIFIER) {
            f_data.f_type = NODE_VARIABLE;
            f_data.f_int.Set(NODE_VAR_FLAG_CONST | NODE_VAR_FLAG_ENUM);
            entry.SetData(f_data);
            current_name = f_data.f_str;
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_INVALID_ENUM, "each 'enum' entry needs to include an identifier");
        }
        NodePtr expr;
        if(f_data.f_type == '=') {
            GetToken();
            ConditionalExpression(expr, false);
        }
        else if(previous.f_type == NODE_NULL) {
            // very first time
            expr.CreateNode();
            expr.SetInputInfo(f_lexer.GetInput());
            Data set_zero;
            set_zero.f_type = NODE_INT64;
            set_zero.f_int.Set(0);
            expr.SetData(set_zero);
        }
        else {
            expr.CreateNode(NODE_ADD);
            expr.SetInputInfo(f_lexer.GetInput());
            NodePtr left;
            left.CreateNode();
            left.SetInputInfo(f_lexer.GetInput());
            left.SetData(previous);
            expr.AddChild(left);
            NodePtr one;
            one.CreateNode();
            one.SetInputInfo(f_lexer.GetInput());
            Data set_one;
            set_one.f_type = NODE_INT64;
            set_one.f_int.Set(1);
            one.SetData(set_one);
            expr.AddChild(one);
        }

        NodePtr set;
        set.CreateNode(NODE_SET);
        set.SetInputInfo(f_lexer.GetInput());
        set.AddChild(expr);
        entry.AddChild(set);

        previous.f_type = NODE_IDENTIFIER;
        previous.f_str = current_name;

        if(f_data.f_type == ',') {
            GetToken();
        }
        else if(f_data.f_type != '}') {
            f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "',' expected between enumeration elements");
        }
    }

    if(f_data.f_type == '}') {
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'}' expected to close the 'enum' definition");
    }
}




}
// namespace as

// vim: ts=4 sw=4 et
