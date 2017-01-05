/* parser_directive.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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
        case Node::node_t::NODE_ABSTRACT:
        case Node::node_t::NODE_FALSE:
        case Node::node_t::NODE_FINAL:
        case Node::node_t::NODE_IDENTIFIER:
        case Node::node_t::NODE_NATIVE:
        case Node::node_t::NODE_PRIVATE:
        case Node::node_t::NODE_PROTECTED:
        case Node::node_t::NODE_PUBLIC:
        case Node::node_t::NODE_STATIC:
        case Node::node_t::NODE_TRANSIENT:
        case Node::node_t::NODE_TRUE:
        case Node::node_t::NODE_VOLATILE:
            // TODO: Check that we don't find the same type twice...
            //       We may also want to enforce an order in some cases?
            break;

        default:
            return;

        }

        if(!node)
        {
            node = f_lexer->get_new_node(Node::node_t::NODE_ATTRIBUTES);
        }

        // at this point attributes are kept as nodes, the directive()
        // function saves them as a link in the node, later the compiler
        // transform them in actual NODE_ATTR_... flags
        node->append_child(f_node);
        get_token();
    }
}




void Parser::directive_list(Node::pointer_t& node)
{
    if(node)
    {
        // should not happen, if it does, we have got a really bad internal error
        throw exception_internal_error("directive_list() called with a non-null node pointer"); // LCOV_EXCL_LINE
    }

    node = f_lexer->get_new_node(Node::node_t::NODE_DIRECTIVE_LIST);
    for(;;)
    {
        // skip empty statements quickly
        while(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
        {
            get_token();
        }

        switch(f_node->get_type())
        {
        case Node::node_t::NODE_EOF:
        case Node::node_t::NODE_ELSE:
        case Node::node_t::NODE_CLOSE_CURVLY_BRACKET:
            // these end the list of directives
            return;

        default:
            directive(node);
            break;

        }
    }
    /*NOTREACHED*/
}


void Parser::directive(Node::pointer_t& node)
{
    // we expect node to be a list of directives already
    // when defined (see directive_list())
    if(!node)
    {
        node = f_lexer->get_new_node(Node::node_t::NODE_DIRECTIVE_LIST);
    }

    // read attributes (identifiers, public/private, true/false)
    // if we find attributes and the directive accepts them,
    // then they are added to the directive as the last entry
    Node::pointer_t attr_list;
    attributes(attr_list);
    size_t attr_count(attr_list ? attr_list->get_children_size() : 0);
    Node::pointer_t instruction_node(f_node);
    Node::node_t type(f_node->get_type());
    Node::pointer_t last_attr;

    // depending on the following token, we may want to restore
    // the last "attribute" (if it is an identifier)
    switch(type)
    {
    case Node::node_t::NODE_COLON:
        if(attr_count == 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
            msg << "unexpected ':' without an identifier.";
            // skip the spurious colon and return
            get_token();
            return;
        }
        last_attr = attr_list->get_child(attr_count - 1);
        if(last_attr->get_type() != Node::node_t::NODE_IDENTIFIER)
        {
            // special cases of labels in classes
            if(last_attr->get_type() != Node::node_t::NODE_PRIVATE
            && last_attr->get_type() != Node::node_t::NODE_PROTECTED
            && last_attr->get_type() != Node::node_t::NODE_PUBLIC)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
                msg << "unexpected ':' without a valid label.";
                // skip the spurious colon and return
                get_token();
                return;
            }
            last_attr->to_identifier();
        }
        /*FALLTHROUGH*/
    case Node::node_t::NODE_ADD:
    case Node::node_t::NODE_AS:
    case Node::node_t::NODE_ASSIGNMENT:
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
    case Node::node_t::NODE_BITWISE_AND:
    case Node::node_t::NODE_BITWISE_OR:
    case Node::node_t::NODE_BITWISE_XOR:
    case Node::node_t::NODE_COMMA:
    case Node::node_t::NODE_CONDITIONAL:
    case Node::node_t::NODE_DECREMENT:
    case Node::node_t::NODE_DIVIDE:
    case Node::node_t::NODE_EQUAL:
    case Node::node_t::NODE_GREATER:
    case Node::node_t::NODE_GREATER_EQUAL:
    case Node::node_t::NODE_IMPLEMENTS:
    case Node::node_t::NODE_INSTANCEOF:
    case Node::node_t::NODE_IN:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_IS:
    case Node::node_t::NODE_LESS:
    case Node::node_t::NODE_LESS_EQUAL:
    case Node::node_t::NODE_LOGICAL_AND:
    case Node::node_t::NODE_LOGICAL_OR:
    case Node::node_t::NODE_LOGICAL_XOR:
    case Node::node_t::NODE_MATCH:
    case Node::node_t::NODE_MAXIMUM:
    case Node::node_t::NODE_MEMBER:
    case Node::node_t::NODE_MINIMUM:
    case Node::node_t::NODE_MODULO:
    case Node::node_t::NODE_MULTIPLY:
    case Node::node_t::NODE_NOT_EQUAL:
    case Node::node_t::NODE_OPEN_PARENTHESIS:
    case Node::node_t::NODE_OPEN_SQUARE_BRACKET:
    case Node::node_t::NODE_POWER:
    case Node::node_t::NODE_PRIVATE:
    case Node::node_t::NODE_PUBLIC:
    case Node::node_t::NODE_RANGE:
    case Node::node_t::NODE_REST:
    case Node::node_t::NODE_ROTATE_LEFT:
    case Node::node_t::NODE_ROTATE_RIGHT:
    case Node::node_t::NODE_SCOPE:
    case Node::node_t::NODE_SEMICOLON:
    case Node::node_t::NODE_SHIFT_LEFT:
    case Node::node_t::NODE_SHIFT_RIGHT:
    case Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_STRICTLY_EQUAL:
    case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
    case Node::node_t::NODE_SUBTRACT:
        if(attr_count > 0)
        {
            last_attr = attr_list->get_child(attr_count - 1);
            unget_token(f_node);
            f_node = last_attr;
            --attr_count;
            attr_list->delete_child(attr_count);
            if(type != Node::node_t::NODE_COLON)
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
    if(type == Node::node_t::NODE_USE)
    {
        get_token();
        // Note that we do not change the variable 'type' here!
    }

    // check for directives which cannot have attributes
    if(attr_count > 0)
    {
        switch(type)
        {
        case Node::node_t::NODE_IDENTIFIER:
            {
                // "final identifier [= expression]" is legal but needs
                // to be transformed here to work properly
                Node::pointer_t child(attr_list->get_child(0));
                if(attr_count == 1 && child->get_type() == Node::node_t::NODE_FINAL)
                {
                    attr_list.reset();
                    type = Node::node_t::NODE_FINAL;
                }
                else
                {
                    attr_count = 0;
                }
            }
            break;

        case Node::node_t::NODE_USE:
            // pragma cannot be annotated
            if(f_node->get_type() != Node::node_t::NODE_NAMESPACE)
            {
                attr_count = 0;
            }
            break;

        case Node::node_t::NODE_ADD:
        case Node::node_t::NODE_ARRAY_LITERAL:
        case Node::node_t::NODE_BITWISE_NOT:
        case Node::node_t::NODE_BREAK:
        case Node::node_t::NODE_CONTINUE:
        case Node::node_t::NODE_CASE:
        case Node::node_t::NODE_CATCH:
        case Node::node_t::NODE_COLON:
        case Node::node_t::NODE_DECREMENT:
        case Node::node_t::NODE_DEFAULT:
        case Node::node_t::NODE_DELETE:
        case Node::node_t::NODE_DO:
        case Node::node_t::NODE_FALSE:
        case Node::node_t::NODE_FLOAT64:
        case Node::node_t::NODE_FOR:
        case Node::node_t::NODE_FINALLY:
        case Node::node_t::NODE_GOTO:
        case Node::node_t::NODE_IF:
        case Node::node_t::NODE_INCREMENT:
        case Node::node_t::NODE_INT64:
        case Node::node_t::NODE_LOGICAL_NOT:
        case Node::node_t::NODE_NEW:
        case Node::node_t::NODE_NULL:
        case Node::node_t::NODE_OBJECT_LITERAL:
        case Node::node_t::NODE_OPEN_PARENTHESIS:
        case Node::node_t::NODE_OPEN_SQUARE_BRACKET:
        case Node::node_t::NODE_REGULAR_EXPRESSION:
        case Node::node_t::NODE_RETURN:
        case Node::node_t::NODE_SEMICOLON: // annotated empty statements are not allowed
        case Node::node_t::NODE_SMART_MATCH: // TBD?
        case Node::node_t::NODE_STRING:
        case Node::node_t::NODE_SUBTRACT:
        case Node::node_t::NODE_SUPER:    // will accept commas too even in expressions
        case Node::node_t::NODE_SWITCH:
        case Node::node_t::NODE_THIS:
        case Node::node_t::NODE_THROW:
        case Node::node_t::NODE_TRUE:
        case Node::node_t::NODE_TRY:
        case Node::node_t::NODE_TYPEOF:
        case Node::node_t::NODE_UNDEFINED:
        case Node::node_t::NODE_VIDENTIFIER:
        case Node::node_t::NODE_VOID:
        case Node::node_t::NODE_WITH:
        case Node::node_t::NODE_WHILE:
        {
            attr_count = 0;
        }
            break;

        // everything else can be annotated
        default:
            break;

        }
        if(attr_count == 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_input()->get_position());
            msg << "no attributes were expected here (statements, expressions and pragmas cannot be annotated).";
            attr_list.reset();
        }
        if(!attr_list)
        {
            attr_count = 0;
        }
    }

    // The directive node, if created by a sub-function, will
    // be added to the list of directives.
    Node::pointer_t directive_node;
    switch(type)
    {
    // *** PRAGMA ***
    case Node::node_t::NODE_USE:
        // we alread did a GetToken() to skip the NODE_USE
        if(f_node->get_type() == Node::node_t::NODE_NAMESPACE)
        {
            // use namespace ... ';'
            get_token();
            use_namespace(directive_node);
            break;
        }
        if(f_node->get_type() == Node::node_t::NODE_IDENTIFIER)
        {
            Node::pointer_t name(f_node);
            get_token();
            if(f_node->get_type() == Node::node_t::NODE_AS)
            {
                // creating a numeric type
                numeric_type(directive_node, name);
                break;
            }
            // not a numeric type, must be a pragma
            unget_token(f_node);
            f_node = name;
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
    case Node::node_t::NODE_PACKAGE:
        get_token();
        package(directive_node);
        break;

    case Node::node_t::NODE_IMPORT:
        get_token();
        import(directive_node);
        break;

    // *** CLASS DEFINITION ***
    case Node::node_t::NODE_CLASS:
    case Node::node_t::NODE_INTERFACE:
        get_token();
        class_declaration(directive_node, type);
        break;

    case Node::node_t::NODE_ENUM:
        get_token();
        enum_declaration(directive_node);
        break;

    case Node::node_t::NODE_INVARIANT:
        get_token();
        contract_declaration(directive_node, type);
        break;

    // *** FUNCTION DEFINITION ***
    case Node::node_t::NODE_FUNCTION:
        get_token();
        function(directive_node, false);
        break;

    // *** VARIABLE DEFINITION ***
    case Node::node_t::NODE_CONST:
        get_token();
        if(f_node->get_type() == Node::node_t::NODE_VAR)
        {
            get_token();
        }
        variable(directive_node, Node::node_t::NODE_CONST);
        break;

    case Node::node_t::NODE_FINAL:
        // this special case happens when the user forgets to put
        // a variable name (final = 5) or the var keyword is not
        // used; the variable() function generates the correct
        // error and skips the entry as required if necessary
        if(f_node->get_type() == Node::node_t::NODE_FINAL)
        {
            // skip the FINAL keyword
            // otherwise we are already on the IDENTIFIER keyword
            get_token();
        }
        variable(directive_node, Node::node_t::NODE_FINAL);
        break;

    case Node::node_t::NODE_VAR:
        {
            get_token();

            // in this case the VAR keyword may be preceeded by
            // the FINAL keywoard which this far is viewed as an
            // attribute; so make it a keyword again
            bool found(false);
            for(size_t idx(0); idx < attr_count; ++idx)
            {
                Node::pointer_t child(attr_list->get_child(idx));
                if(child->get_type() == Node::node_t::NODE_FINAL)
                {
                    // got it, remove it from the list
                    found = true;
                    attr_list->delete_child(idx);
                    --attr_count;
                    break;
                }
            }
            if(found)
            {
                variable(directive_node, Node::node_t::NODE_FINAL);
            }
            else
            {
                variable(directive_node, Node::node_t::NODE_VAR);
            }
        }
        break;

    // *** STATEMENT ***
    case Node::node_t::NODE_OPEN_CURVLY_BRACKET:
        get_token();
        block(directive_node);
        break;

    case Node::node_t::NODE_SEMICOLON:
        // empty statements are just skipped
        //
        // NOTE: we reach here only when we find attributes
        //       which are not identifiers and this means
        //       we will have gotten an error.
        get_token();
        break;

    case Node::node_t::NODE_BREAK:
    case Node::node_t::NODE_CONTINUE:
        get_token();
        break_continue(directive_node, type);
        break;

    case Node::node_t::NODE_CASE:
        get_token();
        case_directive(directive_node);
        break;

    case Node::node_t::NODE_CATCH:
        get_token();
        catch_directive(directive_node);
        break;

    case Node::node_t::NODE_DEBUGGER:    // just not handled yet...
        get_token();
        debugger(directive_node);
        break;

    case Node::node_t::NODE_DEFAULT:
        get_token();
        default_directive(directive_node);
        break;

    case Node::node_t::NODE_DO:
        get_token();
        do_directive(directive_node);
        break;

    case Node::node_t::NODE_FOR:
        get_token();
        for_directive(directive_node);
        break;

    case Node::node_t::NODE_FINALLY:
    case Node::node_t::NODE_TRY:
        get_token();
        try_finally(directive_node, type);
        break;

    case Node::node_t::NODE_GOTO:
        get_token();
        goto_directive(directive_node);
        break;

    case Node::node_t::NODE_IF:
        get_token();
        if_directive(directive_node);
        break;

    case Node::node_t::NODE_NAMESPACE:
        get_token();
        namespace_block(directive_node, attr_list);
        break;

    case Node::node_t::NODE_RETURN:
        get_token();
        return_directive(directive_node);
        break;

    case Node::node_t::NODE_SWITCH:
        get_token();
        switch_directive(directive_node);
        break;

    case Node::node_t::NODE_SYNCHRONIZED:
        get_token();
        synchronized(directive_node);
        break;

    case Node::node_t::NODE_THROW:
        get_token();
        throw_directive(directive_node);
        break;

    case Node::node_t::NODE_WITH:
    case Node::node_t::NODE_WHILE:
        get_token();
        with_while(directive_node, type);
        break;

    case Node::node_t::NODE_YIELD:
        get_token();
        yield(directive_node);
        break;

    case Node::node_t::NODE_COLON:
        // the label was the last identifier in the
        // attributes which is now in f_node
        f_node->to_label();
        directive_node = f_node;
        // we skip the identifier here
        get_token();
        // and then the ':'
        get_token();
        break;

    // *** EXPRESSION ***
    case Node::node_t::NODE_ARRAY_LITERAL:
    case Node::node_t::NODE_DECREMENT:
    case Node::node_t::NODE_DELETE:
    case Node::node_t::NODE_FALSE:
    case Node::node_t::NODE_FLOAT64:
    case Node::node_t::NODE_IDENTIFIER:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_INT64:
    case Node::node_t::NODE_NEW:
    case Node::node_t::NODE_NULL:
    case Node::node_t::NODE_OBJECT_LITERAL:
    case Node::node_t::NODE_PRIVATE:
    case Node::node_t::NODE_PROTECTED:
    case Node::node_t::NODE_PUBLIC:
    case Node::node_t::NODE_UNDEFINED:
    case Node::node_t::NODE_REGULAR_EXPRESSION:
    case Node::node_t::NODE_STRING:
    case Node::node_t::NODE_SUPER:    // will accept commas too even in expressions
    case Node::node_t::NODE_THIS:
    case Node::node_t::NODE_TRUE:
    case Node::node_t::NODE_TYPEOF:
    case Node::node_t::NODE_VIDENTIFIER:
    case Node::node_t::NODE_VOID:
    case Node::node_t::NODE_LOGICAL_NOT:
    case Node::node_t::NODE_ADD:
    case Node::node_t::NODE_SUBTRACT:
    case Node::node_t::NODE_OPEN_PARENTHESIS:
    case Node::node_t::NODE_OPEN_SQUARE_BRACKET:
    case Node::node_t::NODE_BITWISE_NOT:
    case Node::node_t::NODE_SMART_MATCH: // if here, need to be broken up to ~ and ~
    case Node::node_t::NODE_NOT_MATCH: // if here, need to be broken up to ! and ~
        expression(directive_node);
        break;

    // *** TERMINATOR ***
    case Node::node_t::NODE_EOF:
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_EOF, f_lexer->get_input()->get_position());
        msg << "unexpected end of file reached.";
    }
        return;

    case Node::node_t::NODE_CLOSE_CURVLY_BRACKET:
        // this error does not seem required at this point
        // we get the error from the program already
    //{
    //    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_input()->get_position());
    //    msg << "unexpected '}'.";
    //}
        return;

    // *** INVALID ***
    // The following are for sure invalid tokens in this
    // context. If it looks like some of these could be
    // valid when this function returns, just comment
    // out the corresponding case.
    case Node::node_t::NODE_AS:
    case Node::node_t::NODE_ASSIGNMENT:
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
    case Node::node_t::NODE_BITWISE_AND:
    case Node::node_t::NODE_BITWISE_XOR:
    case Node::node_t::NODE_BITWISE_OR:
    case Node::node_t::NODE_CLOSE_PARENTHESIS:
    case Node::node_t::NODE_CLOSE_SQUARE_BRACKET:
    case Node::node_t::NODE_COMMA:
    case Node::node_t::NODE_COMPARE:
    case Node::node_t::NODE_CONDITIONAL:
    case Node::node_t::NODE_DIVIDE:
    case Node::node_t::NODE_EQUAL:
    case Node::node_t::NODE_GREATER:
    case Node::node_t::NODE_GREATER_EQUAL:
    case Node::node_t::NODE_IMPLEMENTS:
    case Node::node_t::NODE_INSTANCEOF:
    case Node::node_t::NODE_IN:
    case Node::node_t::NODE_IS:
    case Node::node_t::NODE_LESS:
    case Node::node_t::NODE_LESS_EQUAL:
    case Node::node_t::NODE_LOGICAL_AND:
    case Node::node_t::NODE_LOGICAL_OR:
    case Node::node_t::NODE_LOGICAL_XOR:
    case Node::node_t::NODE_MATCH:
    case Node::node_t::NODE_MAXIMUM:
    case Node::node_t::NODE_MEMBER:
    case Node::node_t::NODE_MINIMUM:
    case Node::node_t::NODE_MODULO:
    case Node::node_t::NODE_MULTIPLY:
    case Node::node_t::NODE_NOT_EQUAL:
    case Node::node_t::NODE_POWER:
    case Node::node_t::NODE_RANGE:
    case Node::node_t::NODE_REST:
    case Node::node_t::NODE_ROTATE_LEFT:
    case Node::node_t::NODE_ROTATE_RIGHT:
    case Node::node_t::NODE_SCOPE:
    case Node::node_t::NODE_SHIFT_LEFT:
    case Node::node_t::NODE_SHIFT_RIGHT:
    case Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_STRICTLY_EQUAL:
    case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
    case Node::node_t::NODE_VARIABLE:
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
        msg << "unexpected operator '" << instruction_node->get_type_name() << "'.";
        get_token();
    }
        break;

    case Node::node_t::NODE_ELSE:
    case Node::node_t::NODE_ENSURE:
    case Node::node_t::NODE_EXTENDS:
    case Node::node_t::NODE_REQUIRE:
    case Node::node_t::NODE_THEN:
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_KEYWORD, f_lexer->get_input()->get_position());
        msg << "unexpected keyword '" << instruction_node->get_type_name() << "'.";
        get_token();
    }
        break;

    case Node::node_t::NODE_ABSTRACT:
    //case Node::node_t::NODE_FALSE:
    case Node::node_t::NODE_INLINE:
    case Node::node_t::NODE_NATIVE:
    //case Node::node_t::NODE_PRIVATE:
    //case Node::node_t::NODE_PROTECTED:
    //case Node::node_t::NODE_PUBLIC:
    case Node::node_t::NODE_STATIC:
    case Node::node_t::NODE_TRANSIENT:
    //case Node::node_t::NODE_TRUE:
    case Node::node_t::NODE_VOLATILE:
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_input()->get_position());
        msg << "a statement with only attributes (" << Node::type_to_string(type) << ") is not allowed.";
        attr_list.reset();
        attr_count = 0;

        // skip that attribute which we cannot do anything with
        get_token();
    }
        break;

    // *** NOT POSSIBLE ***
    // These should never happen since they should be caught
    // before this switch is reached or it can't be a node
    // read by the lexer.
    case Node::node_t::NODE_ARRAY:
    case Node::node_t::NODE_ATTRIBUTES:
    case Node::node_t::NODE_AUTO:
    case Node::node_t::NODE_BOOLEAN:
    case Node::node_t::NODE_BYTE:
    case Node::node_t::NODE_CALL:
    case Node::node_t::NODE_CHAR:
    case Node::node_t::NODE_DIRECTIVE_LIST:
    case Node::node_t::NODE_DOUBLE:
    case Node::node_t::NODE_EMPTY:
    case Node::node_t::NODE_EXCLUDE:
    case Node::node_t::NODE_EXPORT:
    case Node::node_t::NODE_FLOAT:
    case Node::node_t::NODE_INCLUDE:
    case Node::node_t::NODE_LABEL:
    case Node::node_t::NODE_LIST:
    case Node::node_t::NODE_LONG:
    case Node::node_t::NODE_NAME:
    case Node::node_t::NODE_PARAM:
    case Node::node_t::NODE_PARAMETERS:
    case Node::node_t::NODE_PARAM_MATCH:
    case Node::node_t::NODE_POST_DECREMENT:
    case Node::node_t::NODE_POST_INCREMENT:
    case Node::node_t::NODE_PROGRAM:
    case Node::node_t::NODE_ROOT:
    case Node::node_t::NODE_SET:
    case Node::node_t::NODE_SHORT:
    case Node::node_t::NODE_THROWS:
    case Node::node_t::NODE_TYPE:
    case Node::node_t::NODE_UNKNOWN:    // ?!
    case Node::node_t::NODE_VAR_ATTRIBUTES:
    case Node::node_t::NODE_other:      // no node should be of this type
    case Node::node_t::NODE_max:        // no node should be of this type
        {
            Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, f_lexer->get_input()->get_position()); // LCOV_EXCL_LINE
            msg << "INTERNAL ERROR: invalid node (" << Node::type_to_string(type) << ") in directive_list."; // LCOV_EXCL_LINE
        }
        throw exception_internal_error("unexpected node type found while parsing directives"); // LCOV_EXCL_LINE

    }
    if(directive_node)
    {
        // if there are attributes link them to the directive
        if(attr_list && attr_list->get_children_size() > 0)
        {
            directive_node->set_attribute_node(attr_list);
        }
        node->append_child(directive_node);
    }

    // Now make sure we have a semicolon for
    // those statements which have to have it.
    switch(type)
    {
    case Node::node_t::NODE_ARRAY_LITERAL:
    case Node::node_t::NODE_BREAK:
    case Node::node_t::NODE_CONST:
    case Node::node_t::NODE_CONTINUE:
    case Node::node_t::NODE_DECREMENT:
    case Node::node_t::NODE_DELETE:
    case Node::node_t::NODE_DO:
    case Node::node_t::NODE_FLOAT64:
    case Node::node_t::NODE_GOTO:
    case Node::node_t::NODE_IDENTIFIER:
    case Node::node_t::NODE_IMPORT:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_INT64:
    case Node::node_t::NODE_NEW:
    case Node::node_t::NODE_NULL:
    case Node::node_t::NODE_OBJECT_LITERAL:
    case Node::node_t::NODE_RETURN:
    case Node::node_t::NODE_REGULAR_EXPRESSION:
    case Node::node_t::NODE_STRING:
    case Node::node_t::NODE_SUPER:
    case Node::node_t::NODE_THIS:
    case Node::node_t::NODE_THROW:
    case Node::node_t::NODE_TYPEOF:
    case Node::node_t::NODE_UNDEFINED:
    case Node::node_t::NODE_USE:
    case Node::node_t::NODE_VAR:
    case Node::node_t::NODE_VIDENTIFIER:
    case Node::node_t::NODE_VOID:
    case Node::node_t::NODE_LOGICAL_NOT:
    case Node::node_t::NODE_ADD:
    case Node::node_t::NODE_SUBTRACT:
    case Node::node_t::NODE_OPEN_PARENTHESIS:
    case Node::node_t::NODE_OPEN_SQUARE_BRACKET:
    case Node::node_t::NODE_BITWISE_NOT:
        // accept missing ';' when we find a '}' next
        if(f_node->get_type() != Node::node_t::NODE_SEMICOLON
        && f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_SEMICOLON_EXPECTED, f_lexer->get_input()->get_position());
            msg << "';' was expected after '" << instruction_node->get_type_name() << "' (current token: '" << f_node->get_type_name() << "').";
        }
        // skip all that whatever up to the next end of this
        while(f_node->get_type() != Node::node_t::NODE_SEMICOLON
           && f_node->get_type() != Node::node_t::NODE_OPEN_CURVLY_BRACKET
           && f_node->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET
           && f_node->get_type() != Node::node_t::NODE_ELSE
           && f_node->get_type() != Node::node_t::NODE_EOF)
        {
            get_token();
        }
        // we need to skip one semi-colon here
        // in case we're not in a directive_list()
        if(f_node->get_type() == Node::node_t::NODE_SEMICOLON)
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
