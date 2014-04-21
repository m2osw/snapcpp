/* program.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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
/***  PARSER PROGRAM  *************************************************/
/**********************************************************************/
/**********************************************************************/

void IntParser::Program(NodePtr& node)
{
    node.CreateNode(NODE_PROGRAM);
    node.SetInputInfo(f_lexer.GetInput());
    while(f_data.f_type != NODE_EOF) {
        NodePtr directive_list;
        DirectiveList(directive_list);
        node.AddChild(directive_list);

        if(f_data.f_type == NODE_ELSE) {
            f_lexer.ErrMsg(AS_ERR_INVALID_KEYWORD, "'else' not expected without an 'if' keyword");
            GetToken();
        }
        else if(f_data.f_type == '}') {
            f_lexer.ErrMsg(AS_ERR_CURVLY_BRAKETS_EXPECTED, "'}' not expected without a '{'");
            GetToken();
        }
    }
}





}
// namespace as

// vim: ts=4 sw=4 et
