/* compiler_program.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/compiler.h"

#include    "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PROGRAM  ********************************************************/
/**********************************************************************/
/**********************************************************************/



void Compiler::program(Node::pointer_t program_node)
{
    // This is the root. Whenever you search to resolve a reference,
    // do not go past that node! What's in the parent of a program is
    // not part of that program...
    f_program = program_node;

#if 0
std::cerr << "program:\n" << *program_node << "\n";
#endif
    // get rid of any declaration marked false
    size_t const org_max(program_node->get_children_size());
    for(size_t idx(0); idx < org_max; ++idx)
    {
        Node::pointer_t child(program_node->get_child(idx));
        if(get_attribute(child, Node::attribute_t::NODE_ATTR_FALSE))
        {
            child->to_unknown();
        }
    }
    program_node->clean_tree();

    NodeLock ln(program_node);

    // look for all the labels in this program (for goto's)
    for(size_t idx(0); idx < org_max; ++idx)
    {
        Node::pointer_t child(program_node->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
        {
            find_labels(program_node, child);
        }
    }

    // a program is composed of directives (usually just one list)
    // which we want to compile
    for(size_t idx(0); idx < org_max; ++idx)
    {
        Node::pointer_t child(program_node->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
        {
            directive_list(child);
        }
    }

#if 0
if(Message::error_count() > 0)
std::cerr << program_node;
#endif
}







}
// namespace as2js

// vim: ts=4 sw=4 et
