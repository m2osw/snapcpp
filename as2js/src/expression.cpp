/* expression.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER EXPRESSION  **********************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Expression(NodePtr& node)
{
    ListExpression(node, false, false);
}


void IntParser::ListExpression(NodePtr& node, bool rest, bool empty)
{
    if(empty && f_data.f_type == ',') {
        node.CreateNode(NODE_EMPTY);
        node.SetInputInfo(f_lexer.GetInput());
    }
    else {
        AssignmentExpression(node);
        // accept named parameters
        if(f_data.f_type == ':' && rest) {
            GetToken();
            NodePtr name;
            name.CreateNode(NODE_NAME);
            name.SetInputInfo(f_lexer.GetInput());
            name.AddChild(node);
            AssignmentExpression(node);
            node.AddChild(name);
        }
    }

    if(f_data.f_type == ',') {
        NodePtr item = node;

        node.CreateNode(NODE_LIST);
        node.SetInputInfo(f_lexer.GetInput());

        node.AddChild(item);

        int has_rest = 0;
        while(f_data.f_type == ',') {
            GetToken();
            if(has_rest == 1) {
                f_lexer.ErrMsg(AS_ERR_INVALID_REST, "'...' was expected to be the last expression only");
                has_rest = 2;
            }
            if(empty && f_data.f_type == ',') {
                NodePtr empty;
                empty.CreateNode(NODE_EMPTY);
                empty.SetInputInfo(f_lexer.GetInput());
                node.AddChild(empty);
            }
            else if(rest && f_data.f_type == NODE_REST) {
                NodePtr rest;
                rest.CreateNode(NODE_REST);
                rest.SetInputInfo(f_lexer.GetInput());
                node.AddChild(rest);
                GetToken();
                if(has_rest == 0) {
                    has_rest = 1;
                }
                // note: we expect ')' here but we
                // let the user put ',' <expr> still
            }
            else {
                AssignmentExpression(item);
                // accept named parameters
                if(f_data.f_type == ':' && rest) {
                    GetToken();
                    NodePtr name;
                    name.CreateNode(NODE_NAME);
                    name.SetInputInfo(f_lexer.GetInput());
                    name.AddChild(item);
                    if(f_data.f_type == NODE_REST) {
                        item.CreateNode(NODE_REST);
                        item.SetInputInfo(f_lexer.GetInput());
                        GetToken();
                        if(has_rest == 0) {
                            has_rest = 1;
                        }
                        // note: we expect ')' here but we
                        // let the user put ',' <expr> still
                    }
                    else {
                        AssignmentExpression(item);
                    }
                    item.AddChild(name);
                }
                node.AddChild(item);
            }
        }

        // TODO: check that the list ends with a NODE_REST
    }
}


void IntParser::AssignmentExpression(NodePtr& node)
{
    ConditionalExpression(node, true);

    // TODO: check that the result is a postfix expression
    switch(f_data.f_type) {
    case '=':    // NODE_ASSIGNMENT
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
        break;

    default:
        return;

    }

    NodePtr left = node;

    node.CreateNode(f_data.f_type);
    node.SetInputInfo(f_lexer.GetInput());

    GetToken();
    NodePtr right;
    AssignmentExpression(right);

    node.AddChild(left);
    node.AddChild(right);
}


void IntParser::ConditionalExpression(NodePtr& node, bool assignment)
{
    MinMaxExpression(node);

    if(f_data.f_type == '?') {
        NodePtr condition = node;

        node.CreateNode(NODE_CONDITIONAL);
        node.SetInputInfo(f_lexer.GetInput());
        node.AddChild(condition);

        GetToken();
        NodePtr left;
        // not like C/C++, not a list expression here
        if(assignment) {
            AssignmentExpression(left);
        }
        else {
            ConditionalExpression(left, false);
        }
        node.AddChild(left);

        if(f_data.f_type == ':') {
            GetToken();
            NodePtr right;
            if(assignment) {
                AssignmentExpression(right);
            }
            else {
                ConditionalExpression(right, false);
            }
            node.AddChild(right);
        }
        else {
            f_lexer.ErrMsg(AS_ERR_INVALID_CONDITIONAL, "invalid use of the conditional operator, ':' was expected");
        }
    }
}



void IntParser::MinMaxExpression(NodePtr& node)
{
    LogicalOrExpression(node);

    while(f_data.f_type == NODE_MINIMUM
    || f_data.f_type == NODE_MAXIMUM) {
        NodePtr left = node;

        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        LogicalOrExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::LogicalOrExpression(NodePtr& node)
{
    LogicalXOrExpression(node);

    while(f_data.f_type == NODE_LOGICAL_OR) {
        NodePtr left = node;

        node.CreateNode(NODE_LOGICAL_OR);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        LogicalXOrExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::LogicalXOrExpression(NodePtr& node)
{
    LogicalAndExpression(node);

    while(f_data.f_type == NODE_LOGICAL_XOR) {
        NodePtr left = node;

        node.CreateNode(NODE_LOGICAL_XOR);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        LogicalAndExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::LogicalAndExpression(NodePtr& node)
{
    BitwiseOrExpression(node);

    while(f_data.f_type == NODE_LOGICAL_AND) {
        NodePtr left = node;

        node.CreateNode(NODE_LOGICAL_AND);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        BitwiseOrExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}



void IntParser::BitwiseOrExpression(NodePtr& node)
{
    BitwiseXOrExpression(node);

    while(f_data.f_type == '|') {
        NodePtr left = node;

        node.CreateNode(NODE_BITWISE_OR);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        BitwiseXOrExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::BitwiseXOrExpression(NodePtr& node)
{
    BitwiseAndExpression(node);

    while(f_data.f_type == '^') {
        NodePtr left = node;

        node.CreateNode(NODE_BITWISE_XOR);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        BitwiseAndExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::BitwiseAndExpression(NodePtr& node)
{
    EqualityExpression(node);

    while(f_data.f_type == '&') {
        NodePtr left = node;

        node.CreateNode(NODE_BITWISE_AND);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        EqualityExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::EqualityExpression(NodePtr& node)
{
    RelationalExpression(node);

    while(f_data.f_type == NODE_EQUAL
    || f_data.f_type == NODE_NOT_EQUAL
    || f_data.f_type == NODE_STRICTLY_EQUAL
    || f_data.f_type == NODE_STRICTLY_NOT_EQUAL) {
        NodePtr left = node;

        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        RelationalExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::RelationalExpression(NodePtr& node)
{
    ShiftExpression(node);

    while(f_data.f_type == NODE_LESS
    || f_data.f_type == NODE_GREATER
    || f_data.f_type == NODE_LESS_EQUAL
    || f_data.f_type == NODE_GREATER_EQUAL
    || f_data.f_type == NODE_IS
    || f_data.f_type == NODE_AS
    || f_data.f_type == NODE_MATCH
    || f_data.f_type == NODE_IN
    || f_data.f_type == NODE_INSTANCEOF) {
        NodePtr left = node;

        node_t type = f_data.f_type;
        node.CreateNode(type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        ShiftExpression(right);

        node.AddChild(left);
        node.AddChild(right);

        if(type == NODE_IN
        && (f_data.f_type == NODE_RANGE || f_data.f_type == NODE_REST)) {
            GetToken();
            ShiftExpression(right);
            node.AddChild(right);
        }
    }
}


void IntParser::ShiftExpression(NodePtr& node)
{
    AdditiveExpression(node);

    while(f_data.f_type == NODE_SHIFT_LEFT
    || f_data.f_type == NODE_SHIFT_RIGHT
    || f_data.f_type == NODE_SHIFT_RIGHT_UNSIGNED
    || f_data.f_type == NODE_ROTATE_LEFT
    || f_data.f_type == NODE_ROTATE_RIGHT) {
        NodePtr left = node;

        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        AdditiveExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::AdditiveExpression(NodePtr& node)
{
    MultiplicativeExpression(node);

    while(f_data.f_type == '+'
    || f_data.f_type == '-') {
        NodePtr left = node;

        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        MultiplicativeExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::MultiplicativeExpression(NodePtr& node)
{
    PowerExpression(node);

    while(f_data.f_type == '*'
    || f_data.f_type == '/'
    || f_data.f_type == '%') {
        NodePtr left = node;

        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        PowerExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}


void IntParser::PowerExpression(NodePtr& node)
{
    UnaryExpression(node);

    if(f_data.f_type == NODE_POWER) {
        NodePtr left = node;

        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();
        NodePtr right;
        PowerExpression(right);

        node.AddChild(left);
        node.AddChild(right);
    }
}



void IntParser::UnaryExpression(NodePtr& node)
{
    switch(f_data.f_type) {
    case NODE_DELETE:
    case NODE_INCREMENT:
    case NODE_DECREMENT:
    {
        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();
        NodePtr postfix;
        PostfixExpression(postfix);
        node.AddChild(postfix);
    }
        break;

    case NODE_VOID:
    case NODE_TYPEOF:
    case '+':
    case '-':
    case '~':
    case '!':
    {
        node.CreateNode(f_data.f_type);
        node.SetInputInfo(f_lexer.GetInput());

        GetToken();

        NodePtr unary;
        UnaryExpression(unary);

        node.AddChild(unary);
    }
        break;

    default:
        PostfixExpression(node);
        break;

    }
}


void IntParser::PostfixExpression(NodePtr& node)
{
    PrimaryExpression(node);

    for(;;) {
        switch(f_data.f_type) {
        case '.':
        {
            NodePtr left = node;

            node.CreateNode(NODE_MEMBER);
            node.SetInputInfo(f_lexer.GetInput());

            GetToken();
            NodePtr right;
            PrimaryExpression(right);

            node.AddChild(left);
            node.AddChild(right);
        }
            break;

        case NODE_SCOPE:
        {
            GetToken();
            if(f_data.f_type == NODE_IDENTIFIER) {
                NodePtr left = node;

                node.CreateNode(NODE_SCOPE);
                node.SetInputInfo(f_lexer.GetInput());

                NodePtr right;
                right.CreateNode();
                right.SetInputInfo(f_lexer.GetInput());
                right.SetData(f_data);

                node.AddChild(left);
                node.AddChild(right);

                GetToken();
            }
            else {
                f_lexer.ErrMsg(AS_ERR_INVALID_SCOPE, "'::' is expected to be followed by an identifier");
            }
            // don't repeat scope (it seems)
            return;
        }
            break;

        case NODE_INCREMENT:
        {
            NodePtr left = node;

            node.CreateNode(NODE_POST_INCREMENT);
            node.SetInputInfo(f_lexer.GetInput());

            GetToken();

            node.AddChild(left);
        }
            break;

        case NODE_DECREMENT:
        {
            NodePtr left = node;

            node.CreateNode(NODE_POST_DECREMENT);
            node.SetInputInfo(f_lexer.GetInput());

            GetToken();

            node.AddChild(left);
        }
            break;

        case '(':        // arguments
        {
            NodePtr left = node;

            node.CreateNode(NODE_CALL);
            node.SetInputInfo(f_lexer.GetInput());

            GetToken();

            node.AddChild(left);

            // any arguments?
            NodePtr right;
            if(f_data.f_type != ')') {
                NodePtr list;
                ListExpression(list, true, false);
                Data& data = list.GetData();
                if(data.f_type == NODE_LIST) {
                    right = list;
                }
                else {
                    right.CreateNode(NODE_LIST);
                    right.SetInputInfo(f_lexer.GetInput());
                    right.AddChild(list);
                }
            }
            else {
                // an empty list!
                right.CreateNode(NODE_LIST);
                right.SetInputInfo(f_lexer.GetInput());
            }
            node.AddChild(right);

            if(f_data.f_type == ')') {
                GetToken();
            }
            else {
                f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to end the list of arguments");
            }
        }
            break;

        case '[':        // array/property access
        {
            NodePtr left = node;

            // NOTE: this could be NODE_MEMBER in most
            //     cases however the NODE_ARRAY supports
            //     lists (including the empty list)
            //     which NODE_MEMBER doesn't
            node.CreateNode(NODE_ARRAY);
            node.SetInputInfo(f_lexer.GetInput());

            GetToken();

            node.AddChild(left);

            // any arguments?
            if(f_data.f_type != ']') {
                NodePtr right;
                ListExpression(right, false, false);
                node.AddChild(right);
            }

            if(f_data.f_type == ']') {
                GetToken();
            }
            else {
                f_lexer.ErrMsg(AS_ERR_SQUARE_BRAKETS_EXPECTED, "']' expected to end the list of element references");
            }
        }
            break;

        default:
            return;

        }
    }
}


void IntParser::PrimaryExpression(NodePtr& node)
{
    switch(f_data.f_type) {
    case NODE_NULL:
    case NODE_UNDEFINED:
    case NODE_TRUE:
    case NODE_FALSE:
    case NODE_IDENTIFIER:
    case NODE_INT64:
    case NODE_FLOAT64:
    case NODE_STRING:
    case NODE_THIS:
    case NODE_REGULAR_EXPRESSION:
    case NODE_PUBLIC:
    case NODE_PRIVATE:
    {
        node.CreateNode();
        node.SetInputInfo(f_lexer.GetInput());
        node.SetData(f_data);
        GetToken();
    }
        break;

    case NODE_NEW:
    {
        node.CreateNode(NODE_NEW);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();
        NodePtr object;
        PostfixExpression(object);
        node.AddChild(object);
    }
        break;

    case NODE_SUPER:
    {
        node.CreateNode(NODE_SUPER);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();
    }
        break;

    case '(':
    {
        GetToken();
        ListExpression(node, false, false);
        Data& d = node.GetData();
        // NOTE: the following is important in different cases
        // such as (a).field which is dynamic (i.e. we get the
        // content of variable a as the name of the object to
        // access and thus it is not equivalent to a.field)
        if(d.f_type == NODE_IDENTIFIER) {
            d.f_type = NODE_VIDENTIFIER;
        }
        if(f_data.f_type == ')') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected to match the '('");
        }
    }
        break;

    case '[':
    {
        node.CreateNode(NODE_ARRAY_LITERAL);
        node.SetInputInfo(f_lexer.GetInput());
        GetToken();

        NodePtr elements;
        ListExpression(elements, false, true);
        node.AddChild(elements);
        if(f_data.f_type == ']') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_SQUARE_BRAKETS_EXPECTED, "']' expected to match the '[' of this array");
        }
    }
        break;

    case '{':
    {
        GetToken();
        ObjectLiteralExpression(node);
        if(f_data.f_type == '}') {
            GetToken();
        }
        else {
            f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'}' expected to match the '{' of this object literal");
        }
    }
        break;

    case NODE_FUNCTION:
    {
        GetToken();
        Function(node, true);
    }
        break;

    default:
        f_lexer.ErrMsg(AS_ERR_INVALID_EXPRESSION, "unexpected token found in an expression");
        break;

    }
}



void IntParser::ObjectLiteralExpression(NodePtr& node)
{
    NodePtr        name;
    node_t        type;

    node.CreateNode(NODE_OBJECT_LITERAL);
    node.SetInputInfo(f_lexer.GetInput());
    for(;;) {
        name.CreateNode(NODE_TYPE);
        name.SetInputInfo(f_lexer.GetInput());
        switch(type = f_data.f_type) {
        case '(':
        {
            // We keep the '(' so an identifier becomes
            // a VIDENTIFIER and thus remains dynamic.
            //GetToken();
            NodePtr type;
            Expression(type);
            name.AddChild(type);
        }
            goto and_scope;

        case NODE_IDENTIFIER:    // <name> or <namespace>::<name>
        case NODE_PRIVATE:    // private::<name> only
        case NODE_PUBLIC:    // public::<name> only
            // NOTE: an IDENTIFIER here remains NODE_IDENTIFIER
            // so it doesn't look like the previous expression
            // (i.e. an expression literal can be just an
            // identifier but it will be marked as
            // NODE_VIDENTIFIER instead)
            name.SetData(f_data);
            GetToken();
and_scope:
            if(f_data.f_type == NODE_SCOPE) {
                GetToken();
                if(f_data.f_type == NODE_IDENTIFIER) {
                    NodePtr scope;
                    scope.CreateNode();
                    scope.SetInputInfo(f_lexer.GetInput());
                    scope.SetData(f_data);
                    name.AddChild(scope);
                }
                else {
                    f_lexer.ErrMsg(AS_ERR_INVALID_SCOPE, "'::' is expected to be followed by an identifier");
                }
            }
            else if(type != NODE_IDENTIFIER) {
                f_lexer.ErrMsg(AS_ERR_INVALID_FIELD_NAME, "'public' or 'private' cannot be used as a field name, '::' was expected");
            }
            break;

        case NODE_INT64:
        case NODE_FLOAT64:
        case NODE_STRING:
            name.SetData(f_data);
            GetToken();
            break;

        default:
            f_lexer.ErrMsg(AS_ERR_INVALID_FIELD, "the name of a field was expected");
            break;

        }

        if(f_data.f_type == ':') {
            GetToken();
        }
        else {
            // if we have a closing brace here, the programmer
            // tried to end his list with a comma; we just
            // accept that one silently! (like in C/C++)
            if(f_data.f_type == '}') {
                break;
            }

            f_lexer.ErrMsg(AS_ERR_COLON_EXPECTED, "':' expected after the name of a field");
            if(f_data.f_type == ';') {
                // this is probably the end...
                return;
            }

            // if we have a comma here, the programmer
            // just forgot a few things...
            if(f_data.f_type == ',') {
                GetToken();
                // we accept a comma at the end here too!
                if(f_data.f_type == '}'
                || f_data.f_type == ';') {
                    break;
                }
                continue;
            }
        }

        // add the name only now so we have a mostly
        // valid tree from here on
        node.AddChild(name);

        NodePtr value;
        AssignmentExpression(value);
        node.AddChild(value);

        if(f_data.f_type != ',') {
            break;
        }
        GetToken();
    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
