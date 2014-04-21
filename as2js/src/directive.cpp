/* directive.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER DIRECTIVES  **********************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Attributes(NodePtr& node)
{
    // Attributes are read first.
    // Depending on what follows the first set of attributes
    // we can determine what we've got (expression, statement,
    // etc.)
    // There can be no attribute and the last IDENTIFIER may
    // not be an attribute, also...
    for(;;) {
        switch(f_data.f_type) {
        case NODE_FALSE:
        case NODE_IDENTIFIER:
        case NODE_PRIVATE:
        case NODE_PUBLIC:
        case NODE_TRUE:
            break;

        default:
            return;

        }

        if(!node.HasNode()) {
            node.CreateNode(NODE_ATTRIBUTES);
            node.SetInputInfo(f_lexer.GetInput());
        }

        NodePtr attr;
        attr.CreateNode();
        attr.SetInputInfo(f_lexer.GetInput());
        attr.SetData(f_data);
        node.AddChild(attr);
        GetToken();
    }
}




void IntParser::DirectiveList(NodePtr& node)
{
    node.CreateNode(NODE_DIRECTIVE_LIST);
    node.SetInputInfo(f_lexer.GetInput());
    for(;;) {
        // skip empty statements quickly
        while(f_data.f_type == ';') {
            GetToken();
        }

        if(f_data.f_type == NODE_EOF
        || f_data.f_type == NODE_ELSE
        || f_data.f_type == '}') {
            return;
        }

        Directive(node);
    }
}


void IntParser::Directive(NodePtr& node)
{
    // we expect node to be a list of directives already
    // when defined (see DirectiveList())
    if(!node.HasNode()) {
        node.CreateNode(NODE_DIRECTIVE_LIST);
        node.SetInputInfo(f_lexer.GetInput());
    }

    // read attributes (identifiers, public/private, true/false)
    // if we find attributes and the directive accepts them,
    // then they are added to the directive as the last entry
    NodePtr attr_list;
    Attributes(attr_list);
    long attr_count = attr_list.GetChildCount();

    node_t type = f_data.f_type;
    NodePtr last_attr;

    // depending on the following token, we may want to restore
    // the last attribute (if it is an identifier)
    switch(type) {
    case ':':
        if(attr_count == 0) {
            f_lexer.ErrMsg(AS_ERR_INVALID_OPERATOR, "unexpected ':' without an identifier");
            break;
        }
        last_attr = attr_list.GetChild(attr_count - 1);
        if(last_attr.GetData().f_type != NODE_IDENTIFIER) {
            f_lexer.ErrMsg(AS_ERR_INVALID_OPERATOR, "unexpected ':' without an identifier");
            break;
        }
        /*FALLTHROUGH*/
    case NODE_AS:
    case NODE_ASSIGNMENT:
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
    case NODE_CONDITIONAL:
    case NODE_DECREMENT:
    case NODE_EQUAL:
    case NODE_GREATER_EQUAL:
    case NODE_IMPLEMENTS:
    case NODE_INSTANCEOF:
    case NODE_IN:
    case NODE_INCREMENT:
    case NODE_IS:
    case NODE_LESS_EQUAL:
    case NODE_LOGICAL_AND:
    case NODE_LOGICAL_OR:
    case NODE_LOGICAL_XOR:
    case NODE_MATCH:
    case NODE_MAXIMUM:
    case NODE_MEMBER:
    case NODE_MINIMUM:
    case NODE_NOT_EQUAL:
    case NODE_POWER:
    case NODE_PRIVATE:
    case NODE_PUBLIC:
    case NODE_RANGE:
    case NODE_REST:
    case NODE_ROTATE_LEFT:
    case NODE_ROTATE_RIGHT:
    case NODE_SCOPE:
    case NODE_SHIFT_LEFT:
    case NODE_SHIFT_RIGHT:
    case NODE_SHIFT_RIGHT_UNSIGNED:
    case NODE_STRICTLY_EQUAL:
    case NODE_STRICTLY_NOT_EQUAL:
    case '*':
    case '/':
    case ',':
    case '%':
    case '&':
    case '^':
    case '|':
    case '<':
    case '>':
    case '+':
    case '-':
    case '(':
    case ';':
    case '[':
        if(attr_count > 0) {
            last_attr = attr_list.GetChild(attr_count - 1);
            UngetToken(f_data);
            f_data = last_attr.GetData();
            --attr_count;
            attr_list.DeleteChild(attr_count);
            if(type != ':') {
                type = last_attr.GetData().f_type;
            }
        }
        break;

    default:
        // do nothing here
        break;

    }

    // we have a special case where a USE can be
    // followed by NAMESPACE vs. an identifier.
    // (i.e. use a namespace or define a pragma)
    if(type == NODE_USE) {
        GetToken();
    }

    // check for directives which can't have attributes
    if(attr_count > 0) {
        switch(type) {
        case NODE_USE:
            if(f_data.f_type == NODE_NAMESPACE) {
                break;
            }
            /*FALLTHROUGH*/
            // pragma can't be annotated
        case NODE_ARRAY_LITERAL:
        case NODE_BREAK:
        case NODE_CONTINUE:
        case NODE_CASE:
        case NODE_CATCH:
        case NODE_DEFAULT:
        case NODE_DO:
        case NODE_FOR:
        case NODE_FINALLY:
        case NODE_GOTO:
        case NODE_IF:
        case NODE_RETURN:
        case NODE_SWITCH:
        case NODE_THROW:
        case NODE_TRY:
        case NODE_WITH:
        case NODE_WHILE:
        case NODE_DECREMENT:
        case NODE_DELETE:
        case NODE_FLOAT64:
        case NODE_IDENTIFIER:
        case NODE_INCREMENT:
        case NODE_INT64:
        case NODE_NEW:
        case NODE_NULL:
        case NODE_OBJECT_LITERAL:
        case NODE_UNDEFINED:
        case NODE_REGULAR_EXPRESSION:
        case NODE_STRING:
        case NODE_SUPER:    // will accept commas too even in expressions
        case NODE_THIS:
        case NODE_TYPEOF:
        case NODE_VIDENTIFIER:
        case NODE_VOID:
        case '!':
        case '+':
        case '-':
        case '(':
        case '[':
        case '~':
        case ':':
        case ';':
            // annotated empty statements are not allowed
            f_lexer.ErrMsg(AS_ERR_INVALID_ATTRIBUTES, "no attributes were expected here (statements, expressions and pragmas can't be annotated)");
            attr_list.ClearNode();
            attr_count = 0;
            break;

        // everything else can be annotated
        default:
            break;

        }
    }

    // The directive node, if created by a sub-function, will
    // be added to the list of directives.
    NodePtr directive;
    switch(type) {
    // *** PRAGMA ***
    case NODE_USE:
        // we alread did a GetToken() to skip the NODE_USE
        if(f_data.f_type == NODE_NAMESPACE) {
            // use namespace ... ';'
            GetToken();
            UseNamespace(directive);
            break;
        }
        // TODO? Pragmas are not part of the tree
        // Note: pragmas affect the Options and
        // are not currently added to the final
        // tree of nodes. [is that correct?! it
        // should be as long as we don't have
        // run-time pragmas]
        Pragma();
        break;

    // *** PACKAGE ***
    case NODE_PACKAGE:
        GetToken();
        Package(directive);
        break;

    case NODE_IMPORT:
        GetToken();
        Import(directive);
        break;

    // *** CLASS DEFINITION ***
    case NODE_CLASS:
    case NODE_INTERFACE:
        GetToken();
        Class(directive, type);
        break;

    case NODE_ENUM:
        GetToken();
        Enum(directive);
        break;

    // *** FUNCTION DEFINITION ***
    case NODE_FUNCTION:
        GetToken();
        Function(directive, false);
        break;

    // *** VARIABLE DEFINITION ***
    case NODE_CONST:
        GetToken();
        if(f_data.f_type == NODE_VAR) {
            GetToken();
        }
        Variable(directive, true);
        break;

    case NODE_VAR:
        GetToken();
        Variable(directive, false);
        break;

    // *** STATEMENT ***
    case NODE_OPEN_CURVLY_BRACKET:
        GetToken();
        Block(directive);
        break;

    case ';':
        // empty statements are just skipped
        //
        // NOTE: we reach here only when we find attributes
        //     which aren't identifiers and this means
        //     we will have gotten an error.
        GetToken();
        break;

    case NODE_BREAK:
    case NODE_CONTINUE:
        GetToken();
        BreakContinue(directive, type);
        break;

    case NODE_CASE:
        GetToken();
        Case(directive);
        break;

    case NODE_CATCH:
        GetToken();
        Catch(directive);
        break;

    case NODE_DEFAULT:
        GetToken();
        Default(directive);
        break;

    case NODE_DO:
        GetToken();
        Do(directive);
        break;

    case NODE_FOR:
        GetToken();
        For(directive);
        break;

    case NODE_FINALLY:
    case NODE_TRY:
        GetToken();
        TryFinally(directive, type);
        break;

    case NODE_GOTO:
        GetToken();
        Goto(directive);
        break;

    case NODE_IF:
        GetToken();
        If(directive);
        break;

    case NODE_NAMESPACE:
        GetToken();
        Namespace(directive);
        break;

    case NODE_RETURN:
        GetToken();
        Return(directive);
        break;

    case NODE_SWITCH:
        GetToken();
        Switch(directive);
        break;

    case NODE_THROW:
        GetToken();
        Throw(directive);
        break;

    case NODE_WITH:
    case NODE_WHILE:
        GetToken();
        WithWhile(directive, type);
        break;

    case ':':
        // the label was the last identifier in the
        // attributes which is now in f_data
        directive.CreateNode();
        directive.SetInputInfo(f_lexer.GetInput());
        f_data.f_type = NODE_LABEL;
        directive.SetData(f_data);
        // we skip the identifier here
        GetToken();
        // and then the ':'
        GetToken();
        break;

    // *** EXPRESSION ***
    case NODE_ARRAY_LITERAL:
    case NODE_DECREMENT:
    case NODE_DELETE:
    case NODE_FALSE:
    case NODE_FLOAT64:
    case NODE_IDENTIFIER:
    case NODE_INCREMENT:
    case NODE_INT64:
    case NODE_NEW:
    case NODE_NULL:
    case NODE_OBJECT_LITERAL:
    case NODE_PRIVATE:
    case NODE_PUBLIC:
    case NODE_UNDEFINED:
    case NODE_REGULAR_EXPRESSION:
    case NODE_STRING:
    case NODE_SUPER:    // will accept commas too even in expressions
    case NODE_THIS:
    case NODE_TRUE:
    case NODE_TYPEOF:
    case NODE_VIDENTIFIER:
    case NODE_VOID:
    case '!':
    case '+':
    case '-':
    case '(':
    case '[':
    case '~':
        Expression(directive);
        break;

    // *** TERMINATOR ***
    case NODE_EOF:
    case '}':
        return;

    // *** INVALID ***
    // The following are for sure invalid tokens in this
    // context. If it looks like some of these could be
    // valid when this function returns, just comment
    // out the corresponding case.
    case NODE_AS:
    case NODE_ASSIGNMENT:
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
    case NODE_CONDITIONAL:
    case NODE_EQUAL:
    case NODE_GREATER_EQUAL:
    case NODE_IMPLEMENTS:
    case NODE_INSTANCEOF:
    case NODE_IN:
    case NODE_IS:
    case NODE_LESS_EQUAL:
    case NODE_LOGICAL_AND:
    case NODE_LOGICAL_OR:
    case NODE_LOGICAL_XOR:
    case NODE_MATCH:
    case NODE_MAXIMUM:
    case NODE_MEMBER:
    case NODE_MINIMUM:
    case NODE_NOT_EQUAL:
    case NODE_POWER:
    case NODE_RANGE:
    case NODE_REST:
    case NODE_ROTATE_LEFT:
    case NODE_ROTATE_RIGHT:
    case NODE_SCOPE:
    case NODE_SHIFT_LEFT:
    case NODE_SHIFT_RIGHT:
    case NODE_SHIFT_RIGHT_UNSIGNED:
    case NODE_STRICTLY_EQUAL:
    case NODE_STRICTLY_NOT_EQUAL:
    case NODE_VARIABLE:
    case ')':
    case '*':
    case '/':
    case ',':
    case '%':
    case '&':
    case '^':
    case '|':
    case '<':
    case '>':
    case ']':
        f_lexer.ErrMsg(AS_ERR_INVALID_OPERATOR, "unexpected operator");
        GetToken();
        break;

    case NODE_DEBUGGER:    // just not handled yet...
    case NODE_ELSE:
    case NODE_EXTENDS:
        f_lexer.ErrMsg(AS_ERR_INVALID_KEYWORD, "unexpected keyword");
        GetToken();
        break;

    // *** NOT POSSIBLE ***
    // These should never happen since they should be caught
    // before this switch is reached or it can't be a node
    // read by the lexer.
    case NODE_ARRAY:
    case NODE_ATTRIBUTES:
    case NODE_AUTO:
    case NODE_CALL:
    case NODE_DIRECTIVE_LIST:
    case NODE_EMPTY:
    case NODE_ENTRY:
    case NODE_EXCLUDE:
    case NODE_FOR_IN:    // maybe this should be a terminator?
    case NODE_INCLUDE:
    case NODE_LABEL:
    case NODE_LIST:
    case NODE_MASK:
    case NODE_NAME:
    case NODE_PARAM:
    case NODE_PARAMETERS:
    case NODE_PARAM_MATCH:
    case NODE_POST_DECREMENT:
    case NODE_POST_INCREMENT:
    case NODE_PROGRAM:
    case NODE_ROOT:
    case NODE_SET:
    case NODE_TYPE:
    case NODE_UNKNOWN:    // ?!
    case NODE_VAR_ATTRIBUTES:
    case NODE_other:    // no node should be of this type
    case NODE_max:        // no node should be of this type
        fprintf(stderr, "INTERNAL ERROR: invalid node (%d) in directive_list.\n", type);
        AS_ASSERT(0);
        break;

    }
    if(directive.HasNode()) {
        if(attr_list.GetChildCount() > 0) {
            directive.SetLink(NodePtr::LINK_ATTRIBUTES, attr_list);
        }
        node.AddChild(directive);
    }

    // Now make sure we have a semicolon for
    // those statements which have to have it.
    switch(type) {
    case NODE_ARRAY_LITERAL:
    case NODE_BREAK:
    case NODE_CONST:
    case NODE_CONTINUE:
    case NODE_DECREMENT:
    case NODE_DELETE:
    case NODE_DO:
    case NODE_FLOAT64:
    case NODE_GOTO:
    case NODE_IDENTIFIER:
    case NODE_IMPORT:
    case NODE_INCREMENT:
    case NODE_INT64:
    case NODE_NAMESPACE:
    case NODE_NEW:
    case NODE_NULL:
    case NODE_OBJECT_LITERAL:
    case NODE_RETURN:
    case NODE_REGULAR_EXPRESSION:
    case NODE_STRING:
    case NODE_SUPER:
    case NODE_THIS:
    case NODE_THROW:
    case NODE_TYPEOF:
    case NODE_UNDEFINED:
    case NODE_USE:
    case NODE_VAR:
    case NODE_VIDENTIFIER:
    case NODE_VOID:
    case '!':
    case '+':
    case '-':
    case '(':
    case '[':
    case '~':
        // accept missing ';' when we find a '}' next
        if(f_data.f_type != ';' && f_data.f_type != '}') {
            f_lexer.ErrMsg(AS_ERR_SEMICOLON_EXPECTED, "';' was expected");
        }
        while(f_data.f_type != ';'
        && f_data.f_type != '}'
        && f_data.f_type != NODE_ELSE
        && f_data.f_type != NODE_EOF) {
            GetToken();
        }
        // we need to skip one semi-color here
        // in case we're not in a DirectiveList()
        if(f_data.f_type == ';') {
            GetToken();
        }
        break;

    default:
        break;

    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
