/* package.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER PACKAGE  *************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::package(Node::pointer_t& node)
{
    String        name;

    node = f_lexer->get_new_node(Node::NODE_PACKAGE);

    if(f_node->get_type() == Node::NODE_IDENTIFIER)
    {
        name = f_node->get_string();
        get_token();
        while(f_node->get_type() == Node::NODE_MEMBER)
        {
            get_token();
            if(f_node->get_type() != Node::NODE_IDENTIFIER)
            {
                // unexpected token/missing name
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                msg << "invalid package name (expected an identifier after the last '.')";
                break;
            }
            name += ".";
            name += f_node->get_string();
            get_token();
        }
    }
    else if(f_node->get_type() == Node::NODE_STRING)
    {
        name = f_node->get_string();
        // TODO: Validate Package Name (in case of a STRING)
        // I think we need to check out the name here to make sure
        // that's a valid package name (not too sure though whether
        // we can't just have any name)
        get_token();
    }

    // set the name and flags of this package
    node->set_string(name);

    if(f_node->get_type() == Node::NODE_OPEN_CURVLY_BRACKET)
    {
        get_token();
    }
    else
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
        msg << "'{' expected after the package name";
        // TODO: should we return and not try to read the package?
    }

    Node::pointer_t directives;
    directive_list(directives);
    node->append_child(directives);

    // when we return we should have a '}'
    if(f_node->get_type() == Node::NODE_CLOSE_CURVLY_BRACKET)
    {
        get_token();
    }
    else
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
        msg << "'}' expected after the package declaration";
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER IMPORT  **************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::import(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::NODE_IMPORT);

    if(f_node->get_type() == Node::NODE_IMPLEMENTS)
    {
        node->set_flag(Node::NODE_IMPORT_FLAG_IMPLEMENTS, true);
        get_token();
    }

    if(f_node->get_type() == Node::NODE_IDENTIFIER)
    {
        String name;
        Node::pointer_t first(f_node);
        get_token();
        bool const is_renaming = f_node->get_type() == Node::NODE_ASSIGNMENT;
        if(is_renaming)
        {
            // add first as the package alias
            node->append_child(first);

            get_token();
            if(f_node->get_type() == Node::NODE_STRING)
            {
                name = f_node->get_string();
                get_token();
                if(f_node->get_type() == Node::NODE_MEMBER)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                    msg << "a package name is either a string or a list of identifiers separated by periods (.); you can't mixed both";
                }
            }
            else if(f_node->get_type() == Node::NODE_IDENTIFIER)
            {
                name = f_node->get_string();
                get_token();
            }
            else
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                msg << "the name of a package was expected";
            }
        }
        else
        {
            name = first->get_string();
        }

        int everything = 0;
        while(f_node->get_type() == Node::NODE_MEMBER)
        {
            if(everything == 1)
            {
                everything = 2;
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                msg << "the * notation can only be used once at the end of a name";
            }
            name += ".";
            get_token();
            if(f_node->get_type() == Node::NODE_MULTIPLY)
            {
                if(is_renaming && everything == 0)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                    msg << "the * notation cannot be used when renaming an import";
                    everything = 2;
                }
                // everything in that directory
                name += "*";
                if(everything == 0)
                {
                    everything = 1;
                }
            }
            else if(f_node->get_type() == Node::NODE_IDENTIFIER)
            {
                if(f_node->get_type() == Node::NODE_STRING)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                    msg << "a package name is either a string or a list of identifiers separated by periods (.); you can't mixed both";
                }
                else
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                    msg << "the name of a package was expected";
                }
                break;
            }
            else
            {
                name += f_node->get_string();
            }
            get_token();
        }

        node->set_string(name);
    }
    else if(f_node->get_type() == Node::NODE_STRING)
    {
        // TODO: Validate Package Name (in case of a STRING)
        node->set_string(f_node->get_string());
        get_token();
    }
    else
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
        msg << "a composed name or a string was expected after 'import'";
        if(f_node->get_type() != Node::NODE_SEMICOLON && f_node->get_type() != Node::NODE_COMMA)
        {
            get_token();
        }
    }

    // Any namespace and/or include/exclude info?
    // NOTE: We accept multiple namespace and multiple include
    //     or exclude.
    //     However, include and exclude are mutually exclusive.
    long include_exclude = 0;
    while(f_node->get_type() == Node::NODE_COMMA)
    {
        get_token();
        if(f_node->get_type() == Node::NODE_NAMESPACE)
        {
            get_token();
            // read the namespace (an expression)
            Node::pointer_t expr;
            conditional_expression(expr, false);
            Node::pointer_t use(f_lexer->get_new_node(Node::NODE_USE /*namespace*/));
            use->append_child(expr);
            node->append_child(use);
        }
        else if(f_node->get_type() == Node::NODE_IDENTIFIER)
        {
            if(f_node->get_string() == "include")
            {
                if(include_exclude == 2)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                    msg << "include and exclude are mutually exclusive";
                    include_exclude = 3;
                }
                else if(include_exclude == 0)
                {
                    include_exclude = 1;
                }
                get_token();
                // read the list of inclusion (an expression)
                Node::pointer_t expr;
                conditional_expression(expr, false);
                Node::pointer_t include(f_lexer->get_new_node(Node::NODE_INCLUDE));
                include->append_child(expr);
                node->append_child(include);
            }
            else if(f_node->get_string() == "exclude")
            {
                if(include_exclude == 1)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                    msg << "include and exclude are mutually exclusive";
                    include_exclude = 3;
                }
                else if(include_exclude == 0)
                {
                    include_exclude = 2;
                }
                get_token();
                // read the list of exclusion (an expression)
                Node::pointer_t expr;
                conditional_expression(expr, false);
                Node::pointer_t exclude(f_lexer->get_new_node(Node::NODE_EXCLUDE));
                exclude->append_child(expr);
                node->append_child(exclude);
            }
            else
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_PACKAGE_NAME, f_lexer->get_input()->get_position());
                msg << "namespace, include or exclude was expected after the comma";
            }
        }
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER NAMESPACE  ***********************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::use_namespace(Node::pointer_t& node)
{
    Node::pointer_t expr;
    expression(expr);
    node = f_lexer->get_new_node(Node::NODE_USE /*namespace*/);
    node->append_child(expr);
}



void Parser::namespace_block(Node::pointer_t& node)
{
    if(f_node->get_type() == Node::NODE_IDENTIFIER)
    {
        // save the name of the namespace
        node = f_lexer->get_new_node(Node::NODE_NAMESPACE);
        node->set_string(f_node->get_string());
        get_token();
    }
    else
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_NAMESPACE, f_lexer->get_input()->get_position());
        msg << "the 'namespace' declaration expects an identifier";
    }
}



}
// namespace as2js

// vim: ts=4 sw=4 et
