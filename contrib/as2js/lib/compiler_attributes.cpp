/* compiler_attributes.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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



void Compiler::variable_to_attrs(Node::pointer_t node, Node::pointer_t var_node)
{
    if(var_node->get_type() != Node::node_t::NODE_SET)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_VARIABLE, var_node->get_position());
        msg << "an attribute variable has to be given a value.";
        return;
    }

    Node::pointer_t a(var_node->get_child(0));
    switch(a->get_type())
    {
    case Node::node_t::NODE_FALSE:
    case Node::node_t::NODE_IDENTIFIER:
    case Node::node_t::NODE_INLINE:
    case Node::node_t::NODE_PRIVATE:
    case Node::node_t::NODE_PROTECTED:
    case Node::node_t::NODE_PUBLIC:
    case Node::node_t::NODE_TRUE:
        node_to_attrs(node, a);
        return;

    default:
        // expect a full boolean expression in this case
        break;

    }

    // compute the expression
    expression(a);
    Optimizer::optimize(a);

    switch(a->get_type())
    {
    case Node::node_t::NODE_TRUE:
    case Node::node_t::NODE_FALSE:
        node_to_attrs(node, a);
        return;

    default:
        break;

    }

    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, var_node->get_position());
    msg << "an attribute which is an expression needs to result in a boolean value (true or false).";
}


void Compiler::identifier_to_attrs(Node::pointer_t node, Node::pointer_t a)
{
    // an identifier cannot be an empty string
    String const identifier(a->get_string());
    switch(identifier[0])
    {
    case 'a':
        if(identifier == "array")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_ARRAY, true);
            return;
        }
        if(identifier == "autobreak")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_AUTOBREAK, true);
            return;
        }
        break;

    case 'c':
        if(identifier == "constructor")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_CONSTRUCTOR, true);
            return;
        }
        break;

    case 'd':
        if(identifier == "deprecated")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_DEPRECATED, true);
            return;
        }
        if(identifier == "dynamic")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_DYNAMIC, true);
            return;
        }
        break;

    case 'e':
        if(identifier == "enumerable")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_ENUMERABLE, true);
            return;
        }
        break;

    case 'f':
        if(identifier == "foreach")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_FOREACH, true);
            return;
        }
        break;

    case 'i':
        if(identifier == "internal")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_INTERNAL, true);
            return;
        }
        break;

    case 'n':
        if(identifier == "nobreak")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_NOBREAK, true);
            return;
        }
        break;

    case 'u':
        if(identifier == "unsafe")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_UNSAFE, true);
            return;
        }
        if(identifier == "unused")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_UNUSED, true);
            return;
        }
        break;

    case 'v':
        if(identifier == "virtual")
        {
            node->set_attribute(Node::attribute_t::NODE_ATTR_VIRTUAL, true);
            return;
        }
        break;

    }

    // it could be a user defined variable list of attributes
    Node::pointer_t resolution;
    if(!resolve_name(node, a, resolution, Node::pointer_t(), SEARCH_FLAG_NO_PARSING))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, a->get_position());
        msg << "cannot find a variable named '" << a->get_string() << "'.";
        return;
    }
    if(!resolution)
    {
        // TODO: do we expect an error here?
        return;
    }
    if(resolution->get_type() != Node::node_t::NODE_VARIABLE
    && resolution->get_type() != Node::node_t::NODE_VAR_ATTRIBUTES)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DYNAMIC, a->get_position());
        msg << "a dynamic attribute name can only reference a variable and '" << a->get_string() << "' is not one.";
        return;
    }

    // it is a variable, go through the list and call ourselves recursively
    // with each identifiers; but make sure we do not loop forever
    if(resolution->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_ATTRS))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_LOOPING_REFERENCE, a->get_position());
        msg << "the dynamic attribute variable '" << a->get_string() << "' is used circularly (it loops).";
        return;
    }

    resolution->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_ATTRS, true); // to avoid infinite loop
    resolution->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES, true);
    NodeLock ln(resolution);
    size_t const max_children(resolution->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(resolution->get_child(idx));
        variable_to_attrs(node, child);
    }
    resolution->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_ATTRS, false);
}


void Compiler::node_to_attrs(Node::pointer_t node, Node::pointer_t a)
{
    switch(a->get_type())
    {
    case Node::node_t::NODE_ABSTRACT:
        node->set_attribute(Node::attribute_t::NODE_ATTR_ABSTRACT, true);
        break;

    case Node::node_t::NODE_FALSE:
        node->set_attribute(Node::attribute_t::NODE_ATTR_FALSE, true);
        break;

    case Node::node_t::NODE_FINAL:
        node->set_attribute(Node::attribute_t::NODE_ATTR_FINAL, true);
        break;

    case Node::node_t::NODE_IDENTIFIER:
        identifier_to_attrs(node, a);
        break;

    case Node::node_t::NODE_INLINE:
        node->set_attribute(Node::attribute_t::NODE_ATTR_INLINE, true);
        break;

    case Node::node_t::NODE_NATIVE: // Note: I called this one INTRINSIC before
        node->set_attribute(Node::attribute_t::NODE_ATTR_NATIVE, true);
        break;

    case Node::node_t::NODE_PRIVATE:
        node->set_attribute(Node::attribute_t::NODE_ATTR_PRIVATE, true);
        break;

    case Node::node_t::NODE_PROTECTED:
        node->set_attribute(Node::attribute_t::NODE_ATTR_PROTECTED, true);
        break;

    case Node::node_t::NODE_PUBLIC:
        node->set_attribute(Node::attribute_t::NODE_ATTR_PUBLIC, true);
        break;

    case Node::node_t::NODE_STATIC:
        node->set_attribute(Node::attribute_t::NODE_ATTR_STATIC, true);
        break;

    case Node::node_t::NODE_TRANSIENT:
        node->set_attribute(Node::attribute_t::NODE_ATTR_TRANSIENT, true);
        break;

    case Node::node_t::NODE_TRUE:
        node->set_attribute(Node::attribute_t::NODE_ATTR_TRUE, true);
        break;

    case Node::node_t::NODE_VOLATILE:
        node->set_attribute(Node::attribute_t::NODE_ATTR_VOLATILE, true);
        break;

    default:
        // TODO: this is a scope (user defined name)
        // ERROR: unknown attribute type
        // Note that will happen whenever someone references a
        // variable which is an expression which does not resolve
        // to a valid attribute and thus we need a user error here
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, a->get_position());
        msg << "unsupported attribute data type, dynamic expressions for attributes need to be resolved as constants.";
        break;

    }
}


void Compiler::prepare_attributes(Node::pointer_t node)
{
    // done here?
    if(node->get_attribute(Node::attribute_t::NODE_ATTR_DEFINED))
    {
        return;
    }

    // mark ourselves as done even if errors occur
    node->set_attribute(Node::attribute_t::NODE_ATTR_DEFINED, true);

    if(node->get_type() == Node::node_t::NODE_PROGRAM)
    {
        // programs do not get any specific attributes
        // (optimization)
        return;
    }

    Node::pointer_t attr(node->get_attribute_node());
    if(attr)
    {
        NodeLock ln(attr);
        size_t const max_attr(attr->get_children_size());
        for(size_t idx(0); idx < max_attr; ++idx)
        {
            node_to_attrs(node, attr->get_child(idx));
        }
    }

    // check whether intrinsic is already set
    // (in which case it is probably an error)
    bool const has_direct_native(node->get_attribute(Node::attribute_t::NODE_ATTR_NATIVE));

    // Note: we already returned if it is equal
    //       to program; here it is just documentation
    if(node->get_type() != Node::node_t::NODE_PACKAGE
    && node->get_type() != Node::node_t::NODE_PROGRAM)
    {
        Node::pointer_t parent(node->get_parent());
        if(parent
        && parent->get_type() != Node::node_t::NODE_PACKAGE
        && parent->get_type() != Node::node_t::NODE_PROGRAM)
        {
            // recurse against all parents as required
            prepare_attributes(parent);

            // child can redefine (ignore parent if any defined)
            // [TODO: should this be an error if conflicting?]
            if(!node->get_attribute(Node::attribute_t::NODE_ATTR_PUBLIC)
            && !node->get_attribute(Node::attribute_t::NODE_ATTR_PRIVATE)
            && !node->get_attribute(Node::attribute_t::NODE_ATTR_PROTECTED))
            {
                node->set_attribute(Node::attribute_t::NODE_ATTR_PUBLIC,    parent->get_attribute(Node::attribute_t::NODE_ATTR_PUBLIC));
                node->set_attribute(Node::attribute_t::NODE_ATTR_PRIVATE,   parent->get_attribute(Node::attribute_t::NODE_ATTR_PRIVATE));
                node->set_attribute(Node::attribute_t::NODE_ATTR_PROTECTED, parent->get_attribute(Node::attribute_t::NODE_ATTR_PROTECTED));
            }

            // child can redefine (ignore parent if defined)
            if(!node->get_attribute(Node::attribute_t::NODE_ATTR_STATIC)
            && !node->get_attribute(Node::attribute_t::NODE_ATTR_ABSTRACT)
            && !node->get_attribute(Node::attribute_t::NODE_ATTR_VIRTUAL))
            {
                node->set_attribute(Node::attribute_t::NODE_ATTR_STATIC,   parent->get_attribute(Node::attribute_t::NODE_ATTR_STATIC));
                node->set_attribute(Node::attribute_t::NODE_ATTR_ABSTRACT, parent->get_attribute(Node::attribute_t::NODE_ATTR_ABSTRACT));
                node->set_attribute(Node::attribute_t::NODE_ATTR_VIRTUAL,  parent->get_attribute(Node::attribute_t::NODE_ATTR_VIRTUAL));
            }

            // inherit
            node->set_attribute(Node::attribute_t::NODE_ATTR_NATIVE,     parent->get_attribute(Node::attribute_t::NODE_ATTR_NATIVE));
            node->set_attribute(Node::attribute_t::NODE_ATTR_ENUMERABLE, parent->get_attribute(Node::attribute_t::NODE_ATTR_ENUMERABLE));

            // false has priority
            if(parent->get_attribute(Node::attribute_t::NODE_ATTR_FALSE))
            {
                node->set_attribute(Node::attribute_t::NODE_ATTR_TRUE, false);
                node->set_attribute(Node::attribute_t::NODE_ATTR_FALSE, true);
            }

            if(parent->get_type() != Node::node_t::NODE_CLASS)
            {
                node->set_attribute(Node::attribute_t::NODE_ATTR_DYNAMIC, parent->get_attribute(Node::attribute_t::NODE_ATTR_DYNAMIC));
                node->set_attribute(Node::attribute_t::NODE_ATTR_FINAL,   parent->get_attribute(Node::attribute_t::NODE_ATTR_FINAL));
            }
        }
    }

    // a function which has a body cannot be intrinsic
    if(node->get_attribute(Node::attribute_t::NODE_ATTR_NATIVE)
    && node->get_type() == Node::node_t::NODE_FUNCTION)
    {
        NodeLock ln(node);
        size_t const max(node->get_children_size());
        for(size_t idx(0); idx < max; ++idx)
        {
            Node::pointer_t list(node->get_child(idx));
            if(list->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
            {
                // it is an error if the user defined
                // it directly on the function; it is
                // fine if it comes from the parent
                if(has_direct_native)
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NATIVE, node->get_position());
                    msg << "'native' is not permitted on a function with a body.";
                }
                node->set_attribute(Node::attribute_t::NODE_ATTR_NATIVE, false);
                break;
            }
        }
    }
}


bool Compiler::get_attribute(Node::pointer_t node, Node::attribute_t const a)
{
    prepare_attributes(node);
    return node->get_attribute(a);
}








}
// namespace as2js

// vim: ts=4 sw=4 et
