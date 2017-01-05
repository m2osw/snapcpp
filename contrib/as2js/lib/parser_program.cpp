/* parser_program.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER PROGRAM  *************************************************/
/**********************************************************************/
/**********************************************************************/

void Parser::program(Node::pointer_t& node)
{
    node = f_lexer->get_new_node(Node::node_t::NODE_PROGRAM);
    while(f_node->get_type() != Node::node_t::NODE_EOF)
    {
        Node::pointer_t directives;
        directive_list(directives);
        node->append_child(directives);

        if(f_node->get_type() == Node::node_t::NODE_ELSE)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_KEYWORD, f_lexer->get_input()->get_position());
            msg << "'else' not expected without an 'if' keyword.";
            get_token();
        }
        else if(f_node->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CURVLY_BRACKETS_EXPECTED, f_lexer->get_input()->get_position());
            msg << "'}' not expected without a '{'.";
            get_token();
        }
    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
