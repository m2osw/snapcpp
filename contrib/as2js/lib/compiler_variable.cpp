/* compiler_variable.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


namespace as2js
{





// we can simplify constant variables with their content whenever it is
// a string, number or other non-dynamic constant
bool Compiler::replace_constant_variable(Node::pointer_t& replace, Node::pointer_t resolution)
{
    if(resolution->get_type() != Node::node_t::NODE_VARIABLE)
    {
        return false;
    }

    if(!resolution->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_CONST))
    {
        return false;
    }

    NodeLock resolution_ln(resolution);
    size_t const max_children(resolution->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t set(resolution->get_child(idx));
        if(set->get_type() != Node::node_t::NODE_SET)
        {
            continue;
        }

        Optimizer::optimize(set);

        if(set->get_children_size() != 1)
        {
            return false;
        }
        NodeLock set_ln(set);

        Node::pointer_t value(set->get_child(0));
        type_expr(value);

        switch(value->get_type())
        {
        case Node::node_t::NODE_STRING:
        case Node::node_t::NODE_INT64:
        case Node::node_t::NODE_FLOAT64:
        case Node::node_t::NODE_TRUE:
        case Node::node_t::NODE_FALSE:
        case Node::node_t::NODE_NULL:
        case Node::node_t::NODE_UNDEFINED:
        case Node::node_t::NODE_REGULAR_EXPRESSION:
            {
                Node::pointer_t clone(value->clone_basic_node());
                replace->replace_with(clone);
                replace = clone;
            }
            return true;

        default:
            // dynamic expression, can't
            // be resolved at compile time...
            return false;

        }
        /*NOTREACHED*/
    }

    return false;
}


void Compiler::var(Node::pointer_t var_node)
{
    // when variables are used, they are initialized
    // here, we initialize them only if they have
    // side effects; this is because a variable can
    // be used as an attribute and it would often
    // end up as an error (i.e. attributes not
    // found as identifier(s) defining another
    // object)
    NodeLock ln(var_node);
    size_t const vcnt(var_node->get_children_size());
    for(size_t v(0); v < vcnt; ++v)
    {
        Node::pointer_t variable_node(var_node->get_child(v));
        variable(variable_node, true);
    }
}


void Compiler::variable(Node::pointer_t variable_node, bool const side_effects_only)
{
    size_t const max_children(variable_node->get_children_size());

    // if we already have a type, we have been parsed
    if(variable_node->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_DEFINED)
    || variable_node->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES))
    {
        if(!side_effects_only)
        {
            if(!variable_node->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_COMPILED))
            {
                for(size_t idx(0); idx < max_children; ++idx)
                {
                    Node::pointer_t child(variable_node->get_child(idx));
                    if(child->get_type() == Node::node_t::NODE_SET)
                    {
                        Node::pointer_t expr(child->get_child(0));
                        expression(expr);
                        variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_COMPILED, true);
                        break;
                    }
                }
            }
            variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_INUSE, true);
        }
        return;
    }

    variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_DEFINED, true);
    variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_INUSE, !side_effects_only);

    bool const constant(variable_node->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_CONST));

    // make sure to get the attributes before the node gets locked
    // (we know that the result is true in this case)
    if(!get_attribute(variable_node, Node::attribute_t::NODE_ATTR_DEFINED))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, variable_node->get_position());
        msg << "get_attribute() did not return true as expected.";
        throw exception_exit(1, "get_attribute() did not return true as expected.");
    }

    NodeLock ln(variable_node);
    int set(0);

    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(variable_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::node_t::NODE_UNKNOWN:
            break;

        case Node::node_t::NODE_SET:
            {
                Node::pointer_t expr(child->get_child(0));
                if(expr->get_type() == Node::node_t::NODE_PRIVATE
                || expr->get_type() == Node::node_t::NODE_PUBLIC)
                {
                    // this is a list of attributes
                    ++set;
                }
                else if(set == 0
                     && (!side_effects_only || expr->has_side_effects()))
                {
                    variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_COMPILED, true);
                    variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_INUSE, true);
                    expression(expr);
                }
                ++set;
            }
            break;

        case Node::node_t::NODE_TYPE:
            // define the variable type in this case
            {
                variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_COMPILED, true);

                Node::pointer_t expr(child->get_child(0));
                expression(expr);
                if(!variable_node->get_type_node())
                {
                    ln.unlock();
                    variable_node->set_type_node(child->get_instance());
                }
            }
            break;

        default:
            Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, variable_node->get_position());
            msg << "variable has a child node of an unknown type.";
            throw exception_exit(1, "variable has a child node of an unknown type.");

        }
    }

    if(set > 1)
    {
        variable_node->to_var_attributes();
        if(!constant)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NEED_CONST, variable_node->get_position());
            msg << "a variable cannot be a list of attributes unless it is made constant and '" << variable_node->get_string() << "' is not constant.";
        }
    }
    else
    {
        // read the initializer (we're expecting an expression, but
        // if this is only one identifier or PUBLIC or PRIVATE then
        // we're in a special case...)
        add_variable(variable_node);
    }
}


void Compiler::add_variable(Node::pointer_t variable_node)
{
    // For variables, we want to save a link in the
    // first directive list; this is used to clear
    // all the variables whenever a frame is left
    // and enables us to declare local variables as
    // such in functions
    //
    // [i.e. local variables defined in a frame are
    // undefined once you quit that frame; we do that
    // because the Flash instructions don't give us
    // correct frame management and a goto inside a
    // frame would otherwise possibly use the wrong
    // variable value!]
    Node::pointer_t parent(variable_node);
    bool first(true);
    for(;;)
    {
        parent = parent->get_parent();
        switch(parent->get_type())
        {
        case Node::node_t::NODE_DIRECTIVE_LIST:
            if(first)
            {
                first = false;
                parent->add_variable(variable_node);
            }
            break;

        case Node::node_t::NODE_FUNCTION:
            // mark the variable as local
            variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_LOCAL, true);
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        case Node::node_t::NODE_CLASS:
        case Node::node_t::NODE_INTERFACE:
            // mark the variable as a member of this class or interface
            variable_node->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_MEMBER, true);
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        case Node::node_t::NODE_PROGRAM:
        case Node::node_t::NODE_PACKAGE:
            // variable is global
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        default:
            break;

        }
    }
}




}
// namespace as2js

// vim: ts=4 sw=4 et
