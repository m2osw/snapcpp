/* compiler_directive.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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



Node::pointer_t Compiler::directive_list(Node::pointer_t directive_list_node)
{
    size_t const p(f_scope->get_children_size());

    // TODO: should we go through the list a first time
    //       so we get the list of namespaces for these
    //       directives at once; so in other words you
    //       could declare the namespaces in use at the
    //       start or the end of this scope and it works
    //       the same way...

    size_t const max_children(directive_list_node->get_children_size());

    // get rid of any declaration marked false
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(directive_list_node->get_child(idx));
        if(get_attribute(child, Node::attribute_t::NODE_ATTR_FALSE))
        {
            child->to_unknown();
        }
    }

    bool no_access(false);
    Node::pointer_t end_list;

    // compile each directive one by one...
    {
        NodeLock ln(directive_list_node);
        for(size_t idx(0); idx < max_children; ++idx)
        {
            Node::pointer_t child(directive_list_node->get_child(idx));
            if(!no_access && end_list)
            {
                // err only once on this one
                no_access = true;
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INACCESSIBLE_STATEMENT, child->get_position());
                msg << "code is not accessible after a break, continue, goto, throw or return statement.";
            }
#if 0
fprintf(stderr, "Directive at ");
child.DisplayPtr(stderr);
fprintf(stderr, " (%d + 1 of %d)\n", idx, max);
#endif

            switch(child->get_type())
            {
            case Node::node_t::NODE_PACKAGE:
                // there is nothing to do on those
                // until users reference them...
                break;

            case Node::node_t::NODE_DIRECTIVE_LIST:
                // Recursive!
                end_list = directive_list(child);
                // TODO: we need a real control flow
                //       information to know whether this
                //       latest list had a break, continue,
                //       goto or return statement which
                //       was (really) breaking us too.
                break;

            case Node::node_t::NODE_LABEL:
                // labels do not require any compile whatever...
                break;

            case Node::node_t::NODE_VAR:
                var(child);
                break;

            case Node::node_t::NODE_WITH:
                with(child);
                break;

            case Node::node_t::NODE_USE: // TODO: should that move in a separate loop?
                use_namespace(child);
                break;

            case Node::node_t::NODE_GOTO:
                goto_directive(child);
                end_list = child;
                break;

            case Node::node_t::NODE_FOR:
                for_directive(child);
                break;

            case Node::node_t::NODE_SWITCH:
                switch_directive(child);
                break;

            case Node::node_t::NODE_CASE:
                case_directive(child);
                break;

            case Node::node_t::NODE_DEFAULT:
                default_directive(child);
                break;

            case Node::node_t::NODE_IF:
                if_directive(child);
                break;

            case Node::node_t::NODE_WHILE:
                while_directive(child);
                break;

            case Node::node_t::NODE_DO:
                do_directive(child);
                break;

            case Node::node_t::NODE_THROW:
                throw_directive(child);
                end_list = child;
                break;

            case Node::node_t::NODE_TRY:
                try_directive(child);
                break;

            case Node::node_t::NODE_CATCH:
                catch_directive(child);
                break;

            case Node::node_t::NODE_FINALLY:
                finally(child);
                break;

            case Node::node_t::NODE_BREAK:
            case Node::node_t::NODE_CONTINUE:
                break_continue(child);
                end_list = child;
                break;

            case Node::node_t::NODE_ENUM:
                enum_directive(child);
                break;

            case Node::node_t::NODE_FUNCTION:
                function(child);
                break;

            case Node::node_t::NODE_RETURN:
                end_list = return_directive(child);
                break;

            case Node::node_t::NODE_CLASS:
            case Node::node_t::NODE_INTERFACE:
                // TODO: any non-intrinsic function or
                //       variable member referenced in
                //       a class requires that the
                //       whole class be assembled.
                //       (Unless we can just assemble
                //       what the user accesses.)
                class_directive(child);
                break;

            case Node::node_t::NODE_IMPORT:
                import(child);
                break;

            // all the possible expression entries
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
            case Node::node_t::NODE_CALL:
            case Node::node_t::NODE_DECREMENT:
            case Node::node_t::NODE_DELETE:
            case Node::node_t::NODE_INCREMENT:
            case Node::node_t::NODE_MEMBER:
            case Node::node_t::NODE_NEW:
            case Node::node_t::NODE_POST_DECREMENT:
            case Node::node_t::NODE_POST_INCREMENT:
                expression(child);
                break;

            case Node::node_t::NODE_UNKNOWN:
                // ignore nodes marked as unknown ("nearly deleted")
                break;

            default:
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, child->get_position());
                    msg << "directive node '" << child->get_type_name() << "' not handled yet in Compiler::directive_list().";
                }
                break;

            }

            if(end_list && idx + 1 < max_children)
            {
                Node::pointer_t next(directive_list_node->get_child(idx + 1));
                if(next->get_type() == Node::node_t::NODE_CASE
                || next->get_type() == Node::node_t::NODE_DEFAULT)
                {
                    // process can continue with another case or default
                    // statement following a return, throw, etc.
                    end_list.reset();
                }
            }
        }
    }

    // The node may be a PACKAGE node in which case the "new variables"
    // does not apply (TODO: make sure of that!)
    if(directive_list_node->get_type() == Node::node_t::NODE_DIRECTIVE_LIST
    && directive_list_node->get_flag(Node::flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES))
    {
        size_t const max_variables(directive_list_node->get_variable_size());
        for(size_t idx(0); idx < max_variables; ++idx)
        {
            Node::pointer_t variable_node(directive_list_node->get_variable(idx));
            Node::pointer_t var_parent(variable_node->get_parent());
            if(var_parent && var_parent->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_TOADD))
            {
                // TBD: is that just the var declaration and no
                //      assignment? because the assignment needs to
                //      happen at the proper time!!!
                var_parent->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_TOADD, false);
                directive_list_node->insert_child(0, var_parent); // insert at the start!
            }
        }
        directive_list_node->set_flag(Node::flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES, false);
    }

    // go through the f_scope list and remove the "use namespace" that
    // were added while working on the items of this list
    // (why?!? because those are NOT like in C++, they are standalone
    // instructions... weird!)
    size_t max_use_namespace(f_scope->get_children_size());
    while(p < max_use_namespace)
    {
        max_use_namespace--;
        f_scope->delete_child(max_use_namespace);
    }

    return end_list;
}




}
// namespace as2js

// vim: ts=4 sw=4 et
