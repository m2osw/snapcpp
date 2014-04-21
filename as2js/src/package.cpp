/* package.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER PACKAGE  *************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Package(NodePtr& node)
{
    String        name;

    node.CreateNode(NODE_PACKAGE);
    node.SetInputInfo(f_lexer.GetInput());

    if(f_data.f_type == NODE_IDENTIFIER) {
        name = f_data.f_str;
        GetToken();
        while(f_data.f_type == NODE_MEMBER) {
            GetToken();
            if(f_data.f_type != NODE_IDENTIFIER) {
                // unexpected token/missing name
                f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "invalid package name (expected an identifier after the last '.')");
                break;
            }
            name.AppendChar('.');
            name += f_data.f_str;
            GetToken();
        }
    }
    else if(f_data.f_type == NODE_STRING) {
        name = f_data.f_str;
        // TODO: Validate Package Name (in case of a STRING)
        // I think we need to check out the name here to make sure
        // that's a valid package name (not too sure thought whether
        // we can't just have any name)
        GetToken();
    }

    // set the name and flags of this package
    Data& data = node.GetData();
    data.f_str = name;

    if(f_data.f_type == '{') {
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "'{' expected after the package name");
        // TODO: should we return and not try to read the package?
    }

    NodePtr directive_list;
    DirectiveList(directive_list);
    node.AddChild(directive_list);

    // when we return we should have a '}'
    if(f_data.f_type == '}') {
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "'}' expected after the package declaration");
    }
}




/**********************************************************************/
/**********************************************************************/
/***  PARSER IMPORT  **************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Import(NodePtr& node)
{
    node.CreateNode(NODE_IMPORT);
    node.SetInputInfo(f_lexer.GetInput());
    Data& data = node.GetData();

    if(f_data.f_type == NODE_IMPLEMENTS) {
        data.f_int.Set(NODE_IMPORT_FLAG_IMPLEMENTS);
        GetToken();
    }

    if(f_data.f_type == NODE_IDENTIFIER) {
        String name;
        Data first(f_data);
        GetToken();
        bool is_renaming = f_data.f_type == '=';
        if(is_renaming) {
            NodePtr rename;
            rename.CreateNode();
            node.SetInputInfo(f_lexer.GetInput());
            rename.SetData(first);
            node.AddChild(rename);
            GetToken();
            if(f_data.f_type == NODE_STRING) {
                name = f_data.f_str;
                GetToken();
                if(f_data.f_type == '.') {
                    f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "a package name is either a string or a list of identifiers separated by periods (.); you can't mixed both");
                }
            }
            else if(f_data.f_type == NODE_IDENTIFIER) {
                name = f_data.f_str;
                GetToken();
            }
            else {
                f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "the name of a package was expected");
            }
        }
        else {
            name = first.f_str;
        }

        int everything = 0;
        while(f_data.f_type == '.') {
            if(everything == 1) {
                everything = 2;
                f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "the * notation can only be used once at the end of a name");
            }
            name.AppendChar('.');
            GetToken();
            if(f_data.f_type == '*') {
                if(is_renaming && everything == 0) {
                    f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "the * notation cannot be used when renaming an import");
                    everything = 2;
                }
                // everything in that directory
                name.AppendChar('*');
                if(everything == 0) {
                    everything = 1;
                }
            }
            else if(f_data.f_type != NODE_IDENTIFIER) {
                if(f_data.f_type == NODE_STRING) {
                    f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "a package name is either a string or a list of identifiers separated by periods (.); you can't mixed both");
                }
                else {
                    f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "the name of a package was expected");
                }
                break;
            }
            else {
                name += f_data.f_str;
            }
            GetToken();
        }

        data.f_str = name;
    }
    else if(f_data.f_type == NODE_STRING) {
        // TODO: Validate Package Name (in case of a STRING)
        data.f_str = f_data.f_str;
        GetToken();
    }
    else {
        f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "a composed name or a string was expected after 'import'");
        if(f_data.f_type != ';' && f_data.f_type != ',') {
            GetToken();
        }
    }

    // Any namespace and/or include/exclude info?
    // NOTE: We accept multiple namespace and multiple include
    //     or exclude.
    //     However, include and exclude are mutually exclusive.
    long include_exclude = 0;
    while(f_data.f_type == ',') {
        GetToken();
        if(f_data.f_type == NODE_NAMESPACE) {
            GetToken();
            // read the namespace (an expression)
            NodePtr expr;
            ConditionalExpression(expr, false);
            NodePtr use;
            use.CreateNode(NODE_USE /*namespace*/);
            use.SetInputInfo(f_lexer.GetInput());
            use.AddChild(expr);
            node.AddChild(use);
        }
        else if(f_data.f_type == NODE_IDENTIFIER) {
            if(f_data.f_str == "include") {
                if(include_exclude == 2) {
                    f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "include and exclude are mutually exclusive");
                    include_exclude = 3;
                }
                else if(include_exclude == 0) {
                    include_exclude = 1;
                }
                GetToken();
                // read the list of inclusion (an expression)
                NodePtr expr;
                ConditionalExpression(expr, false);
                NodePtr include;
                include.CreateNode(NODE_INCLUDE);
                include.SetInputInfo(f_lexer.GetInput());
                include.AddChild(expr);
                node.AddChild(include);
            }
            else if(f_data.f_str == "exclude") {
                if(include_exclude == 1) {
                    f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "include and exclude are mutually exclusive");
                    include_exclude = 3;
                }
                else if(include_exclude == 0) {
                    include_exclude = 2;
                }
                GetToken();
                // read the list of exclusion (an expression)
                NodePtr expr;
                ConditionalExpression(expr, false);
                NodePtr exclude;
                exclude.CreateNode(NODE_EXCLUDE);
                exclude.SetInputInfo(f_lexer.GetInput());
                exclude.AddChild(expr);
                node.AddChild(exclude);
            }
            else {
                f_lexer.ErrMsg(AS_ERR_INVALID_PACKAGE_NAME, "namespace, include or exclude was expected after the comma");
            }
        }
    }
}





/**********************************************************************/
/**********************************************************************/
/***  PARSER NAMESPACE  ***********************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::UseNamespace(NodePtr& node)
{
#if 1
    NodePtr expr;
    Expression(expr);
    node.CreateNode(NODE_USE /*namespace*/);
    node.SetInputInfo(f_lexer.GetInput());
    node.AddChild(expr);
#else
    if(f_data.f_type == '(') {
        GetToken();
        NodePtr expr;
        Expression(expr);
        if(f_data.f_type == ')') {
            GetToken();
            node.CreateNode(NODE_USE /*namespace*/);
            node.SetInputInfo(f_lexer.GetInput());
            node.AddChild(expr);
        }
        else {
            f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "')' expected after the 'use namespace' expression");
        }
    }
    else {
        f_lexer.ErrMsg(AS_ERR_PARENTHESIS_EXPECTED, "'(' expected after the 'use namespace' keywords");
    }
#endif
}



void IntParser::Namespace(NodePtr& node)
{
    if(f_data.f_type == NODE_IDENTIFIER)
    {
        // save the name of the namespace
        node.CreateNode();
        node.SetInputInfo(f_lexer.GetInput());
        f_data.f_type = NODE_NAMESPACE;
        node.SetData(f_data);
        GetToken();
    }
    else
    {
        f_lexer.ErrMsg(AS_ERR_INVALID_NAMESPACE, "the 'namespace' declaration expects an identifier");
    }
}



}
// namespace as2js

// vim: ts=4 sw=4 et
