/* directive.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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
#include    "as2js/exceptions.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER DIRECTIVES  **********************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::attributes(Node::pointer_t& node)
{
    // Attributes are read first.
    // Depending on what follows the first set of attributes
    // we can determine what we've got (expression, statement,
    // etc.)
    // There can be no attribute and the last IDENTIFIER may
    // not be an attribute, also...
    for(;;)
    {
        switch(f_node->get_type())
        {
        case Node::NODE_FALSE:
        case Node::NODE_IDENTIFIER:
        case Node::NODE_PRIVATE:
        case Node::NODE_PUBLIC:
        case Node::NODE_TRUE:
            // TODO: Check that we don't find the same type twice...
            //       We may also want to enforce an order in some cases?
            break;

        default:
            return;

        }

        // TBD: necessary test?
        //      (this function gets called from many places... so watch out)
        if(!node)
        {
            node = f_lexer->get_new_node(Node::NODE_ATTRIBUTES);
        }

        node->append_child(f_node);
        get_token();
    }
}




void Parser::directive_list(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::NODE_DIRECTIVE_LIST);
    for(;;)
    {
        // skip empty statements quickly
        while(f_node->get_type() == Node::NODE_SEMICOLON)
        {
            get_token();
        }

        switch(f_node->get_type())
        {
        case Node::NODE_EOF:
        case Node::NODE_ELSE:
        case Node::NODE_CLOSE_CURVLY_BRACKET:
            // these end the list of directives
            return;

        default:
            directive(node);
            break;

        }
    }
}


void Parser::directive(Node::pointer_t& node)
{
    // we expect node to be a list of directives already
    // when defined (see directive_list())
    if(!node)
    {
        node = f_lexer->get_new_node(Node::NODE_DIRECTIVE_LIST);
    }

    // read attributes (identifiers, public/private, true/false)
    // if we find attributes and the directive accepts them,
    // then they are added to the directive as the last entry
    Node::pointer_t attr_list;
    attributes(attr_list);
    size_t attr_count(attr_list->get_children_size());
    Node::node_t type(f_node->get_type());
    Node::pointer_t last_attr;

    // depending on the following token, we may want to restore
    // the last attribute (if it is an identifier)
    switch(type)
    {
    case Node::NODE_COLON:
        if(attr_count == 0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
            msg << "unexpected ':' without an identifier";
            break;
        }
        last_attr = attr_list->get_child(attr_count - 1);
        if(last_attr->get_type() != Node::NODE_IDENTIFIER)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
            msg << "unexpected ':' without an identifier";
            break;
        }
        /*FALLTHROUGH*/
    case Node::NODE_AS:
    case Node::NODE_ASSIGNMENT:
    case Node::NODE_ASSIGNMENT_ADD:
    case Node::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::NODE_ASSIGNMENT_DIVIDE:
    case Node::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::NODE_ASSIGNMENT_MAXIMUM:
    case Node::NODE_ASSIGNMENT_MINIMUM:
    case Node::NODE_ASSIGNMENT_MODULO:
    case Node::NODE_ASSIGNMENT_MULTIPLY:
    case Node::NODE_ASSIGNMENT_POWER:
    case Node::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_ASSIGNMENT_SUBTRACT:
    case Node::NODE_CONDITIONAL:
    case Node::NODE_DECREMENT:
    case Node::NODE_EQUAL:
    case Node::NODE_GREATER_EQUAL:
    case Node::NODE_IMPLEMENTS:
    case Node::NODE_INSTANCEOF:
    case Node::NODE_IN:
    case Node::NODE_INCREMENT:
    case Node::NODE_IS:
    case Node::NODE_LESS_EQUAL:
    case Node::NODE_LOGICAL_AND:
    case Node::NODE_LOGICAL_OR:
    case Node::NODE_LOGICAL_XOR:
    case Node::NODE_MATCH:
    case Node::NODE_MAXIMUM:
    case Node::NODE_MEMBER:
    case Node::NODE_MINIMUM:
    case Node::NODE_NOT_EQUAL:
    case Node::NODE_POWER:
    case Node::NODE_PRIVATE:
    case Node::NODE_PUBLIC:
    case Node::NODE_RANGE:
    case Node::NODE_REST:
    case Node::NODE_ROTATE_LEFT:
    case Node::NODE_ROTATE_RIGHT:
    case Node::NODE_SCOPE:
    case Node::NODE_SHIFT_LEFT:
    case Node::NODE_SHIFT_RIGHT:
    case Node::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_STRICTLY_EQUAL:
    case Node::NODE_STRICTLY_NOT_EQUAL:
    case Node::NODE_MULTIPLY:
    case Node::NODE_DIVIDE:
    case Node::NODE_COMMA:
    case Node::NODE_MODULO:
    case Node::NODE_BITWISE_AND:
    case Node::NODE_BITWISE_XOR:
    case Node::NODE_BITWISE_OR:
    case Node::NODE_LESS:
    case Node::NODE_GREATER:
    case Node::NODE_ADD:
    case Node::NODE_SUBTRACT:
    case Node::NODE_OPEN_PARENTHESIS:
    case Node::NODE_SEMICOLON:
    case Node::NODE_OPEN_SQUARE_BRACKET:
        if(attr_count > 0)
        {
            last_attr = attr_list->get_child(attr_count - 1);
            unget_token(f_node);
            --attr_count;
            attr_list->delete_child(attr_count);
            if(type != Node::NODE_COLON)
            {
                type = last_attr->get_type();
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
    if(type == Node::NODE_USE)
    {
        get_token();
        // Note that we do not change the variable 'type' here!
    }

    // check for directives which can't have attributes
    if(attr_count > 0)
    {
        switch(type)
        {
        case Node::NODE_USE:
            if(f_node->get_type() == Node::NODE_NAMESPACE)
            {
                break;
            }
            /*FALLTHROUGH*/
            // pragma can't be annotated
        case Node::NODE_ARRAY_LITERAL:
        case Node::NODE_BREAK:
        case Node::NODE_CONTINUE:
        case Node::NODE_CASE:
        case Node::NODE_CATCH:
        case Node::NODE_DEFAULT:
        case Node::NODE_DO:
        case Node::NODE_FOR:
        case Node::NODE_FINALLY:
        case Node::NODE_GOTO:
        case Node::NODE_IF:
        case Node::NODE_RETURN:
        case Node::NODE_SWITCH:
        case Node::NODE_THROW:
        case Node::NODE_TRY:
        case Node::NODE_WITH:
        case Node::NODE_WHILE:
        case Node::NODE_DECREMENT:
        case Node::NODE_DELETE:
        case Node::NODE_FLOAT64:
        case Node::NODE_IDENTIFIER:
        case Node::NODE_INCREMENT:
        case Node::NODE_INT64:
        case Node::NODE_NEW:
        case Node::NODE_NULL:
        case Node::NODE_OBJECT_LITERAL:
        case Node::NODE_UNDEFINED:
        case Node::NODE_REGULAR_EXPRESSION:
        case Node::NODE_STRING:
        case Node::NODE_SUPER:    // will accept commas too even in expressions
        case Node::NODE_THIS:
        case Node::NODE_TYPEOF:
        case Node::NODE_VIDENTIFIER:
        case Node::NODE_VOID:
        case Node::NODE_LOGICAL_NOT:
        case Node::NODE_ADD:
        case Node::NODE_SUBTRACT:
        case Node::NODE_OPEN_PARENTHESIS:
        case Node::NODE_OPEN_SQUARE_BRACKET:
        case Node::NODE_BITWISE_NOT:
        case Node::NODE_COLON:
        case Node::NODE_SEMICOLON:
        {
            // annotated empty statements are not allowed
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_input()->get_position());
            msg << "no attributes were expected here (statements, expressions and pragmas can't be annotated)";
            attr_list.reset();
            attr_count = 0;
        }
            break;

        // everything else can be annotated
        default:
            break;

        }
    }

    // The directive node, if created by a sub-function, will
    // be added to the list of directives.
    Node::pointer_t directive_node;
    switch(type)
    {
    // *** PRAGMA ***
    case Node::NODE_USE:
        // we alread did a GetToken() to skip the NODE_USE
        if(f_node->get_type() == Node::NODE_NAMESPACE)
        {
            // use namespace ... ';'
            get_token();
            use_namespace(directive_node);
            break;
        }
        // TODO? Pragmas are not part of the tree
        //
        // Note: pragmas affect the Options and are
        //       not currently added to the final
        //       tree of nodes. [is that correct?! it
        //       should be fine as long as we do not
        //       have run-time pragmas]
        pragma();
        break;

    // *** PACKAGE ***
    case Node::NODE_PACKAGE:
        get_token();
        package(directive_node);
        break;

    case Node::NODE_IMPORT:
        get_token();
        import(directive_node);
        break;

    // *** CLASS DEFINITION ***
    case Node::NODE_CLASS:
    case Node::NODE_INTERFACE:
        get_token();
        class_declaration(directive_node, type);
        break;

    case Node::NODE_ENUM:
        get_token();
        enum_declaration(directive_node);
        break;

    // *** FUNCTION DEFINITION ***
    case Node::NODE_FUNCTION:
        get_token();
        function(directive_node, false);
        break;

    // *** VARIABLE DEFINITION ***
    case Node::NODE_CONST:
        get_token();
        if(f_node->get_type() == Node::NODE_VAR)
        {
            get_token();
        }
        variable(directive_node, true);
        break;

    case Node::NODE_VAR:
        get_token();
        variable(directive_node, false);
        break;

    // *** STATEMENT ***
    case Node::NODE_OPEN_CURVLY_BRACKET:
        get_token();
        block(directive_node);
        break;

    case Node::NODE_SEMICOLON:
        // empty statements are just skipped
        //
        // NOTE: we reach here only when we find attributes
        //     which aren't identifiers and this means
        //     we will have gotten an error.
        get_token();
        break;

    case Node::NODE_BREAK:
    case Node::NODE_CONTINUE:
        get_token();
        break_continue(directive_node, type);
        break;

    case Node::NODE_CASE:
        get_token();
        case_directive(directive_node);
        break;

    case Node::NODE_CATCH:
        get_token();
        catch_directive(directive_node);
        break;

    case Node::NODE_DEFAULT:
        get_token();
        default_directive(directive_node);
        break;

    case Node::NODE_DO:
        get_token();
        do_directive(directive_node);
        break;

    case Node::NODE_FOR:
        get_token();
        for_directive(directive_node);
        break;

    case Node::NODE_FINALLY:
    case Node::NODE_TRY:
        get_token();
        try_finally(directive_node, type);
        break;

    case Node::NODE_GOTO:
        get_token();
        goto_directive(directive_node);
        break;

    case Node::NODE_IF:
        get_token();
        if_directive(directive_node);
        break;

    case Node::NODE_NAMESPACE:
        get_token();
        namespace_block(directive_node);
        break;

    case Node::NODE_RETURN:
        get_token();
        return_directive(directive_node);
        break;

    case Node::NODE_SWITCH:
        get_token();
        switch_directive(directive_node);
        break;

    case Node::NODE_THROW:
        get_token();
        throw_directive(directive_node);
        break;

    case Node::NODE_WITH:
    case Node::NODE_WHILE:
        get_token();
        with_while(directive_node, type);
        break;

    case Node::NODE_COLON:
        // the label was the last identifier in the
        // attributes which is now in f_node
        directive_node = f_node;
        // we skip the identifier here
        get_token();
        // and then the ':'
        get_token();
        break;

    // *** EXPRESSION ***
    case Node::NODE_ARRAY_LITERAL:
    case Node::NODE_DECREMENT:
    case Node::NODE_DELETE:
    case Node::NODE_FALSE:
    case Node::NODE_FLOAT64:
    case Node::NODE_IDENTIFIER:
    case Node::NODE_INCREMENT:
    case Node::NODE_INT64:
    case Node::NODE_NEW:
    case Node::NODE_NULL:
    case Node::NODE_OBJECT_LITERAL:
    case Node::NODE_PRIVATE:
    case Node::NODE_PUBLIC:
    case Node::NODE_UNDEFINED:
    case Node::NODE_REGULAR_EXPRESSION:
    case Node::NODE_STRING:
    case Node::NODE_SUPER:    // will accept commas too even in expressions
    case Node::NODE_THIS:
    case Node::NODE_TRUE:
    case Node::NODE_TYPEOF:
    case Node::NODE_VIDENTIFIER:
    case Node::NODE_VOID:
    case Node::NODE_LOGICAL_NOT:
    case Node::NODE_ADD:
    case Node::NODE_SUBTRACT:
    case Node::NODE_OPEN_PARENTHESIS:
    case Node::NODE_OPEN_SQUARE_BRACKET:
    case Node::NODE_BITWISE_NOT:
        expression(directive_node);
        break;

    // *** TERMINATOR ***
    case Node::NODE_EOF:
    case Node::NODE_CLOSE_CURVLY_BRACKET:
        return;

    // *** INVALID ***
    // The following are for sure invalid tokens in this
    // context. If it looks like some of these could be
    // valid when this function returns, just comment
    // out the corresponding case.
    case Node::NODE_AS:
    case Node::NODE_ASSIGNMENT:
    case Node::NODE_ASSIGNMENT_ADD:
    case Node::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::NODE_ASSIGNMENT_DIVIDE:
    case Node::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::NODE_ASSIGNMENT_MAXIMUM:
    case Node::NODE_ASSIGNMENT_MINIMUM:
    case Node::NODE_ASSIGNMENT_MODULO:
    case Node::NODE_ASSIGNMENT_MULTIPLY:
    case Node::NODE_ASSIGNMENT_POWER:
    case Node::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_ASSIGNMENT_SUBTRACT:
    case Node::NODE_CONDITIONAL:
    case Node::NODE_EQUAL:
    case Node::NODE_GREATER_EQUAL:
    case Node::NODE_IMPLEMENTS:
    case Node::NODE_INSTANCEOF:
    case Node::NODE_IN:
    case Node::NODE_IS:
    case Node::NODE_LESS_EQUAL:
    case Node::NODE_LOGICAL_AND:
    case Node::NODE_LOGICAL_OR:
    case Node::NODE_LOGICAL_XOR:
    case Node::NODE_MATCH:
    case Node::NODE_MAXIMUM:
    case Node::NODE_MEMBER:
    case Node::NODE_MINIMUM:
    case Node::NODE_NOT_EQUAL:
    case Node::NODE_POWER:
    case Node::NODE_RANGE:
    case Node::NODE_REST:
    case Node::NODE_ROTATE_LEFT:
    case Node::NODE_ROTATE_RIGHT:
    case Node::NODE_SCOPE:
    case Node::NODE_SHIFT_LEFT:
    case Node::NODE_SHIFT_RIGHT:
    case Node::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_STRICTLY_EQUAL:
    case Node::NODE_STRICTLY_NOT_EQUAL:
    case Node::NODE_VARIABLE:
    case Node::NODE_CLOSE_PARENTHESIS:
    case Node::NODE_MULTIPLY:
    case Node::NODE_DIVIDE:
    case Node::NODE_COMMA:
    case Node::NODE_MODULO:
    case Node::NODE_BITWISE_AND:
    case Node::NODE_BITWISE_XOR:
    case Node::NODE_BITWISE_OR:
    case Node::NODE_LESS:
    case Node::NODE_GREATER:
    case Node::NODE_CLOSE_SQUARE_BRACKET:
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
        msg << "unexpected operator";
        get_token();
    }
        break;

    case Node::NODE_DEBUGGER:    // just not handled yet...
    case Node::NODE_ELSE:
    case Node::NODE_EXTENDS:
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_KEYWORD, f_lexer->get_input()->get_position());
        msg << "unexpected keyword";
        get_token();
    }
        break;

    // *** NOT POSSIBLE ***
    // These should never happen since they should be caught
    // before this switch is reached or it can't be a node
    // read by the lexer.
    case Node::NODE_ARRAY:
    case Node::NODE_ATTRIBUTES:
    case Node::NODE_AUTO:
    case Node::NODE_CALL:
    case Node::NODE_DIRECTIVE_LIST:
    case Node::NODE_EMPTY:
    case Node::NODE_ENTRY:
    case Node::NODE_EXCLUDE:
    case Node::NODE_FOR_IN:    // maybe this should be a terminator?
    case Node::NODE_INCLUDE:
    case Node::NODE_LABEL:
    case Node::NODE_LIST:
    case Node::NODE_MASK:
    case Node::NODE_NAME:
    case Node::NODE_PARAM:
    case Node::NODE_PARAMETERS:
    case Node::NODE_PARAM_MATCH:
    case Node::NODE_POST_DECREMENT:
    case Node::NODE_POST_INCREMENT:
    case Node::NODE_PROGRAM:
    case Node::NODE_ROOT:
    case Node::NODE_SET:
    case Node::NODE_TYPE:
    case Node::NODE_UNKNOWN:    // ?!
    case Node::NODE_VAR_ATTRIBUTES:
    case Node::NODE_other:    // no node should be of this type
    case Node::NODE_max:        // no node should be of this type
    {
        Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_INTERNAL_ERROR, f_lexer->get_input()->get_position());
        msg << "INTERNAL ERROR: invalid node (" << Node::operator_to_string(type) << ") in directive_list.";
        throw exception_internal_error("unexpected node type found while parsing directives");
    }

    }
    if(directive_node)
    {
        // if there are attributes link them to the directive
        if(attr_list->get_children_size() > 0)
        {
            directive_node->set_link(Node::LINK_ATTRIBUTES, attr_list);
        }
        node->append_child(directive_node);
    }

    // Now make sure we have a semicolon for
    // those statements which have to have it.
    switch(type)
    {
    case Node::NODE_ARRAY_LITERAL:
    case Node::NODE_BREAK:
    case Node::NODE_CONST:
    case Node::NODE_CONTINUE:
    case Node::NODE_DECREMENT:
    case Node::NODE_DELETE:
    case Node::NODE_DO:
    case Node::NODE_FLOAT64:
    case Node::NODE_GOTO:
    case Node::NODE_IDENTIFIER:
    case Node::NODE_IMPORT:
    case Node::NODE_INCREMENT:
    case Node::NODE_INT64:
    case Node::NODE_NAMESPACE:
    case Node::NODE_NEW:
    case Node::NODE_NULL:
    case Node::NODE_OBJECT_LITERAL:
    case Node::NODE_RETURN:
    case Node::NODE_REGULAR_EXPRESSION:
    case Node::NODE_STRING:
    case Node::NODE_SUPER:
    case Node::NODE_THIS:
    case Node::NODE_THROW:
    case Node::NODE_TYPEOF:
    case Node::NODE_UNDEFINED:
    case Node::NODE_USE:
    case Node::NODE_VAR:
    case Node::NODE_VIDENTIFIER:
    case Node::NODE_VOID:
    case Node::NODE_LOGICAL_NOT:
    case Node::NODE_ADD:
    case Node::NODE_SUBTRACT:
    case Node::NODE_OPEN_PARENTHESIS:
    case Node::NODE_OPEN_SQUARE_BRACKET:
    case Node::NODE_BITWISE_NOT:
        // accept missing ';' when we find a '}' next
        if(f_node->get_type() != Node::NODE_SEMICOLON
        && f_node->get_type() != Node::NODE_CLOSE_CURVLY_BRACKET)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_input()->get_position());
            msg << "';' was expected";
        }
        // skip all that whatever up to the next end of this
        while(f_node->get_type() != Node::NODE_SEMICOLON
           && f_node->get_type() != Node::NODE_CLOSE_CURVLY_BRACKET
           && f_node->get_type() != Node::NODE_ELSE
           && f_node->get_type() != Node::NODE_EOF)
        {
            get_token();
        }
        // we need to skip one semi-color here
        // in case we're not in a directive_list()
        if(f_node->get_type() == Node::NODE_SEMICOLON)
        {
            get_token();
        }
        break;

    default:
        break;

    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
