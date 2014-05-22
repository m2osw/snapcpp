/* compiler_compile.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


namespace as2js
{





bool Compiler::expression_new(Node::pointer_t new_node)
{
    //
    // handle the special case of:
    //    VAR name := NEW class()
    //

//fprintf(stderr, "BEFORE:\n");
//new_node.Display(stderr);
    Node::pointer_t call(new_node->get_child(0));
    if(!call)
    {
        return false;
    }

    if(call->get_type() != Node::NODE_CALL)
    {
        return false;
    }

    // get the function name
    Node::pointer_t id(call->get_child(0));
    if(id->get_type() != Node::NODE_IDENTIFIER)
    {
        return false;
    }

    // determine the types of the parameters to search a corresponding object or function
    Node::pointer_t params(call->get_child(1));
    size_t const count(params->get_children_size());
//fprintf(stderr, "ResolveCall() with %d expressions\n", count);
//new_node.Display(stderr);
    for(size_t idx(0); idx < count; ++idx)
    {
        expression(params->get_child(idx));
    }

    // resolve what is named
    Node::pointer_t resolution;
    if(!resolve_name(id, id, resolution, params, SEARCH_FLAG_GETTER))
    {
        // an error is generated later if this is a call and no function can be found
        return false;
    }

    // is the name a class or interface?
    if(resolution->get_type() != Node::NODE_CLASS
    && resolution->get_type() != Node::NODE_INTERFACE)
    {
        return false;
    }

    // move the nodes under CALL up one level
    Node::pointer_t type(call->get_child(0));
    Node::pointer_t expr(call->get_child(1));
    new_node->delete_child(0);      // remove the CALL
    new_node->append_child(type);   // replace with TYPE + parameters (LIST)
    new_node->append_child(expr);

//fprintf(stderr, "CHANGED TO:\n");
//new_node.Display(stderr);

    return true;
}


void Compiler::can_instantiate_type(Node::pointer_t expr)
{
    if(expr->get_type() != Node::NODE_IDENTIFIER) {
        // dynamic, cannot test at compile time...
        return;
    }

    Node::pointer_t inst(expr->get_link(Node::LINK_INSTANCE));
    if(inst->get_type() == Node::NODE_INTERFACE)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "you can only instantiate an object from a class. '" << expr->get_string() << "' is an interface.";
        return;
    }
    if(inst->get_type() != Node::NODE_CLASS)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "you can only instantiate an object from a class. '" << expr->get_string() << "' does not seem to be a class.";
        return;
    }

    // check all the functions and make sure none are [still] abstract
    // in this class...
    Node::pointer_t func;
    if(has_abstract_functions(inst, inst, func))
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_ABSTRACT, expr->get_position());
        msg << "the class '"
            << expr->get_string()
            << "' has an abstract function '"
            << func->get_string()
            << "' in file '"
            << func->get_position().get_filename()
            << "' at line #"
            << func->get_position().get_line()
            << " and cannot be instantiated. (If you have an overloaded version of that function it may have the wrong prototype.)";
        return;
    }

    // we're fine...
}


void Compiler::check_this_validity(Node::pointer_t expr)
{
    Node::pointer_t parent(expr);
    for(;;)
    {
        parent = parent->get_parent();
        if(!parent)
        {
            return;
        }
        switch(parent->get_type())
        {
        case Node::NODE_FUNCTION:
            // If we are in a static function, then we
            // don't have access to 'this'. Note that
            // it doesn't matter whether we're in a
            // class or not...
            if(parent->get_flag(Node::NODE_FUNCTION_FLAG_OPERATOR)
            || get_attribute(parent, Node::NODE_ATTR_STATIC)
            || get_attribute(parent, Node::NODE_ATTR_CONSTRUCTOR)
            || is_constructor(parent))
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_STATIC, expr->get_position());
                msg << "'this' cannot be used in a static function nor a constructor.";
            }
            return;

        case Node::NODE_CLASS:
        case Node::NODE_INTERFACE:
        case Node::NODE_PROGRAM:
        case Node::NODE_ROOT:
            return;

        default:
            break;

        }
    }
}


void Compiler::expression(Node::pointer_t expr, Node::pointer_t params)
{
    // we already came here on that one?
    if(expr->get_link(Node::LINK_TYPE))
    {
        return;
    }

    // try to optimize the expression before to compile it
    // (it can make a huge difference!)
    f_optimizer->optimize(expr);

    switch(expr->get_type())
    {
    case Node::NODE_STRING:
    case Node::NODE_INT64:
    case Node::NODE_FLOAT64:
    case Node::NODE_TRUE:
    case Node::NODE_FALSE:
        type_expr(expr);
        return;

    case Node::NODE_ARRAY_LITERAL:
        type_expr(expr);
        break;

    case Node::NODE_OBJECT_LITERAL:
        object_literal(expr);
        return;

    case Node::NODE_NULL:
    case Node::NODE_PUBLIC:
    case Node::NODE_PRIVATE:
    case Node::NODE_UNDEFINED:
        return;

    case Node::NODE_SUPER:
        check_super_validity(expr);
        return;

    case Node::NODE_THIS:
        check_this_validity(expr);
        return;

    case Node::NODE_ADD:
    case Node::NODE_ARRAY:
    case Node::NODE_AS:
    case Node::NODE_ASSIGNMENT_ADD:
    case Node::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::NODE_ASSIGNMENT_DIVIDE:
    case Node::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::NODE_ASSIGNMENT_MAXIMUM:
    case Node::NODE_ASSIGNMENT_MINIMUM:
    case Node::NODE_ASSIGNMENT_MODULO:
    case Node::NODE_ASSIGNMENT_MULTIPLY:
    case Node::NODE_ASSIGNMENT_POWER:
    case Node::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_ASSIGNMENT_SUBTRACT:
    case Node::NODE_BITWISE_AND:
    case Node::NODE_BITWISE_NOT:
    case Node::NODE_BITWISE_OR:
    case Node::NODE_BITWISE_XOR:
    case Node::NODE_CONDITIONAL:
    case Node::NODE_DECREMENT:
    case Node::NODE_DELETE:
    case Node::NODE_DIVIDE:
    case Node::NODE_EQUAL:
    case Node::NODE_GREATER:
    case Node::NODE_GREATER_EQUAL:
    case Node::NODE_IN:
    case Node::NODE_INCREMENT:
    case Node::NODE_INSTANCEOF:
    case Node::NODE_TYPEOF:
    case Node::NODE_IS:
    case Node::NODE_LESS:
    case Node::NODE_LESS_EQUAL:
    case Node::NODE_LIST:
    case Node::NODE_LOGICAL_AND:
    case Node::NODE_LOGICAL_NOT:
    case Node::NODE_LOGICAL_OR:
    case Node::NODE_LOGICAL_XOR:
    case Node::NODE_MATCH:
    case Node::NODE_MAXIMUM:
    case Node::NODE_MINIMUM:
    case Node::NODE_MODULO:
    case Node::NODE_MULTIPLY:
    case Node::NODE_NOT_EQUAL:
    case Node::NODE_POST_DECREMENT:
    case Node::NODE_POST_INCREMENT:
    case Node::NODE_POWER:
    case Node::NODE_RANGE:
    case Node::NODE_ROTATE_LEFT:
    case Node::NODE_ROTATE_RIGHT:
    case Node::NODE_SCOPE:
    case Node::NODE_SHIFT_LEFT:
    case Node::NODE_SHIFT_RIGHT:
    case Node::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_STRICTLY_EQUAL:
    case Node::NODE_STRICTLY_NOT_EQUAL:
    case Node::NODE_SUBTRACT:
        break;

    case Node::NODE_NEW:
        // TBD: should that be !expression_new() instead?
        //      (i.e. return immediately on failure)
        if(expression_new(expr))
        {
            return;
        }
        break;

    case Node::NODE_VOID:
        // If the expression has no side effect (i.e. doesn't
        // call a function, doesn't use ++ or --, etc.) then
        // we don't even need to keep it! Instead we replace
        // the void by undefined.
        if(expr->has_side_effects())
        {
            // we need to keep some of this expression
            //
            // TODO: we need to optimize better; this
            // should only keep expressions with side
            // effects and not all expressions; for
            // instance:
            //    void (a + b(c));
            // should become:
            //    void b(c);
            // (assuming that 'a' isn't a call to a getter
            // function which could have a side effect)
            break;
        }
        // this is what void returns, assuming the expression
        // had no side effect, that's all we need here
        expr = expr->create_replacement(Node::NODE_UNDEFINED);
        return;

    case Node::NODE_ASSIGNMENT:
        assignment_operator(expr);
        return;

    case Node::NODE_FUNCTION:
        function(expr);
        return;

    case Node::NODE_MEMBER:
        resolve_member(expr, params, SEARCH_FLAG_GETTER);
        return;

    case Node::NODE_IDENTIFIER:
    case Node::NODE_VIDENTIFIER:
        if(!special_identifier(expr))
        {
            Node::pointer_t resolution;
            if(resolve_name(expr, expr, resolution, params, SEARCH_FLAG_GETTER))
            {
                if(!replace_constant_variable(expr, resolution))
                {
                    Node::pointer_t current(expr->get_link(Node::LINK_INSTANCE));
                    if(current)
                    {
                        // TBD: I'm not exactly sure what this does right now, we
                        //      probably can ameliorate the error message, although
                        //      we should actually never get it!
                        throw exception_internal_error("The link instance of this VIDENTIFIER was already defined...");
                    }
                    expr->set_link(Node::LINK_INSTANCE, resolution);
                    Node::pointer_t type(resolution->get_link(Node::LINK_TYPE));
                    if(type)
                    {
                        expr->set_link(Node::LINK_TYPE, type);
                    }
                }
            }
            else
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_NOT_FOUND, expr->get_position());
                msg << "cannot find any variable or class declaration for: '" << expr->get_string() << "'.";
            }
        }
        return;

    case Node::NODE_CALL:
        resolve_call(expr);
        return;

    default:
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "unhandled expression data type \"" << expr->get_type_name() << "\".";
        }
        return;

    }

// When not returned yet, we want that expression to
// compile all the children nodes as expressions.
    size_t const max_children(expr->get_children_size());
    {
        NodeLock ln(expr);
        for(size_t idx(0); idx < max_children; ++idx)
        {
            Node::pointer_t child(expr->get_child(idx));
            // skip labels
            if(child->get_type() != Node::NODE_NAME)
            {
                expression(child);
            }
        }
    }

// Now check for operators to give them a type
    switch(expr->get_type())
    {
    case Node::NODE_ADD:
    case Node::NODE_SUBTRACT:
        if(max_children == 1)
        {
            unary_operator(expr);
        }
        else
        {
            binary_operator(expr);
        }
        break;

    case Node::NODE_BITWISE_NOT:
    case Node::NODE_DECREMENT:
    case Node::NODE_INCREMENT:
    case Node::NODE_LOGICAL_NOT:
    case Node::NODE_POST_DECREMENT:
    case Node::NODE_POST_INCREMENT:
        unary_operator(expr);
        break;

    case Node::NODE_BITWISE_AND:
    case Node::NODE_BITWISE_OR:
    case Node::NODE_BITWISE_XOR:
    case Node::NODE_DIVIDE:
    case Node::NODE_EQUAL:
    case Node::NODE_GREATER:
    case Node::NODE_GREATER_EQUAL:
    case Node::NODE_LESS:
    case Node::NODE_LESS_EQUAL:
    case Node::NODE_LOGICAL_AND:
    case Node::NODE_LOGICAL_OR:
    case Node::NODE_LOGICAL_XOR:
    case Node::NODE_MATCH:
    case Node::NODE_MAXIMUM:
    case Node::NODE_MINIMUM:
    case Node::NODE_MODULO:
    case Node::NODE_MULTIPLY:
    case Node::NODE_NOT_EQUAL:
    case Node::NODE_POWER:
    case Node::NODE_RANGE:
    case Node::NODE_ROTATE_LEFT:
    case Node::NODE_ROTATE_RIGHT:
    case Node::NODE_SCOPE:
    case Node::NODE_SHIFT_LEFT:
    case Node::NODE_SHIFT_RIGHT:
    case Node::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_STRICTLY_EQUAL:
    case Node::NODE_STRICTLY_NOT_EQUAL:
        binary_operator(expr);
        break;

    case Node::NODE_IN:
    case Node::NODE_CONDITIONAL:    // cannot be overwritten!
        break;

    case Node::NODE_ARRAY:
    case Node::NODE_ARRAY_LITERAL:
    case Node::NODE_AS:
    case Node::NODE_DELETE:
    case Node::NODE_INSTANCEOF:
    case Node::NODE_IS:
    case Node::NODE_TYPEOF:
    case Node::NODE_VOID:
        // nothing special we can do here...
        break;

    case Node::NODE_NEW:
        can_instantiate_type(expr->get_child(0));
        break;

    case Node::NODE_LIST:
        {
            // this is the type of the last entry
            Node::pointer_t child(expr->get_child(max_children - 1));
            expr->set_link(Node::LINK_TYPE, child->get_link(Node::LINK_TYPE));
        }
        break;

    case Node::NODE_ASSIGNMENT_ADD:
    case Node::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::NODE_ASSIGNMENT_DIVIDE:
    case Node::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::NODE_ASSIGNMENT_MAXIMUM:
    case Node::NODE_ASSIGNMENT_MINIMUM:
    case Node::NODE_ASSIGNMENT_MODULO:
    case Node::NODE_ASSIGNMENT_MULTIPLY:
    case Node::NODE_ASSIGNMENT_POWER:
    case Node::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_ASSIGNMENT_SUBTRACT:
        // TODO: we need to replace the intrinsic special
        //       assignment ops with a regular assignment
        //       (i.e. a += b becomes a = a + (b))
        binary_operator(expr);
        break;

    default:
        throw exception_internal_error("error: there is a missing entry in the 2nd switch of Compiler::expression()");

    }
}



}
// namespace as2js

// vim: ts=4 sw=4 et
