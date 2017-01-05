/* compiler_compile.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


/**********************************************************************/
/**********************************************************************/
/***  COMPILE  ********************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief "Compile" the code, which means optimize and make compatible.
 *
 * The following functions "compile" the code:
 *
 * \li It will optimize everything it can by reducing expressions that
 *     can be computed at "compile" time;
 * \li It transforms advanced features of as2js such as classes into
 *     JavaScript compatible code such as prototypes.
 *
 * On other words, it means that the compiler (1) tries to resolve all
 * the references that are found in the current tree; (2) loads the
 * libraries referenced by the different import instructions which
 * are necessary (or at least seem to be); (3) and run the optimizer
 * against the code at various times.
 *
 * The compiler calls the optimizer for you because it is important in
 * various places and the optimizations applied will vary depending on
 * the compiler changes and further changes may be applied after the
 * optimizations. So on return the tree is expected to be 100% compatible
 * with a JavaScript all browser interpreters and optimized as much as
 * possible to be output as minimized as can be.
 *
 * \param[in,out] root  The root node or program to compile.
 *
 * \return The number of errors generated while compiling. If zero, no errors
 *         so you can proceed with the tree.
 */
int Compiler::compile(Node::pointer_t& root)
{
    int const errcnt(Message::error_count());

    if(root)
    {
        // all the "use namespace ... / with ..." currently in effect
        f_scope = root->create_replacement(Node::node_t::NODE_SCOPE);

        if(root->get_type() == Node::node_t::NODE_PROGRAM)
        {
            program(root);
        }
        else if(root->get_type() == Node::node_t::NODE_ROOT)
        {
            NodeLock ln(root);
            size_t const max_children(root->get_children_size());
            for(size_t idx(0); idx < max_children; ++idx)
            {
                Node::pointer_t child(root->get_child(idx));
                if(child->get_type() == Node::node_t::NODE_PROGRAM)
                {
                    program(child);
                }
            }
        }
        else
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, root->get_position());
            msg << "the Compiler::compile() function expected a root or a program node to start with.";
        }
    }

    return Message::error_count() - errcnt;
}











// note that we search for labels in functions, programs, packages
// [and maybe someday classes, but for now classes can't have
// code and thus no labels]
void Compiler::find_labels(Node::pointer_t function_node, Node::pointer_t node)
{
    // NOTE: function may also be a program or a package.
    switch(node->get_type())
    {
    case Node::node_t::NODE_LABEL:
    {
        Node::pointer_t label(function_node->find_label(node->get_string()));
        if(label)
        {
            // TODO: test function type
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, function_node->get_position());
            msg << "label '" << node->get_string() << "' defined twice in the same program, package or function.";
        }
        else
        {
            function_node->add_label(node);
        }
    }
        return;

    // sub-declarations and expressions are just skipped
    // decls:
    case Node::node_t::NODE_FUNCTION:
    case Node::node_t::NODE_CLASS:
    case Node::node_t::NODE_INTERFACE:
    case Node::node_t::NODE_VAR:
    case Node::node_t::NODE_PACKAGE:    // ?!
    case Node::node_t::NODE_PROGRAM:    // ?!
    // expr:
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
        return;

    default:
        // other nodes may have children we want to check out
        break;

    }

    NodeLock ln(node);
    size_t max_children(node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(node->get_child(idx));
        find_labels(function_node, child);
    }
}


































void Compiler::print_search_errors(Node::pointer_t name)
{
    // all failed, check whether we have errors...
    if(f_err_flags == SEARCH_ERROR_NONE)
    {
        return;
    }

    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_MATCH, name->get_position());
    msg << "the name '" << name->get_string() << "' could not be resolved because:\n";

    if((f_err_flags & SEARCH_ERROR_PRIVATE) != 0)
    {
        msg << "   You cannot access a private class member from outside that very class.";
    }
    if((f_err_flags & SEARCH_ERROR_PROTECTED) != 0)
    {
        msg << "   You cannot access a protected class member from outside a class or its derived classes.";
    }
    if((f_err_flags & SEARCH_ERROR_PROTOTYPE) != 0)
    {
        msg << "   One or more functions were found, but none matched the input parameters.";
    }
    if((f_err_flags & SEARCH_ERROR_WRONG_PRIVATE) != 0)
    {
        msg << "   You cannot use the private attribute outside of a package or a class.";
    }
    if((f_err_flags & SEARCH_ERROR_WRONG_PROTECTED) != 0)
    {
        msg << "   You cannot use the protected attribute outside of a class.";
    }
    if((f_err_flags & SEARCH_ERROR_PRIVATE_PACKAGE) != 0)
    {
        msg << "   You cannot access a package private declaration from outside of that package.";
    }
}


}
// namespace as2js

// vim: ts=4 sw=4 et
