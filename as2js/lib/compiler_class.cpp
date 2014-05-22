/* compiler_class.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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



void Compiler::declare_class(Node::pointer_t class_node)
{
    size_t const max_children(class_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        //NodeLock ln(class_node);
        Node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::NODE_DIRECTIVE_LIST:
            declare_class(child); // recursive!
            break;

        case Node::NODE_CLASS:
        case Node::NODE_INTERFACE:
            class_directive(child);
            break;

        case Node::NODE_ENUM:
            enum_directive(child);
            break;

        case Node::NODE_FUNCTION:
            function(child);
            break;

        case Node::NODE_VAR:
            var(child);
            break;

        default:
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_NODE, child->get_position());
                msg << "the '" << child->get_type_name() << "' token cannot be a class member.";
            }
            break;

        }
    }
}




void Compiler::extend_class(Node::pointer_t class_node, Node::pointer_t extend_name)
{
    expression(extend_name);

    Node::pointer_t super(extend_name->get_link(Node::LINK_INSTANCE));
    if(super)
    {
        if(get_attribute(super, Node::NODE_ATTR_FINAL))
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_FINAL, class_node->get_position());
            msg << "class '" << super->get_string() << "' is marked final and it cannot be extended by '" << class_node->get_string() << "'.";
        }
    }
}


void Compiler::class_directive(Node::pointer_t& class_node)
{
    size_t const max(class_node->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        //NodeLock ln(class_node);
        Node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::NODE_DIRECTIVE_LIST:
            declare_class(child);
            break;

        case Node::NODE_EXTENDS:
        case Node::NODE_IMPLEMENTS:
            extend_class(class_node, child->get_child(0));
            break;

        default:
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INTERNAL_ERROR, class_node->get_position());
                msg << "invalid token '" << child->get_type_name() << "' in a class definition.";
            }
            break;

        }
    }
}


/** \brief Enum directive.
 *
 * Enumerations are like classes defining a list of constant values.
 *
 * \param[in] enum_node  The enumeration node to work on.
 */
void Compiler::enum_directive(Node::pointer_t& enum_node)
{
    NodeLock ln(enum_node);
    size_t const max_children(enum_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t entry(enum_node->get_child(idx));
        if(entry->get_children_size() != 1)
        {
            // not valid, skip
            continue;
        }
        Node::pointer_t set = entry->get_child(0);
        if(set->get_children_size() != 1)
        {
            // not valid, skip
            continue;
        }
        // compile the expression
        expression(set->get_child(0));
    }
}




}
// namespace as2js

// vim: ts=4 sw=4 et
