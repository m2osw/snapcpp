/* parser.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include "as2js/parser.h"

#include "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER  *********************************************************/
/**********************************************************************/
/**********************************************************************/

Parser::Parser(Input::pointer_t input, Options::pointer_t options)
    : f_lexer(new Lexer(input, options))
    , f_options(options)
    //, f_root(nullptr) -- auto-init [we keep it unknown at the start]
    //, f_node(nullptr) -- auto-init
    //, f_unget() -- auto-init
{
}


Node::pointer_t Parser::parse()
{
    // This parses everything and creates ONE tree
    // with the result. The tree obviously needs to
    // fit in RAM...

    // We lose the previous tree if any and create a new
    // root node. This is our program node.
    get_token();
    program(f_root);

    return f_root;
}


void Parser::get_token()
{
    bool const reget(!f_unget.empty());

    if(reget)
    {
        f_node = f_unget.back();
        f_unget.pop_back();
    }
    else
    {
        f_node = f_lexer->get_next_token();
    }
}


void Parser::unget_token(Node::pointer_t& node)
{
    f_unget.push_back(node);
}



/** \brief Check whether a given option is set.
 *
 * Because the parser checks options in many places, it makes use of this
 * helper function just in case we wanted to handle various special cases.
 * (I had two before, but both have been removed at this point.)
 *
 * This function checks whether the specified option is set. If so,
 * then it returns true, otherwise it returns false.
 *
 * \param[in] option  The option to check.
 *
 * \return true if the option was set, false otherwise.
 */
bool Parser::has_option_set(Options::option_t option) const
{
    return f_options->get_option(option) != 0;
}



}
// namespace as2js

// vim: ts=4 sw=4 et
